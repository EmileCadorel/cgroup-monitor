#include "vm.hh"
#include <monitor/utils/toml.hh>
#include <monitor/utils/config.hh>
#include <fstream>
#include <nlohmann/json.hpp>
#include <monitor/utils/log.hh>
#include <monitor/utils/exception.hh>

using namespace monitor;
using namespace monitor::libvirt;
using namespace monitor::concurrency;
using namespace monitor::utils;
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace server {

    VMServer::VMServer (monitor::libvirt::LibvirtClient & client) :
	_listener (net::SockAddrV4 (net::Ipv4Address (0, 0, 0, 0), 0)),
	_libvirt (client)
    {}
    

    void VMServer::start () {
	this-> _loopTh = monitor::concurrency::spawn (this, &VMServer::acceptingLoop);
    }

    void VMServer::join () {
	monitor::concurrency::join (this-> _loopTh);
	this-> kill ();
    }

    void VMServer::kill () {
	monitor::concurrency::kill (this-> _loopTh);
	this-> _listener.close ();
    }
    
    void VMServer::acceptingLoop (monitor::concurrency::thread th) {
	this-> _listener.start ();
	this-> dumpConfig ();
	logging::info ("Server running on port :", this-> _listener.port ());
	for (;;) {
	    auto client = this-> _listener.accept ();
	    auto prot = client.receiveInt ();
	    logging::info ("New client with protocol :", prot);
	    switch (prot) {
	    case VMProtocol::PROVISION: {
		this-> treatProvision (client);
		break;
	    }
	    case VMProtocol::KILL:  {
		this-> treatKill (client);
		break;
	    }
	    case VMProtocol::IP: {
		this-> treatIp (client);
		break;
	    }
	    case VMProtocol::NAT: {
		this-> treatNat (client);
		break;
	    }
	    default: {
		client.sendInt (VMProtocol::ERR);
		client.sendInt (VMProtocolError::PROTOCOL);
		break;
	    }
	    }
	    client.close ();
	}
    }

    void VMServer::treatProvision (net::TcpStream & stream) {
	auto len = stream.receiveInt ();
	auto file = stream.receive (len);
	auto cfg = utils::toml::parse (file);
	try {
	    auto inner = cfg.get<utils::config::dict> ("vm");
	    auto name = inner.get<std::string> ("name");
	    if (!this-> _libvirt.hasVM (name)) {
		LibvirtVM vm (cfg, this-> _libvirt);
		vm.provision ();
		stream.sendInt (VMProtocol::IP);
		auto ip = vm.ip ();
		stream.sendInt (ip.length ());
		stream.send (ip);
		return;
	    }
	} catch (utils::exception & e) {
	    e.print ();
	}

	stream.sendInt (VMProtocol::ERR);
	stream.sendInt (VMProtocolError::NOT_FOUND);
    }
    
    void VMServer::treatKill (net::TcpStream & stream) {
	auto len = stream.receiveInt ();
	auto name = stream.receive (len);
	try {
	    if (this-> _libvirt.hasVM (name)) {
		auto vm = this-> _libvirt.getVM (name);
		vm.kill ();
		stream.sendInt (VMProtocol::OK);
		return;
	    }
	} catch (...) {
	}

	stream.sendInt (VMProtocol::ERR);
	stream.sendInt (VMProtocolError::NOT_FOUND);
    }

    void VMServer::treatIp (net::TcpStream & stream) {
	auto len = stream.receiveInt ();
	auto name = stream.receive (len);
	try {
	    if (this-> _libvirt.hasVM (name)) {
		auto vm = this-> _libvirt.getVM (name);
		stream.sendInt (VMProtocol::IP);
		auto ip = vm.ip ();
		stream.sendInt (ip.length ());
		stream.send (ip);
		return;
	    }
	} catch (...) {
	}

	stream.sendInt (VMProtocol::ERR);
	stream.sendInt (VMProtocolError::NOT_FOUND);	
    }

    void VMServer::treatNat (net::TcpStream & stream) {
	auto len = stream.receiveInt ();
	auto name = stream.receive (len);
	try {
	    if (this-> _libvirt.hasVM (name)) {
		auto vm = this-> _libvirt.getVM (name);
		auto host = stream.receiveInt ();
		auto guest = stream.receiveInt ();

		this-> _libvirt.openNat (vm, host, guest);
		stream.sendInt (VMProtocol::OK);
		return;
	    }
	} catch (...) {
	}

	stream.sendInt (VMProtocol::ERR);
	stream.sendInt (VMProtocolError::NOT_FOUND);	
    }


    void VMServer::dumpConfig (const std::filesystem::path & path) const {
	json j;
	j ["port"] = this-> _listener.port ();

	fs::create_directories (path);
	std::ofstream f (path / "daemon.json");
	f << j.dump ();
	f.close ();
    }


}