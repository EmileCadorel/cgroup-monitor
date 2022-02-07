#pragma once
#include <monitor/libvirt/_.hh>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace server {

    namespace market {

	struct CpuMarketConfig {
	    int cpuFreq; 
	    float triggerIncrement;
	    float triggerDecrement;
	    float increasingSpeed;
	    float decreasingSpeed;
	    unsigned long windowSize;
	};


	/**
	 * Market for the cpu resource allocations
	 */
	class CpuMarket {

	    /// The libvirt connection
	    monitor::libvirt::LibvirtClient & _libvirt;

	    /// The accounts of the running vms
	    std::map <std::string, unsigned long> _accounts;

	    /// The configuration of the market
	    CpuMarketConfig _config;
	    
	public:

	    CpuMarket (monitor::libvirt::LibvirtClient & client);
	    
	    /**
	     * @params: 
	     *   - client: the libvirt client
	     */
	    CpuMarket (monitor::libvirt::LibvirtClient & client, CpuMarketConfig config);

	    /**
	     * Change the config of the market
	     */
	    void setConfig (CpuMarketConfig cfg);
	    
	    /**
	     * Execute an iteration of the market
	     * @info: this automatically updates the cpu quotas of the VMs
	     */
	    void run ();

	    /**
	     * Reset the accounts
	     */
	    void reset ();
	    
	    /**
	     * @returns: a json containing the market informations of the current tick
	     */
	    nlohmann::json dumpLogs () const;
	    
	private :
	    
	    /**
	     * Bidding part of the market 
	     */
	    void buyCycles (std::map <std::string, unsigned long> & allocated,
			    std::map <std::string, unsigned long> & buyers,
			    unsigned long & market,
			    unsigned long & allNeeded);

	    /**
	     * Selling the base cycles of the VMs (guarantee of nominal frequency)
	     */
	    std::map <std::string, unsigned long> sellBaseCycles (const std::map <std::string, monitor::libvirt::LibvirtVM*> & vms,
								  unsigned long & market,
								  std::map <std::string, unsigned long> & buyers);

	    /**
	     * Increase the account money of a given VM
	     */
	    void increaseMoney (const std::string & vmName, unsigned long money);
	};
	
    }

}
