#pragma once
#include <monitor/libvirt/_.hh>
#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace server {

    namespace market {

	/**
	 * Market for the memory resource allocations
	 */
	class MemoryMarket {

	    /// The libvirt connection
	    monitor::libvirt::LibvirtClient & _libvirt;


	public :
	    
	    MemoryMarket (monitor::libvirt::LibvirtClient & client);

	    /**
	     * 
	     */
	    void run ();
	    

	};
	
    }

}
