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
	_cpuMarketEnabled (false)
    {
	std::ifstream f ("/var/lib/dio/cpu-market.json");	
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

    	fs::create_directories ("/var/log/dio");
	this-> _logPath = fs::path ("/var/log/dio/") / std::string (("control-log-" + logging::get_time_no_space () + ".json"));
    }

    void Controller::start () {
	this-> _loopTh = monitor::concurrency::spawn (this, &Controller::controlLoop);
    }

    void Controller::join () {
	monitor::concurrency::join (this-> _loopTh);
    }

    void Controller::kill () {
	monitor::concurrency::kill (this-> _loopTh);
    }

    void Controller::controlLoop (monitor::concurrency::thread th) {
	for (;;) {
	    this-> _libvirt.updateControllers ();
	    if (this-> _cpuMarketEnabled) {
		this-> _cpuMarket.run ();
	    }

	    this-> dumpLogs ();	    
	    this-> waitFrame ();
	}
    }

    void Controller::waitFrame () {
	auto s = std::chrono::system_clock::now ();
	auto r = 1.0f - this-> _t.time_since_start ();
	this-> _t.sleep (r);
	auto e = std::chrono::system_clock::now ();
	this-> _t.reset (e, (e - s));
    }    

    void Controller::dumpLogs () const {
	json j;
	j["time"] = logging::get_time ();
	j["duration"] = this-> _t.time_since_start ();
	if (this-> _libvirt.getRunningVMs ().size () != 0) {
	    j["cpu-market"] = this-> _cpuMarket.dumpLogs ();
	    json j2;
	    for (auto & v : this-> _libvirt.getRunningVMs ()) {
		j2 [v.first] = v.second.getCpuController ().dumpLogs ();
	    }
	    
	    j["cpu-control"] = j2;
	}
	
	std::ofstream f (this-> _logPath, std::ios_base::app);
	f << j.dump () << std::endl;
	f.close ();
    }
    
}
