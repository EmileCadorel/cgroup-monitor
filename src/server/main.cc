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
	GroupManager manager (config);
	manager.run ();
	
	return 0;
    } catch (utils::exception ex) {
	ex.print ();
    }
}
