#pragma once

#include "vm.hh"
#include "control.hh"

namespace server {
    
    /**
     * The daemon class is the main class of the monitor
     * It manage the libvirt connections, provisionning etc.
     * And the market, report etc. part
     */
    class Daemon {

	/// The libvirt connection
	monitor::libvirt::LibvirtClient _libvirt;
	
	/// The server provisionning and killing vms
	VMServer _vms;

	/// The controller of the vms
	Controller _controller;
	
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
