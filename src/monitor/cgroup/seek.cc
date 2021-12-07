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
	    this-> _frameDurMicro = (long) (this-> _frameDur * 1000000);
	    this-> _format = inner.get <std::string> ("format");

	    if (cfg.has <utils::config::dict> ("market")) {
		auto mark = cfg.get <utils::config::dict> ("market");
		MarketConfig c;
		c.marketIncrement = mark.getOr <float> ("market-increment", 1.0);
		c.cyclePrice = mark.getOr<float> ("cycle-price", 1.0);
		c.cycleIncrement = mark.getOr<float> ("cycle-increment", 0.1);
		c.baseCycle = mark.getOr<float> ("base-cycle", 0.01);
		c.windowScale = mark.getOr<float> ("window-scale", 0.1);
		c.moneyIncrement = mark.getOr<float> ("money-increment", 0.3);
		c.triggerIncrement = mark.getOr<float> ("trigger-increment", 85.0);
		c.triggerDecrement = mark.getOr<float> ("trigger-decrement", 25.0);
		c.windowSize = mark.getOr<int> ("window-size", 10000);
		c.windowMultiplier = mark.getOr<int> ("window-multiplier", 5);
		c.windowMaximumTurn = mark.getOr<int> ("max-window-turn", 10);

		this-> _market = Market (c);
		this-> _withMarket = true;
	    } else this-> _withMarket = inner.getOr <bool> ("marketing", false);
	}

	
	GroupManager::GroupManager (net::SockAddrV4 addr, float frameDur) : _addr (addr), _frameDurMicro ((long) (frameDur * 1000000)), _frameDur (frameDur) {}

	void GroupManager::run () {	    
	    this-> _th = concurrency::spawn (this, &GroupManager::tcpLoop);
	    while (true) {
		this-> update ();
		auto s = std::chrono::system_clock::now ();
		if (this-> _withMarket) {
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
	    auto r = this-> _frameDur - this-> _t.time_since_start ();
	    auto s = std::chrono::system_clock::now ();
	    this-> _t.sleep (r);
	    auto e = std::chrono::system_clock::now ();
	    this-> _t.reset ((e - s));
	}
	
	void GroupManager::update () {
	    std::string path = "/sys/fs/cgroup/cpu";
	    std::map <std::string, GroupInfo> result;
	    std::map <std::string, VMInfo> vmResult;
	    
	    this-> recursiveUpdate (fs::path (path), result, vmResult);
	    this-> _groups = result;
	    this-> _vms = vmResult;
	}

	void GroupManager::recursiveUpdate (const fs::path & path, std::map <std::string, GroupInfo> & result, std::map <std::string, VMInfo> & vmRes) {
	    if (path.u8string ().find("machine-qemu") != std::string::npos) {
		auto info = this-> updateVM (path);
		vmRes.emplace (path.filename (), info);
	    } else {	    
		if (fs::exists (path / "cpu.cfs_quota_us")) {
		    auto it = this-> _groups.find (path.filename ());
		    if (it != this-> _groups.end ()) {
			it-> second.update (this-> _frameDur);
			result.emplace (path.filename (), it-> second);
		    } else {
			result.emplace  (path.filename (), GroupInfo (path));
		    }
		}
	    
		for (const auto & entry : fs::directory_iterator(path)) {
		    if (fs::is_directory (entry.path ())) {
			this-> recursiveUpdate (fs::path (entry.path ()), result, vmRes);
		    }
		}
	    }
	}

	VMInfo GroupManager::updateVM (const fs::path & path) {
	    auto it = this-> _vms.find (path.filename ());
	    if (it != this-> _vms.end ()) {
		it-> second.update ();
		return it-> second;
	    } else {
		return VMInfo {path};
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
		auto str = report.str (this-> _format) + "\n";
	    
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
