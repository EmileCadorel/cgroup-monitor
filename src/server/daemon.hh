#pragma once

#include "vm.hh"

namespace server {
    
    /**
     * The daemon class is the main class of the monitor
     * It manage the libvirt connections, provisionning etc.
     * And the market, report etc. part
     */
    class Daemon {

	VMServer _vms;	
	
	
    public: 
	
	Daemon ();

	/**
	 * Start the different part of the daemon
	 */
	void start ();

	/**
	 * Wait for the end of the parts of the dameon 
	 * @info: this function will never exit
	 */
	void join ();

	/**
	 * Force the killing of the daemon
	 */
	void kill ();
	
    };

}
