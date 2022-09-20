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
	_vcpuMarketEnabled (false),
	_vcpuMarket (client)
    {
	//this-> readCpuMarketConfig ();
	
    	fs::create_directories ("/var/log/dio");
	::remove (fs::path ("/var/log/dio/control-log.json").c_str ());
	this-> _logPath = fs::path ("/var/log/dio/control-log.json");
    }

    void Controller::start () {
	this-> _cpuLoopTh = monitor::concurrency::spawn (this, &Controller::cpuControlLoop);
    }

    void Controller::join () {
	monitor::concurrency::join (this-> _cpuLoopTh);
    }

    void Controller::kill () {
	monitor::concurrency::kill (this-> _cpuLoopTh);
    }

    void Controller::resetMarketCounters () {
	this-> _mutex.lock ();
	if (this-> _vcpuMarketEnabled) {
	    this-> _vcpuMutex.lock ();
	    this-> _vcpuMarket.reset ();
	    this-> _vcpuMutex.unlock ();
	}

	::remove (fs::path ("/var/log/dio/control-log.json").c_str ());
	this-> _mutex.unlock ();
    }
    
    void Controller::cpuControlLoop (monitor::concurrency::thread th) {
	int i = 0; 
	for (;;) {
	    this-> _libvirt.updateVCPUControllers ();
	    if (i == 1) {
		this-> _libvirt.updateVCPUBeforeMarket ();
		if (this-> _vcpuMarketEnabled) {
		    this-> _vcpuMutex.lock ();
		    this-> _vcpuMarket.run ();
		    this-> _vcpuMutex.unlock ();
		}
		
		this-> dumpCpuLogs ();
		i = 0;
	    }	    
	    this-> waitCpuFrame ();
	    i += 1;
	}
    }
    
    void Controller::waitCpuFrame () {
	auto s = std::chrono::system_clock::now ();
	auto r = 1.f - this-> _cpuT.time_since_start ();
	if (r > 0.f) {
	    this-> _cpuT.sleep (r);
	}
	auto e = std::chrono::system_clock::now ();
	this-> _cpuT.reset (e, (e - s));
    }    

    
    void Controller::readCpuMarketConfig (const fs::path & path) {
	std::ifstream f (path / "cpu-market.json");	
	if (f.good ()) {
	    std::stringstream ss;
	    ss << f.rdbuf ();
	    f.close ();
	    auto j = json::parse (ss.str ());

	    if (j.contains ("enable") && j["enable"].is_boolean ()) {
		this-> _vcpuMarketEnabled = j["enable"].get<bool> ();
	    }

	    if (this-> _vcpuMarketEnabled) {
		auto marketConfig = market::VCPUMarketConfig {
		    j["frequency"].get<int> (),
		    j["trigger-increment"].get<float> () / 100.0f,
		    j["trigger-decrement"].get<float> () / 100.0f,
		    j["increment-speed"].get<float> () / 100.0f,
		    j["decrement-speed"].get<float> () / 100.0f,
		    j["window-size"].get<unsigned long> ()
		};
		this-> _vcpuMarket.setConfig (marketConfig);
	    }	    	    
	} else {
	    this-> _vcpuMarketEnabled = false;
	}

	if (this-> _vcpuMarketEnabled) {
	    logging::info ("CPU Market enabled");
	} else {
	    logging::warn ("CPU Market disabled");
	}
    }


    void Controller::dumpCpuLogs () {
	json j;
	j["time"] = logging::get_time ();
	j["cpu-duration"] = this-> _cpuT.time_since_start ();
	if (this-> _rapl.isEnabled ()) {
	    j["rapl0"] = this-> _rapl.readPP0 ();
	    j["rapl1"] = this-> _rapl.readPP1 ();
	}
	
	json j2, money, freq;
	int i = 0;	    
	for (auto j : this-> _libvirt.getLastCPUFrequency ()) {
	    freq[i] = j;
	    i += 1;
	}

	if (this-> _libvirt.getRunningVMs ().size () != 0) {
	    for (auto & v : this-> _libvirt.getRunningVMs ()) {
		i = 0;
		json all;
		for (auto & vt : v-> getVCPUControllers ()) {
		    all [i] = vt.dumpLogs ();
		    i += 1;
		}
		j2 [v-> id ()] = all;
		money[v-> id()] = v-> money ();
	    }
	    j["cpu-control"] = j2;
	    j["accounts"] = money;
	}
	
	j["freq"] = freq;	

	this-> _mutex.lock ();
	std::ofstream f (this-> _logPath, std::ios_base::app);
	f << j.dump () << std::endl;
	f.close ();
	this-> _mutex.unlock ();
    }
    

}
