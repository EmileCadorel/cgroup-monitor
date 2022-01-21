#pragma once

#include <monitor/concurrency/_.hh>
#include <monitor/libvirt/_.hh>

namespace server {

    /**
     * This class is responsible for resource affectation, and analysis
     */
    class Controller {
	
	/// The timer used to compute frame time, and run the controller at the correct pace
	monitor::concurrency::timer _t;
	
	/// The libvirt connection
	monitor::libvirt::LibvirtClient & _libvirt;

	/// The id of the thread managing the control
	monitor::concurrency::thread _loopTh;	
	
    public:

	/**
	 * @params: 
	 *  - the libvirt client that communicate with libvirt
	 */
	Controller (monitor::libvirt::LibvirtClient & libvirt);
	
	/**
	 * Start the thread controller resource affectations
	 */
	void start ();

	/**
	 * Wait for the end of the control loop
	 */
	void join ();	

	/**
	 * Kill the control loop
	 */
	void kill ();

    private :

	/**
	 * Main loop control the resource affectations
	 */
	void controlLoop (monitor::concurrency::thread t);

	/**
	 * Wait for the next frame
	 */
	void waitFrame ();
	
	/**
	 * Dump the log of the controller
	 * @params: 
	 *   - path: the directory in which dump the logs
	 */
	void dumpLog (const std::filesystem::path & path = "/var/log/dio");
    };
    
}