#include <monitor/cgroup/seek.hh>

#include <unistd.h>
#include <string>
#include <iostream>
#include <chrono>
#include <monitor/utils/log.hh>
#include <monitor/cgroup/report.hh>

namespace fs = std::filesystem;
using namespace monitor::concurrency;
using namespace monitor::utils;

namespace monitor {
    namespace cgroup {

	GroupManager::GroupManager (utils::config::dict & cfg) : _addr (net::SockAddrV4 (net::Ipv4Address (0, 0, 0, 0), 8080)),
							   _frameDurMicro (1000000),
							   _frameDur (1.0f) 
	{
	    auto inner = cfg.get <utils::config::dict> ("monitor");
	    this-> _addr = net::SockAddrV4 (inner.get<std::string> ("addr"));
	    this-> _frameDur = inner.get <float> ("tick");
	    this-> _vmHistory = inner.get <float> ("slope-history");
	    this-> _frameDurMicro = (long) (this-> _frameDur * 1000000);
	    this-> _format = inner.get <std::string> ("format");

	    if (cfg.has <utils::config::dict> ("market")) {
		auto mark = cfg.get <utils::config::dict> ("market");
		MarketConfig c;
		c.baseCycle = mark.getOr<float> ("base-cycle", 30.0) / 100.0;
		c.triggerIncrement = mark.getOr<float> ("trigger-increment", 95.0) / 100.0;
		c.triggerDecrement = mark.getOr<float> ("trigger-decrement", 50.0) / 100.0;
		c.decreasingSpeed = mark.getOr<float> ("decreasing-speed", 10.0) / 100.0;
		c.increasingSpeed = mark.getOr<float> ("increasing-speed", 10.0) / 100.0;
		c.windowSize = mark.getOr<int> ("window-size", 10000);

		this-> _market = Market (c);
		this-> _withMarket = true;
	    } else this-> _withMarket = inner.getOr <bool> ("marketing", false);
	}

	
	GroupManager::GroupManager (net::SockAddrV4 addr, float frameDur) : _addr (addr), _frameDurMicro ((long) (frameDur * 1000000)), _frameDur (frameDur) {}

	void GroupManager::run () {	    
	    this-> _th = concurrency::spawn (this, &GroupManager::tcpLoop);
	    int nb_turn = 0;
	    while (true) {
		auto s = std::chrono::system_clock::now ();
		if (nb_turn % 60 == 0) {
		    nb_turn = 0;
		    this-> fullupdate ();
		} else {
		    this-> update ();
		}
		nb_turn ++;
		
		if (this-> _withMarket) { // The monitor can run without market to make comparison with no capping easily
		    auto result = this-> _market.update (this-> _vms);
		    this-> applyResult (result);
		}
		auto e = std::chrono::system_clock::now ();
		
		this-> sendReports (e - s);
		this-> waitFrame ();
	    }
	}
       
	const std::map <std::string, VMInfo> & GroupManager::getVms () const {
	    return this-> _vms;
	}

	const std::map <std::string, GroupInfo> & GroupManager::getGroups () const {
	    return this-> _groups;
	}

	const Market & GroupManager::getMarket () const {
	    return this-> _market;
	}
	
	void GroupManager::waitFrame () {
	    auto s = std::chrono::system_clock::now ();
	    auto r = this-> _frameDur - this-> _t.time_since_start ();
	    this-> _t.sleep (r);
	    auto e = std::chrono::system_clock::now ();
	    this-> _t.reset (e, (e - s));
	}

	void GroupManager::fullupdate () {
	    std::string path = "/sys/fs/cgroup/cpu/machine.slice";
	    //this-> _vms.clear ();
	    this-> recursiveUpdate (fs::path (path));
	}
	
	void GroupManager::update () {
	    for (auto v = this-> _vms.begin () ; v != this-> _vms.end () ; ) { 
		if (!v-> second.update ()) {
		    this-> _vms.erase (v ++);
		} else {
		    v ++;
		}
	    }
	}

	void GroupManager::recursiveUpdate (const fs::path & path) {
	    if (fs::is_directory (path)) {
		if (path.u8string ().find("machine-qemu") != std::string::npos) {
		    this-> updateVM (path);
		} else {
		    for (const auto & entry : fs::directory_iterator(path)) {
			if (fs::is_directory (entry.path ())) {
			    this-> recursiveUpdate (fs::path (entry.path ()));
			}
		    }	    
		}
	    }
	}

	void GroupManager::updateVM (const fs::path & path) {
	    auto fnd = this-> _vms.find (path.filename ());
	    if (fnd != this-> _vms.end ()) {
		fnd-> second.update ();
	    } else {
		this-> _vms.emplace (path.filename (), VMInfo {path, this-> _vmHistory});
	    }
	}

	void GroupManager::applyResult (const std::map <std::string, unsigned long> & res) {
	    for (auto & it : res) {
		auto fnd = this-> _vms.find (it.first);
		if (fnd != this-> _vms.end ()) {
		    fnd-> second.applyCapping (it.second);
		}
	    }
	}

	void GroupManager::sendReports (std::chrono::duration<double> diff) {
	    this-> _mutex.lock ();
	    if (this-> _clients.size () != 0) {
		this-> _mutex.unlock ();
		Report report (*this, diff.count ());
		auto & str = report.str ();
	    
		std::vector <net::TcpStream> nclients;
		this-> _mutex.lock ();
		for (auto & it : this-> _clients) {
		    if (it.send (str)) {
			nclients.push_back (it);
		    } else {
			logging::info ("Lost client connection", it.addr ());
		    }
		}
		this-> _clients = nclients;
	    }
	    this-> _mutex.unlock ();
	}

	

	void GroupManager::tcpLoop (concurrency::thread t) {	    
	    logging::info ("Starting server at", this-> _addr);
	    net::TcpListener listener (this-> _addr);
	    listener.start ();
	    while (true) {
		auto client = listener.accept ();
		this-> _mutex.lock ();
		this-> _clients.push_back (client);
		logging::info ("New client connected", client.addr ());
		this-> _mutex.unlock ();		
	    }	    
	}
	
    }
}
