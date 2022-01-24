#pragma once

#include <monitor/concurrency/_.hh>
#include <monitor/libvirt/_.hh>
#include "market/cpu.hh"
#include <nlohmann/json.hpp>

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

	/// The cpu market
	market::CpuMarket _cpuMarket;
	
	/// True iif the cpu market has to be executed
	bool _cpuMarketEnabled;

	/// The path of the log file
	std::filesystem::path _logPath;
	
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
	 * Read the configuration file of the cpu market 
	 * @params: 
	 *   - path: the path of the config directory of the controller
	 */
	void readCpuMarketConfig (const std::filesystem::path & path = "/var/lib/dio");
	
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
	void dumpLogs () const;
    };
    
}
