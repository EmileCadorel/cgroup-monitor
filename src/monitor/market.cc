#include <monitor/market.hh>
#include <sys/sysinfo.h>
#include <algorithm>  
#include <monitor/utils/log.hh>


using namespace monitor::cgroup;
using namespace monitor::utils;

namespace monitor {


    Market::Market () {
	this-> _config = MarketConfig {
	    0.3,
	    0.95,
	    0.5,
	    0.1,
	    10000
	};
    }

    Market::Market (MarketConfig conf) {
	this-> _config = conf;
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================       MARKET / AUCTION       =========================
     * ================================================================================
     * ================================================================================
     */

    
    /**
     * Cf. market.hh
     */
    std::map <std::string, unsigned long> Market::update (const std::map <std::string, VMInfo> & vms) {
	if (vms.size () == 0) return {};

	/// The market is the number micro seconds in one second * the number of CPUs on the machine
	/// everything in the market is reported to the second, the VMInfo, and GroupManager make the relation to tick, it is easier that way
	unsigned long market = ((unsigned long) (get_nprocs () * 1000000));
	std::map <std::string, unsigned long> buyers;

	/// Sell the base, cycle and compute the list of VMs, that needs more cycles than what they are consuming
	auto allocated = this-> sellBaseCycles (vms, market, buyers);

	/// Run the auction, for the VMs that needs more cycles than the nominal
	unsigned long allNeeded = 0;
	this-> buyCycles (vms, allocated, buyers, market, this-> _firstIteration, allNeeded);

	/// Compute the number of cycles that are sold since the beginning, for debugging infos
	this-> _firstMarket = (get_nprocs () * 1000000) - market;

	// Rest some cycle that have not been sold, so there is some room for optimization	
	if (market > 0) {
	    long allSold = market;
	    long rest = std::min (allNeeded, market);
	    for (auto & v : buyers) { // we split the rest of the market between all the VMs that failed to buy
		float percent = (float) (v.second) / (float) allNeeded; // Implication of the VMs in the market 
		unsigned long add = (unsigned long) (percent * rest);
		allocated [v.first] += add;
		allSold -= add;	   
	    }
	    
	    this-> _lost = allSold;
	} else this-> _lost = 0;

	return allocated;
    }    

    /**
     * Cf. market.hh
     */
    void Market::buyCycles (const std::map <std::string, cgroup::VMInfo> & vms,
			    std::map <std::string, unsigned long> & allocated,
			    std::map <std::string, unsigned long> & buyers,
			    unsigned long & market,
			    unsigned long & iterations,
			    unsigned long & allNeeded)
	
    {
	/// The list of VMs that failed their bidding, because they have no money
	std::map <std::string, unsigned long> failed;
	iterations = 0;	
	while (market > 0 && buyers.size () > 0) {
	    iterations += 1; 
	    for (auto v = buyers.cbegin () ; v != buyers.cend () ; ) { // we cannot use : for (auto & v : buyers), because we need to erase elements in the map
		auto money = this-> _accounts [v-> first];
		if (v-> second != 0) {
		    unsigned long windowSize = std::min (this-> _config.windowSize, money);

		    /// The VM can buy at most, what they can (money, as windowSize), what they need (v-> second), or what is left in the market
		    auto bought = std::min (std::min (windowSize, v-> second), market);
		    if (bought != 0) { /// The VM bought some cycles
			this-> _accounts [v-> first] = money - bought; // we remove the money from its account
			allocated [v-> first] = allocated [v-> first] + bought; // we update its allocation
			market = market - bought; // we remove the bought cycles from the market
			buyers [v-> first] -= bought; // we remove the needs
			v ++; // next iteration 
		    } else { // bought nothing (or the VM has no money, or the market is empty)
			failed.emplace (v-> first, v-> second); // Insert in failed for final phase
			allNeeded += v-> second; // sum of all the failed
			buyers.erase (v ++); // remove the VM from the buyers, the next iteration would fail as well
		    }
		} else {
		    buyers.erase (v ++); // The VM has no need, we remove it from the buyers
		}
	    }
	}

	buyers = std::move (failed); // The buyers that are staying in the buyers queue are those who failed to buy (no money, or empty market)
    }


    /**
     * Cf. market.hh
     */
    std::map <std::string, unsigned long> Market::sellBaseCycles (const std::map <std::string, cgroup::VMInfo> & vms,
								  unsigned long & market,
								  std::map <std::string, unsigned long> & buyers)
    {
	std::map <std::string, unsigned long> allocated;
	for (auto & v : vms) {
	    unsigned long usage = v.second.getAbsoluteConso ();
	    unsigned long nominal = this-> _config.baseCycle * v.second.getMaximumConso ();
	    unsigned long max = v.second.getMaximumConso ();
	    float perc_usage = v.second.getRelativePercentConso () / 100.0;
	    unsigned long capp = v.second.getAbsoluteCapping ();
	    logging::info (v.first, "usage:", usage, "nominal:", nominal, "max:", max, "perc:", perc_usage, "cap:", capp, "div:", (float)usage/(float)capp);
	    /// If The VM usage is lower than the nominal frequency, we give some money
	    if (usage < nominal) {
		this-> increaseMoney (v.first, nominal - usage);		
	    } else this-> increaseMoney (v.first, 0);
	    
	    /**
	     * We have three cases : 
	     *  - 1) The VMs uses less than the decrease trigger
	     */
	    if (perc_usage < this-> _config.triggerDecrement) {
		/// We decrease the speed of the VM by a bit 
		unsigned long decrease = capp * (1.0 - this-> _config.decreasingSpeed);
		unsigned long current = std::min (nominal, decrease);
		logging::info (v.first, "Less:", decrease, " ", capp, " ", current, " ", nominal);
		allocated [v.first] = current;
		market -= current;

		/// If the VM needs more than nominal, we add it to the buyers for bidding
		if (usage > nominal) buyers [v.first] = std::min (max - nominal, decrease - nominal);
	    }

	    /**
	     *   - 2) The VM usage is higher than the increase trigger
	     */
	    else if (perc_usage > this-> _config.triggerIncrement) {
		logging::info (v.first, "Upper");
		/// We set the speed of the VM to full capacity
		
		auto cap = max;
		buyers [v.first] = cap - nominal;
		allocated [v.first] = nominal;
		market -= nominal;
	    }

	    /**
	     *   - 3) The VM usage is between the two triggers
	     */
	    else {
		logging::info (v.first, "None");
	    	/// We do nothing ?
	    }	    
	}

	return allocated;
    }

    void Market::increaseMoney (const std::string & vmName, unsigned long money) {
	auto fnd = this-> _accounts.find (vmName);
	if (fnd == this-> _accounts.end ()) {
	    this-> _accounts [vmName] = money;
	} else {
	    fnd-> second = fnd-> second + money;
	}
    }
    

    /**
     * ================================================================================
     * ================================================================================
     * =========================           GETTERS            =========================
     * ================================================================================
     * ================================================================================
     */


    const std::map <std::string, unsigned long> & Market::getAccounts () const {
	return this-> _accounts;
    }
    
    unsigned long Market::getFirstNbIterations () const {
	return this-> _firstIteration;
    }

    unsigned long Market::getSecondNbIterations () const {
	return this-> _secondIteration;
    }

    unsigned long Market::getFirstMarketSold () const {
	return this-> _firstMarket;
    }

    unsigned long Market::getLost () const {
	return this-> _lost;
    }

    
}
