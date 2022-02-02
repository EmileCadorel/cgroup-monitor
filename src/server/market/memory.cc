#include "memory.hh"


namespace server {

    namespace market {


	MemoryMarket::MemoryMarket (monitor::libvirt::LibvirtClient & client) : _libvirt (client)
	{}

	void MemoryMarket::setConfig (MemoryMarketConfig cfg) {
	    this-> _config = cfg;
	}

	void MemoryMarket::run () {
	    auto & vms = this-> _libvirt.getRunningVMs ();
	    if (vms.size () == 0) return;

	    // The market is the quantity of memory the host is providing to the VMs
	    unsigned long market = this-> _config.memory;
	    std::map <std::string, unsigned long> buyers;

	    auto allocated = this-> sellBaseKbs (vms, market, buyers);

	    // Run the auction, for the VMs that needs more memory than the guaranteed nominal
	    unsigned long allNeeded = 0;
	    this-> buyKbs (allocated, buyers, market, allNeeded);

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
		vms.find (it.first)-> second-> getMemoryController ().setAllocatedMemory (it.second); 
	    }
	}

	nlohmann::json MemoryMarket::dumpLogs () const {
	    nlohmann::json j;
	    nlohmann::json j2;
	    for (auto & v : this-> _accounts) {
		j2 [v.first] = v.second;
	    }
	    
	    j ["accounts"] = j2;

	    return j;
	}

	void MemoryMarket::buyKbs (std::map <std::string, unsigned long> & allocated,
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
			if (bought != 0) { /// The VM bought some Kbs
			    this-> _accounts [v-> first] = money - bought; // we remove the money from its account
			    allocated [v-> first] = allocated [v-> first] + bought; // we update its allocation
			    market = market - bought; // we remove the bought Kbs from the market
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


	std::map <std::string, unsigned long> MemoryMarket::sellBaseKbs (
	    const std::map <std::string, monitor::libvirt::LibvirtVM*> & vms,
	    unsigned long & market,
	    std::map <std::string, unsigned long> & buyers)
	{
	    std::map <std::string, unsigned long> allocated;
	    for (auto & v : vms) {
		unsigned long usage = v.second-> getMemoryController ().getGuestUsed () + v.second-> getMemoryController ().getSwapping ();
		unsigned long max = v.second-> getMemoryController ().getMaxMemory ();
		unsigned long capp = v.second-> getMemoryController ().getAllocated ();
		auto percUsage = v.second-> getMemoryController ().getRelativePercentUsed ();
		float slope = v.second-> getMemoryController ().getSlope ();
		unsigned long min = 1048576; // 1GB is the minimal
		unsigned long current = min;

		std::cout << slope << " " << percUsage << std::endl;
		current = std::max (min, std::min (usage + min, max));
		if (slope < -0.1f || slope > 0.1f) {
		    if (percUsage > this-> _config.triggerIncrement) {
			current = std::max (min, std::min (max, (unsigned long) ((usage + min) * (1.0 + this-> _config.increasingSpeed))));
		    } else if (percUsage < this-> _config.triggerDecrement) {
			current = std::max (min, std::min (max, (unsigned long) ((capp + min) * (1.0 - this-> _config.decreasingSpeed))));
		    }		
		} 
				
		allocated[v.first] = current;		
	    }
	    return allocated;
	}

	void MemoryMarket::increaseMoney (const std::string & vmName, unsigned long money) {
	    auto fnd = this-> _accounts.find (vmName);
	    if (fnd == this-> _accounts.end ()) {
		this-> _accounts [vmName] = money;
	    } else {
		fnd-> second = fnd-> second + money;
	    }
	}
		
    }
    
}

    
