#include <monitor/cgroup/report.hh>
#include <monitor/cgroup/market.hh>
#include <sstream>
#include <monitor/utils/json.hh>
#include <monitor/utils/toml.hh>
#include <monitor/utils/log.hh>

namespace monitor {

    namespace cgroup {

	Report::Report (const GroupManager & manager, float time) {
	    std::stringstream s;
	    s << "{ \"time\" : \"" << utils::logging::get_time () << "\", \"duration\" : " << time << ", ";
	    this-> reportMarket (s, manager.getMarket ());
	    s << ", ";
	    this-> reportVMs (s, manager.getVms ());
	    s << "}" << std::endl;
	    this-> _content = s.str ();
	}

	void Report::reportMarket (std::stringstream & s, const Market & market) {
	    s << "\"market\" : ";
	    s << "{ \"accounts\" : { ";
	    int i = 0;
	    for (auto & it : market.getAccounts ()) {
		if (i != 0) s << ", ";
		s << "\"" << it.first << "\" : " << it.second;
		i += 1;
	    }
	    s << "}, "; // accounts
	    s << "\"nb-iteration-f\" : " << market.getFirstNbIterations () << "}";
	}

	
	void Report::reportVMs (std::stringstream & s, const std::map <std::string, VMInfo> & vms) {
	    s << "\"vms\" : {";
	    int i = 0;
	    for (auto & it : vms) {
		if (i != 0) s << ", ";
		s << "\"" << it.first << "\" : ";
		this-> vmToDict (s, it.first, it.second);
		i += 1;
	    }
	    s << "}";
	}

	void Report::vmToDict (std::stringstream & s, const std::string & name, const VMInfo & info) const {
	    s << "{ \"conso_cycle\" : " << info.getAbsoluteConso () << ", ";
	    s << "\"host_usage\" : " << info.getPercentageConso () << ", ";
	    s << "\"relative_usage\" : " << info.getRelativePercentConso () << ", ";
	    s << "\"capping\" : " << info.getCapping () << ", ";
	    s << "\"period\" : " << info.getPeriod () << ", ";
	    s << "\"vcpus\" : " << info.getNbVCpus () << ", ";
	    s << "\"slope\" : " << info.getSlope () << " }";
	}

	const std::string & Report::str () const {
	    return this-> _content;
	}
	
    }
    
}
