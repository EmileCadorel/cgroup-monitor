#include "control.hh"
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <monitor/utils/log.hh>

using namespace monitor;
using namespace monitor::libvirt;
using namespace monitor::concurrency;
using namespace monitor::utils;
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace server {

    Controller::Controller (monitor::libvirt::LibvirtClient & client) :
	_libvirt (client),
	_cpuMarket (client),
	_cpuMarketEnabled (false),
	_memMarket (client),
	_memMarketEnabled (false)
    {
	this-> readCpuMarketConfig ();
	this-> readMemMarketConfig ();

    	fs::create_directories ("/var/log/dio");
	::remove (fs::path ("/var/log/dio/control-log.json").c_str ());
	this-> _logPath = fs::path ("/var/log/dio/control-log.json");
    }

    void Controller::start () {
	this-> _cpuLoopTh = monitor::concurrency::spawn (this, &Controller::cpuControlLoop);
	this-> _memLoopTh = monitor::concurrency::spawn (this, &Controller::memoryControlLoop);
    }

    void Controller::join () {
	monitor::concurrency::join (this-> _cpuLoopTh);
	monitor::concurrency::join (this-> _memLoopTh);
    }

    void Controller::kill () {
	monitor::concurrency::kill (this-> _cpuLoopTh);
	monitor::concurrency::kill (this-> _memLoopTh);
    }

    void Controller::resetMarketCounters () {
	this-> _mutex.lock ();
	if (this-> _cpuMarketEnabled) {
	    this-> _cpuMutex.lock ();
	    this-> _cpuMarket.reset ();
	    this-> _cpuMutex.unlock ();
	}

	if (this-> _memMarketEnabled) {
	    this-> _memMutex.lock ();
	    this-> _memMarket.reset ();
	    this-> _memMutex.unlock ();
	}

	::remove (fs::path ("/var/log/dio/control-log.json").c_str ());
	this-> _mutex.unlock ();
    }
    
    void Controller::cpuControlLoop (monitor::concurrency::thread th) {
	for (;;) {
	    this-> _libvirt.updateCpuControllers ();
	    if (this-> _cpuMarketEnabled) {
		this-> _cpuMutex.lock ();
	    	this-> _cpuMarket.run ();
		this-> _cpuMutex.unlock ();
	    }

	    this-> dumpCpuLogs ();	    
	    this-> waitCpuFrame ();
	}
    }
    
    void Controller::waitCpuFrame () {
	auto s = std::chrono::system_clock::now ();
	auto r = 1.0f - this-> _cpuT.time_since_start ();
	this-> _cpuT.sleep (r);
	auto e = std::chrono::system_clock::now ();
	this-> _cpuT.reset (e, (e - s));
    }    


    void Controller::memoryControlLoop (monitor::concurrency::thread) {
	for (;;) {
	    this-> _libvirt.updateMemoryControllers ();
	    if (this-> _memMarketEnabled) {
		this-> _memMutex.lock ();
	    	this-> _memMarket.run ();
		this-> _memMutex.unlock ();
	    }

	    this-> dumpMemoryLogs ();
	    this-> waitMemoryFrame ();
	}
    }

    void Controller::waitMemoryFrame () {
	auto s = std::chrono::system_clock::now ();
	auto r = 1.0f - this-> _memT.time_since_start ();
	this-> _memT.sleep (r);
	auto e = std::chrono::system_clock::now ();
	this-> _memT.reset (e, (e - s));
    }    
    
    void Controller::readCpuMarketConfig (const fs::path & path) {
	std::ifstream f (path / "cpu-market.json");	
	if (f.good ()) {
	    std::stringstream ss;
	    ss << f.rdbuf ();
	    f.close ();
	    auto j = json::parse (ss.str ());

	    if (j.contains ("enable") && j["enable"].is_boolean ()) {
		this-> _cpuMarketEnabled = j["enable"].get<bool> ();
	    }

	    if (this-> _cpuMarketEnabled) {
		auto marketConfig = market::CpuMarketConfig {
		    j["frequency"].get<int> (),
		    j["trigger-increment"].get<float> () / 100.0f,
		    j["trigger-decrement"].get<float> () / 100.0f,
		    j["increment-speed"].get<float> () / 100.0f,
		    j["decrement-speed"].get<float> () / 100.0f,
		    j["window-size"].get<unsigned long> ()
		};
		this-> _cpuMarket.setConfig (marketConfig);
	    }	    	    
	} else {
	    this-> _cpuMarketEnabled = false;
	}

	if (this-> _cpuMarketEnabled) {
	    logging::info ("CPU Market enabled");
	} else {
	    logging::warn ("CPU Market disabled");
	}
    }

    void Controller::readMemMarketConfig (const fs::path & path) {
	std::ifstream f (path / "mem-market.json");	
	if (f.good ()) {
	    std::stringstream ss;
	    ss << f.rdbuf ();
	    f.close ();
	    auto j = json::parse (ss.str ());

	    if (j.contains ("enable") && j["enable"].is_boolean ()) {
		this-> _memMarketEnabled = j["enable"].get<bool> ();
	    }

	    if (this-> _memMarketEnabled) {
		auto marketConfig = market::MemoryMarketConfig {
		    j["memory"].get<unsigned long> (),
		    j["trigger-increment"].get<float> () / 100.0f,
		    j["trigger-decrement"].get<float> () / 100.0f,
		    j["increment-speed"].get<float> () / 100.0f,
		    j["decrement-speed"].get<float> () / 100.0f,
		    j["window-size"].get<unsigned long> ()
		};
		this-> _memMarket.setConfig (marketConfig);
	    }	    	    
	} else {
	    this-> _memMarketEnabled = false;
	}

	if (this-> _memMarketEnabled) {
	    logging::info ("MEM Market enabled");
	} else {
	    logging::warn ("MEM Market disabled");
	}
    }

    
    void Controller::dumpCpuLogs () {
	json j;
	j["time"] = logging::get_time ();
	j["cpu-duration"] = this-> _cpuT.time_since_start ();
	if (this-> _libvirt.getRunningVMs ().size () != 0) {
	    j["cpu-market"] = this-> _cpuMarket.dumpLogs ();
	    json j2;
	    for (auto & v : this-> _libvirt.getRunningVMs ()) {
		j2 [v.first] = v.second-> getCpuController ().dumpLogs ();
	    }
	    
	    j["cpu-control"] = j2;
	}

	this-> _mutex.lock ();
	std::ofstream f (this-> _logPath, std::ios_base::app);
	f << j.dump () << std::endl;
	f.close ();
	this-> _mutex.unlock ();
    }


    void Controller::dumpMemoryLogs () {
	json j;
	j["time"] = logging::get_time ();
	j["mem-duration"] = this-> _memT.time_since_start ();
	if (this-> _libvirt.getRunningVMs ().size () != 0) {
	    j["memory-market"] = this-> _memMarket.dumpLogs ();
	    json j2;
	    for (auto & v : this-> _libvirt.getRunningVMs ()) {
		j2 [v.first] = v.second-> getMemoryController ().dumpLogs ();
	    }
	    
	    j["mem-control"] = j2;
	}

	this-> _mutex.lock ();
	std::ofstream f (this-> _logPath, std::ios_base::app);
	f << j.dump () << std::endl;
	f.close ();
	this-> _mutex.unlock ();
    }

}
