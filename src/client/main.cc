#include <iostream>
#include <monitor/foreign/CLI11.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <monitor/utils/log.hh>
#include <monitor/net/_.hh>
#include <monitor/libvirt/_.hh>

using namespace monitor;
using namespace monitor::net;
using namespace monitor::utils;
using namespace monitor::libvirt;
namespace fs = std::filesystem;
using json = nlohmann::json;

void killVM (const std::string & name, const std::filesystem::path & path = "/etc/dio/") {
    std::ifstream f (path / "daemon.json");
    std::stringstream ss;
    ss << f.rdbuf ();
    
    auto port = json::parse (ss.str ());
    TcpStream client (net::SockAddrV4 (net::Ipv4Address (127, 0, 0, 1), port["port"]));
    client.connect ();
    
    client.sendInt (VMProtocol::KILL);
    client.sendInt (name.length ());
    client.send (name);


    auto resp = client.receiveInt ();
    switch (resp) {
    case VMProtocol::OK :
	logging::success ("VM", name, "killed");
	break;
    default:
	auto err = client.receiveInt ();
	logging::error ("VM", name, "does not exists");
	break;
    }
}

void ipVM (const std::string & name, const std::filesystem::path & path = "/etc/dio/") {
     std::ifstream f (path / "daemon.json");
    std::stringstream ss;
    ss << f.rdbuf ();
    
    auto port = json::parse (ss.str ());
    TcpStream client (net::SockAddrV4 (net::Ipv4Address (127, 0, 0, 1), port["port"]));
    client.connect ();
    
    client.sendInt (VMProtocol::IP);
    client.sendInt (name.length ());
    client.send (name);
    
    auto resp = client.receiveInt ();
    switch (resp) {
    case VMProtocol::IP : {
	auto ipLen = client.receiveInt ();
	auto ip = client.receive (ipLen);
	logging::success ("VM running at :", ip);
	break;
    }
    default:
	auto err = client.receiveInt ();
	logging::error ("VM error :", err);
	break;
    }
}

void provisionVM (const std::filesystem::path & cfgPath, const std::filesystem::path & path = "/etc/dio/") {
    std::ifstream f (path / "daemon.json");
    std::stringstream ss;
    ss << f.rdbuf ();

    std::ifstream content (cfgPath);
    if (!content.good ()) {
	logging::error ("VM config file not found");
    }

    std::stringstream vmCfg;
    vmCfg << content.rdbuf ();    
    
    auto port = json::parse (ss.str ());
    TcpStream client (net::SockAddrV4 (net::Ipv4Address (127, 0, 0, 1), port["port"]));
    client.connect ();
    
    client.sendInt (VMProtocol::PROVISION);
    client.sendInt (vmCfg.str ().length ());
    client.send (vmCfg.str ());
    
    auto resp = client.receiveInt ();
    switch (resp) {
    case VMProtocol::IP : {
	auto ipLen = client.receiveInt ();
	auto ip = client.receive (ipLen);
	logging::success ("VM started at :", ip);
	break;
    }
    default:
	auto err = client.receiveInt ();
	logging::error ("VM error :", err);
	break;
    }
}

void natVM (const std::string & name, int host, int guest, const std::filesystem::path & path = "/etc/dio") {
    std::ifstream f (path / "daemon.json");
    std::stringstream ss;
    ss << f.rdbuf ();
    
    auto port = json::parse (ss.str ());
    TcpStream client (net::SockAddrV4 (net::Ipv4Address (127, 0, 0, 1), port["port"]));
    client.connect ();
    
    client.sendInt (VMProtocol::NAT);
    client.sendInt (name.length ());
    client.send (name);
    client.sendInt (host);
    client.sendInt (guest);
    
    auto resp = client.receiveInt ();
    switch (resp) {
    case VMProtocol::OK :
	logging::success ("VM", name, "nat enable from", host, "->", guest);
	break;
    default:
	auto err = client.receiveInt ();
	logging::error ("VM", name, "does not exists");
	break;
    }
}


void resetCounters (const std::filesystem::path & path = "/etc/dio") {
    std::ifstream f (path / "daemon.json");
    std::stringstream ss;
    ss << f.rdbuf ();
    
    auto port = json::parse (ss.str ());
    TcpStream client (net::SockAddrV4 (net::Ipv4Address (127, 0, 0, 1), port["port"]));
    client.connect ();

    client.sendInt (VMProtocol::RESET_COUNTERS);    
    auto resp = client.receiveInt ();
    switch (resp) {
    case VMProtocol::OK :
	logging::success ("Counter are reset");
	break;
    default:
	auto err = client.receiveInt ();
	logging::error ("Failed to reset counters !");
	break;
    }

}


int main (int argc, char ** argv) {
    CLI::App app {"client"};

    std::string kill = "", provision = "", ip = "";
    std::string nat = "";
    int nat_host = 2020, nat_guest = 22;
    bool flg;
    app.add_option ("--kill", kill, "kill the VM (vm name)");
    app.add_option ("--provision", provision, "provision a VM (toml file)");
    app.add_option ("--ip", ip, "get the ip address of the VM (vm name)");
    app.add_option ("--nat", nat, "enable nat for a given VM");
    app.add_option ("--host", nat_host, "nat in port (host port)");
    app.add_option ("--guest", nat_guest, "nat out port (guest port)");
    app.add_flag ("--reset-counters", flg, "reset market counters of the monitor");
	
    try {
	app.parse(argc, argv);

	if (kill != "") {
	    killVM (kill);
	} else if (provision != "") {
	    provisionVM (provision);
	} else if (ip != "") {
	    ipVM (ip);
	} else if (nat != "") {
	    natVM (nat, nat_host, nat_guest);
	} else if (flg) {
	    resetCounters ();
	} else {
	    std::cout << "exit." << std::endl;
	}
    } catch (const CLI::ParseError &e) {
	return app.exit(e);
    }
}

