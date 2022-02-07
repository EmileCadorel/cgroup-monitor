#pragma once

#include <monitor/concurrency/_.hh>
#include <monitor/libvirt/_.hh>
#include "market/cpu.hh"
#include "market/memory.hh"
#include <nlohmann/json.hpp>

namespace server {

    /**
     * This class is responsible for resource affectation, and analysis
     */
    class Controller {
	
	/// The timer used to compute cpu frame time, and run the controller at the correct pace
	monitor::concurrency::timer _cpuT;

	/// The timer used to compute memory frame time, and run the controller at the correct pace
	monitor::concurrency::timer _memT;	
	
	/// The libvirt connection
	monitor::libvirt::LibvirtClient & _libvirt;

	/// The id of the thread managing the control of cpu
	monitor::concurrency::thread _cpuLoopTh;

	/// The id of the thread managing the control of memory
	monitor::concurrency::thread _memLoopTh;	

	/// The cpu market
	market::CpuMarket _cpuMarket;
	
	/// True iif the cpu market has to be executed
	bool _cpuMarketEnabled;

	/// The memory market
	market::MemoryMarket _memMarket;

	/// True iif the mem market has to be executed
	bool _memMarketEnabled;	

	/// The path of the log file
	std::filesystem::path _logPath;

	/// The mutex used to synchronize the different control loops
	monitor::concurrency::mutex _mutex;

	/// The mutex used to synchronize cpu market access
	monitor::concurrency::mutex _cpuMutex;

	/// The mutex used to synchronize mem market access
	monitor::concurrency::mutex _memMutex;
	
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
	 * Reset the market counters
	 */
	void resetMarketCounters () ;

	
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
	void readCpuMarketConfig (const std::filesystem::path & path = "/usr/lib/dio");

	/**
	 * Read the configuration file of the mem market 
	 * @params: 
	 *   - path: the path of the config directory of the controller
	 */
	void readMemMarketConfig (const std::filesystem::path & path = "/usr/lib/dio");
	
	/**
	 * Main loop control the resource affectations
	 */
	void cpuControlLoop (monitor::concurrency::thread t);

	/**
	 * Wait for the next frame
	 */
	void waitCpuFrame ();

	/**
	 * Main loop control the resource affectations
	 */
	void memoryControlLoop (monitor::concurrency::thread t);

	/**
	 * Wait for the next frame
	 */
	void waitMemoryFrame ();
	
	/**
	 * Dump the log of the cpu controller
	 */
	void dumpCpuLogs () ;

	/**
	 * Dump the log of the memory controller
	 */
	void dumpMemoryLogs () ;
    };
    
}
