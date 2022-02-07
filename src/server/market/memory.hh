#pragma once
#include <monitor/libvirt/_.hh>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace server {

    namespace market {


	struct MemoryMarketConfig {
	    /// The quantity of memory available on the host in Kb
	    unsigned long memory;
	    
	    /// The percentage of usage of memory in VM before an increment of memory allocation
	    float triggerIncrement;

	    /// The speed of the increment of memory
	    float increasingSpeed;

	    /// The percentage of usage of memory in VM before an decrement of memory allocation
	    float triggerDecrement;
	    
	    /// The speed of the decrement of memory
	    float decreasingSpeed;

	    /// The bidding window size
	    unsigned long windowSize;
	};
	
	/**
	 * Market for the memory resource allocations
	 */
	class MemoryMarket {

	    /// The libvirt connection
	    monitor::libvirt::LibvirtClient & _libvirt;

	    /// The configuration of the market
	    MemoryMarketConfig _config;

	    /// The accounts of the running VMs
	    std::map <std::string, unsigned long> _accounts;

	public :
	    
	    MemoryMarket (monitor::libvirt::LibvirtClient & client);

	    /**
	     * Change the configuration of the market
	     */
	    void setConfig (MemoryMarketConfig cfg);
	    
	    /**
	     * Execute an iteration of the market
	     * @info: this automatically updates the memory quotas of the VMs
	     */
	    void run ();

	    /**
	     * Reset the accounts
	     */
	    void reset ();
	    
	    /**
	     * @returns: a json containing the market information of the current tick
	     */
	    nlohmann::json dumpLogs () const;

	private :

	    /**
	     * Bidding part of the market
	     * @params: 
	     *   - allocated: the current allocation (@return)
	     *   - buyers: the list of buyers
	     *   - market: the quantity of memory that can be sold
	     * @returns: 
	     *   - allNeeded: the sum of unsold memory
	     *   - market: the quantity of memory that was not sold
	     *   - buyers: the list of VM that failed to buy 
	     *   - allocated: the new allocations
	     */
	    void buyKbs (std::map <std::string, unsigned long> & allocated,
			   std::map <std::string, unsigned long> & buyers,
			   unsigned long & market,
			   unsigned long & allNeeded);
	    

	    /**
	     * Selling the base alloc of the VMs (guaranteing minimal allocation)
	     * @params: 
	     *    - vms: the list of running VMs
	     *    - market: the quantity of memory (in Kb) available on the host
	     * @returns: 
	     *   - buyers: the list of VMs that will participate to the bidding (associated to the quantity of alloc they will try to buy)
	     *   - .0: the default allocations
	     */
	    std::map <std::string, unsigned long> sellBaseKbs (const std::map <std::string, monitor::libvirt::LibvirtVM*> & vms,
								 unsigned long & market,
								 std::map <std::string, unsigned long> & buyers);

	    /**
	     * Increase the accounts of a given VM
	     * @params: 
	     *   - vmName: the name of the VM
	     *   - money: the quantity of memory to add to the VM
	     */
	    void increaseMoney (const std::string & vmName, unsigned long money);
	    
	};
	
    }

}
