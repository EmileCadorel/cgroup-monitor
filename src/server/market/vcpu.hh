#pragma once
#include <monitor/libvirt/_.hh>
#include <string>
#include <nlohmann/json.hpp>
#include <vector>
#include <list>

namespace server {

    namespace market {
	
	struct VCPUMarketConfig {
	    int cpuFreq; 
	    float triggerIncrement;
	    float triggerDecrement;
	    float increasingSpeed;
	    float decreasingSpeed;
	    unsigned long windowSize;
	};

	/**	   
	 * Market for the vcpu resource allocations
	 */
	class VCPUMarket {

	    /// The libvirt connection monitoring the vcpu consumptions
	    monitor::libvirt::LibvirtClient & _libvirt;

	    /// THe configuration of the market
	    VCPUMarketConfig _config;

	public:

	    VCPUMarket (monitor::libvirt::LibvirtClient & client);
	    
	    /**
	     * @params: 
	     *   - client: the libvirt client
	     */
	    VCPUMarket (monitor::libvirt::LibvirtClient & client, VCPUMarketConfig config);

	    /**
	     * Change the config of the market
	     */
	    void setConfig (VCPUMarketConfig cfg);
	    
	    /**
	     * Execute an iteration of the market
	     * @info: this automatically updates the cpu quotas of the VMs
	     */
	    void run ();

	    /**
	     * Reset the accounts
	     */
	    void reset ();
	    	    
	private :
	    
	    /**
	     * Bidding part of the market 
	     */
	    std::list <monitor::libvirt::control::LibvirtVCPUController*> buyCycles (std::list <monitor::libvirt::control::LibvirtVCPUController*> & buyers,
										     long & market,
										     unsigned long & allNeeded);

	    /**
	     * Selling the base cycles of the VMs (guarantee of nominal frequency)	     
	     */
	    std::list <monitor::libvirt::control::LibvirtVCPUController*> sellBaseCycles (std::vector <monitor::libvirt::LibvirtVM*> & vms,
											  long & market,
		unsigned long & nbVcpus);
	    
	    /**
	     * Selling the base cycles for the vcpu (guarantee of the nominal frequency)
	     */
	    bool sellBaseCycles (monitor::libvirt::control::LibvirtVCPUController & vcpu,
				 long & market);
	    
	    
	};
	

	
    }

}
