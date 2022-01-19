#include <iostream>
#include <monitor/cgroup/seek.hh>
#include <monitor/net/listener.hh>
#include <monitor/utils/toml.hh>

using namespace monitor::cgroup;
using namespace monitor::net;
using namespace monitor;

int main (int argc, char ** argv) {
    try {
	if (argc == 1) throw utils::command_line_error ("./usage config.toml");
	
	auto config = utils::toml::parse_file (argv [1]);
	auto inner = config.get <utils::config::dict> ("monitor");
	auto clientAddr = net::SockAddrV4 (inner.get<std::string> ("daemon"));

	auto client = TcpStream (clientAddr);
	client.connect ();
	auto vms = config.get<utils::config::dict> ("vms");
	client.sendInt (vms.keys ().size ());
	for (auto &vm : vms.keys ()) {
	    auto freq = vms.get<int> (vm);
	    client.sendInt (0); /// Adding a VM
	    client.sendInt (vm.length ()); 
	    client.send (vm);
	    client.sendInt (freq);
	}
	
	return 0;
    } catch (utils::exception ex) {
	ex.print ();
    }
}

