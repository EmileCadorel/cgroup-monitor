#include "cpu.hh"
#include <sys/sysinfo.h>
#include <algorithm>  
#include <monitor/utils/log.hh>

using namespace monitor::libvirt;
using namespace monitor::utils;

namespace server {

    namespace market {

	CpuMarket::CpuMarket (monitor::libvirt::LibvirtClient & client) :
	    _libvirt (client)
	{}
	
	CpuMarket::CpuMarket (monitor::libvirt::LibvirtClient & client, CpuMarketConfig cfg) :
	    _libvirt (client),
	    _config (cfg)
	{}

	void CpuMarket::setConfig (CpuMarketConfig cfg) {
	    this-> _config = cfg;
	}

	nlohmann::json CpuMarket::dumpLogs () const {
	    nlohmann::json j;
	    nlohmann::json j2;
	    for (auto & v : this-> _accounts) {
		j2 [v.first] = v.second;
	    }
	    
	    j ["accounts"] = j2;

	    return j;
	}
	
	
	void CpuMarket::run () {
	    auto & vms = this-> _libvirt.getRunningVMs ();
	    if (vms.size () == 0) return;

	    /// The market is the number micro seconds in one second * the number of CPUs on the machine
	    /// everything in the market is reported to the second, the VMInfo, and GroupManager make the relation to tick, it is easier that way
	    unsigned long market = ((unsigned long) (get_nprocs () * 1000000));
	    std::map <std::string, unsigned long> buyers;

	    /// Sell the base, cycle and compute the list of VMs, that needs more cycles than what they are consuming
	    auto allocated = this-> sellBaseCycles (vms, market, buyers);
	    /// Run the auction, for the VMs that needs more cycles than the nominal
	    unsigned long allNeeded = 0;
	    this-> buyCycles (allocated, buyers, market, allNeeded);

	    // Rest some cycle that have not been sold, so there is some room for optimization	
	    if (market > 0) {
	    	long notSold = market;
	    	long rest = std::min (allNeeded, market);
	    	for (auto & v : buyers) { // we split the rest of the market between all the VMs that failed to buy
	    	    float percent = (float) (v.second) / (float) allNeeded; // Implication of the VMs in the market 
	    	    unsigned long add = (unsigned long) (percent * rest);
	    	    allocated [v.first] += add;
	    	    notSold -= add;	   
	    	}	    
	    }

	    for (auto it : allocated) {
		vms.find (it.first)-> second.getCpuController ().setQuota (it.second, 10000);
	    }
	}

	void CpuMarket::buyCycles (std::map <std::string, unsigned long> & allocated,
				   std::map <std::string, unsigned long> & buyers,
				   unsigned long & market,
				   unsigned long & allNeeded)
	
	{
	    /// The list of VMs that failed their bidding, because they have no money
	    std::map <std::string, unsigned long> failed;
	    while (market > 0 && buyers.size () > 0) {
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


	std::map <std::string, unsigned long> CpuMarket::sellBaseCycles (const std::map <std::string, LibvirtVM> & vms,
									 unsigned long & market,
									 std::map <std::string, unsigned long> & buyers)
	{
	    std::map <std::string, unsigned long> allocated;
	    for (auto & v : vms) {
		unsigned long usage = v.second.getCpuController ().getConsumption ();
		unsigned long max = v.second.vcpus () * 1000000;
		
		unsigned long nominal = ((float) v.second.freq ()) / ((float) this-> _config.cpuFreq) * max;
		unsigned long capp = v.second.getCpuController ().getAbsoluteCapping ();
		
		float perc_usage = v.second.getCpuController ().getRelativePercentConsumption () / 100.0f;
		double slope = v.second.getCpuController ().getSlope ();	    

		/**
		 * We have three cases : 
		 *  - 1) The VMs uses less than the decrease trigger
		 */
		if (slope > -0.1f && slope < 0.1f) {
		    unsigned long increase = std::min (max, (unsigned long) (usage + max * 0.01));
		    unsigned long current = std::min (nominal, increase);
		    allocated [v.first] = current;
		    market -= current;
		    if (increase > nominal) {
			this-> increaseMoney (v.first, 0);
			buyers [v.first] = std::min (max - nominal, increase - nominal);
		    } else {
			this-> increaseMoney (v.first, nominal - increase);
		    }		    
		} else if (perc_usage < this-> _config.triggerDecrement) {
		    /// We decrease the speed of the VM by a bit 
		    unsigned long decrease = std::max (usage, (unsigned long) (capp * (1.0 - this-> _config.decreasingSpeed)));
		    unsigned long current = std::min (nominal, decrease);
		    allocated [v.first] = current;
		    market -= current;

		    /// If the VM needs more than nominal, we add it to the buyers for bidding
		    if (decrease > nominal) {
			this-> increaseMoney (v.first, 0);
			buyers [v.first] = std::min (max - nominal, decrease - nominal);
		    } else {
			this-> increaseMoney (v.first, nominal - decrease);
		    }
		}

		/**
		 *   - 2) The VM usage is higher than the increase trigger
		 */
		else if (perc_usage > this-> _config.triggerIncrement) {
		    /// We increase the speed of the VM by a bit		
		    unsigned long increase = capp * (1.0 + this-> _config.increasingSpeed);
		    unsigned long current = std::min (nominal, increase);
		    allocated [v.first] = current;
		    market -= current;

		    /// If the VM needs more than nominal, we add it to the buyers for bidding
		    if (increase > nominal) {
			this-> increaseMoney (v.first, 0);
			//market += (capp * this-> _config.increasingSpeed) / 3; // recycling
			buyers [v.first] = std::min (max - nominal, increase - nominal);
		    } else {
			this-> increaseMoney (v.first, nominal - increase);
		    }
		}

		/**
		 *   - 3) The VM usage is between the two triggers, or the slope is really flat
		 */
		else {
		    unsigned long current = std::min (nominal, capp);		
		    allocated [v.first] = current;
		    market -= current;
		    
		    /// If the VM needs more than nominal, we add it to the buyers for bidding
		    if (capp > nominal) {
			this-> increaseMoney (v.first, 0);
			buyers [v.first] = std::min (max - nominal, capp - nominal);
		    } else {
			this-> increaseMoney (v.first, nominal - capp);
		    }
		}
	    }

	    return allocated;
	}

	void CpuMarket::increaseMoney (const std::string & vmName, unsigned long money) {
	    auto fnd = this-> _accounts.find (vmName);
	    if (fnd == this-> _accounts.end ()) {
		this-> _accounts [vmName] = money;
	    } else {
		fnd-> second = fnd-> second + money;
	    }
	}

	
    }
    

}
