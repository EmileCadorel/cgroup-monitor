#include "control.hh"
#include <unistd.h>
#include <string>
#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>
#include <monitor/utils/log.hh>

using namespace monitor;
using namespace monitor::libvirt;
using namespace monitor::concurrency;
using namespace monitor::utils;
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace server {

    Controller::Controller (monitor::libvirt::LibvirtClient & client) :
	_libvirt (client)
    {}

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
	    logging::info ("Updating...");
	    this-> _libvirt.updateControllers ();	    
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
    
    
    
}
