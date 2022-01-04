#pragma once

#include <map>
#include <string>

#include <monitor/cgroup/vminfo.hh>

namespace monitor {

    namespace cgroup {
    
	struct MarketConfig {
	    unsigned long cpuFreq;	    
	    float triggerIncrement;
	    float triggerDecrement;
	    float increasingSpeed;
	    float decreasingSpeed;
	    unsigned long windowSize;
	};
    
    
	class Market {

	    /// The accounts of the different VMs
	    std::map <std::string, unsigned long> _accounts;

	    /// The configuration of the market
	    MarketConfig _config;

	    /// The number of lost cycles in the last iteration
	    unsigned long _lost = 0;

	    /// Number of iteration in the first selling
	    unsigned long _firstIteration = 0;

	    /// Number of iteration in the second selling
	    unsigned long _secondIteration = 0;

	    /// The number of cycle sold in the first selling
	    unsigned long _firstMarket = 0;
       	
	public:

	    /**
	     * Create a market with the default configuration
	     */
	    Market ();

	    /**
	     * Create a market with a custom configuration
	     */
	    Market (MarketConfig conf);

	    /**
	     * Run the market selling of cycles. 
	     * @warning: this function does not take into account relative tick, and consider everything in absolute (based on tick of 1 second)
	     * @params: 
	     *   - vms: the list of running VMs cgroup on the node
	     * @returns: the number of authorized cycles for each VMs (@warning: based on second - i.e. AbsoluteCapping, must be reported to tick time)
	     */
	    std::map <std::string, unsigned long> update (const std::map <std::string, cgroup::VMInfo> & vms);


	    /**
	     * @returns: the accounts of the VMs
	     */
	    const std::map <std::string, unsigned long> & getAccounts () const;

	    /**
	     * @returns: the number of iterations that were necessary in the first auction
	     */
	    unsigned long getFirstNbIterations () const;

	    /**
	     * @returns: the number of iterations that were necessary in the second auction (no relevant in this version)
	     */
	    unsigned long getSecondNbIterations () const;


	    /**
	     * @returns: the number of cycles sold in the first selling, and in the base selling (meaning all the cycles that are non free.
	     */
	    unsigned long getFirstMarketSold () const;

	    /**
	     * @returns: the number of cycles that were sold to no one (ideally this is 0, otherwise they are lost)
	     * @info: there is a case in which it can be non null: every VMs has money, and use less that this-> _config.triggerIncrement cycles
	     */
	    unsigned long getLost () const;

	private :

	    /**
	     * Run the market, "bidding" if we can call it that way
	     * @params: 
	     *   - vms: the vms running on the node
	     *   - market: the number of cycles that can be sold in total
	     *   - buyers: the number of cycles needed by the VMs
	     *   - allocated: the current allocated cycles to each VMs
	     * @ref_returns: 
	     *    - market: remove all the cycles that are sold
	     *    - allocated: update the allocations
	     *    - iterations: the number of iterations required by the auction
	     *    - buyers: the number of cycles the VMs failed to buy
	     *    - allNeeded: all the cycles that are needed by the VMs at the end (SUM (needs.values))
	     * @side_effects: 
	     *    - update this-> _accounts, by removing the money spent by the VMs on buying cycles
	     */
	    void buyCycles (const std::map <std::string, cgroup::VMInfo> & vms, std::map <std::string, unsigned long> & allocated, std::map <std::string, unsigned long> & buyers, unsigned long & market, unsigned long & iterations, unsigned long & allNeeded);

		
	    /**
	     * @params: 
	     *   - vms: all the vms infos
	     *   - market: the current market
	     * @ref_returns: 
	     *   - market: updated, removed all the cycles that are sold by default
	     *   - buyers: all the VMs needing more than their nominal frequency, and that will participate to the market
	     * @returns: the base cycles, to guarantee the minimum quality of service before running the selling parts
	     * @side_effects: 
	     *   - update this-> _accounts, by adding the base money of this current monitor loop and removing the money of the base selling
	     */
	    std::map <std::string, unsigned long> sellBaseCycles (const std::map <std::string, cgroup::VMInfo> & vms, unsigned long & market,  std::map <std::string, unsigned long> & buyers);


	    /**
	     * Increase the money of a given vm
	     * @params: 
	     *    - vmName: the name of the vm
	     *    - amount: the amount of money to add
	     * @side_effects: 
	     *  	 *   - update this-> _accounts
	     */
	    void increaseMoney (const std::string & vm, unsigned long amount);
	
	};
    }    
}
