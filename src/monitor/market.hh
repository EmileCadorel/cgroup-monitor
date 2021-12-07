#pragma once

#include <map>
#include <string>

#include <monitor/cgroup/vminfo.hh>

namespace monitor {

    struct MarketConfig {
	float marketIncrement;
	float cyclePrice;
	float cycleIncrement;
	float baseCycle;
	float windowScale;
	float moneyIncrement;
	float triggerIncrement;
	float triggerDecrement;
	unsigned int windowSize;
	unsigned int windowMultiplier;
	unsigned int windowMaximumTurn;
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
	 * @returns: the number of authorized cycles for each VMs (@warning: based on second, must be reported to tick time)
	 */
	std::map <std::string, unsigned long> update (const std::map <std::string, cgroup::VMInfo> & vms);


	/**
	 * @returns: the accounts of the VMs
	 */
	const std::map <std::string, unsigned long> & getAccounts () const;

	/**
	 * @returns: the number of iterations that were necessary in the first selling 
	 */
	unsigned long getFirstNbIterations () const;

	/**
	 * @returns: the number of iterations that were necessary in the second selling 
	 */
	unsigned long getSecondNbIterations () const;


	/**
	 * @returns: the number of cycles sold in the first selling
	 */
	unsigned long getFirstMarketSold () const;

	/**
	 * @returns: the number of cycles that were sold to no one
	 */
	unsigned long getLost () const;

    private :

	/**
	 * Run the market
	 * @params: 
	 *   - vms: the vms running on the node
	 *   - price: the price of one cycle
	 *   - increment: the percentage to add to the actual consumed percentage of the VMs
	 */
	std::map <std::string, unsigned long> buyCycles (const std::map <std::string, cgroup::VMInfo> & vms, const std::map <std::string, unsigned long> & current, std::map <std::string, unsigned long> & needs, unsigned long & market, float cyclePrice, unsigned long & iterations, unsigned long & allNeeded);

	
	/**
	 * Compute the window sizes of the VMs 
	 * @returns: the size of the selling window for the VMs
	 */
	unsigned long computeWindowSize (const std::string & name, const cgroup::VMInfo & vm) const;
	
	/**
	 * @returns: the base cycles, to guarantee the minimum quality of service before running the selling parts
	 */
	std::map <std::string, unsigned long> sellBaseCycles (const std::map <std::string, cgroup::VMInfo> & vms, unsigned long & all, std::map <std::string, unsigned long> & needs) const;
	
	
	/**
	 * Increment the accounts of the running VMs
	 */
	void incrementAccounts (const std::map <std::string, cgroup::VMInfo> & vms);
	
    };
    
}
