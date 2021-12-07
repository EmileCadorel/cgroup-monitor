#include <monitor/cgroup/report.hh>
#include <monitor/market.hh>
#include <sstream>
#include <monitor/utils/json.hh>
#include <monitor/utils/toml.hh>
#include <monitor/utils/log.hh>

namespace monitor {

    namespace cgroup {

	Report::Report (const GroupManager & manager, float time) {

	    this-> reportVMs (manager.getVms ());
	    this-> reportMarket (manager.getMarket ());
	    this-> _content.insert ("time", new std::string (utils::logging::get_time ()));
	    this-> _content.insert ("duration", new float (time));
	}

	void Report::reportMarket (const Market & market) {
	    utils::config::dict accounts, res;
	    for (auto & it : market.getAccounts ()) {
		accounts.insert (it.first, new long (it.second));
	    }
	    
	    res.insert ("accounts", new utils::config::dict (accounts));
	    res.insert ("nb-iteration-f", new long (market.getFirstNbIterations ()));
	    res.insert ("nb-iteration-s", new long (market.getSecondNbIterations ()));
	    res.insert ("nb-sold-f", new long (market.getFirstMarketSold ()));
	    res.insert ("nb-lost", new long (market.getLost ()));
	    
	    this-> _content.insert ("market", new utils::config::dict (res));
	}

	
	void Report::reportVMs (const std::map <std::string, VMInfo> & vms) {
	    utils::config::dict res;
	    for (auto & it : vms) {
		res.insert (it.first, new utils::config::dict (this-> vmToDict (it.first, it.second)));
	    }

	    this-> _content.insert ("vms", new utils::config::dict (res));
	}

	utils::config::dict Report::vmToDict (const std::string & name, const VMInfo & info) const {
	    utils::config::dict res;
	    res.insert ("name", new std::string (name));
	    res.insert ("conso_cycle", new long (info.getAbsoluteConso ()));
	    res.insert ("host_usage", new long (info.getPercentageConso ()));
	    res.insert ("relative_usage", new long (info.getRelativePercentConso ()));
	    res.insert ("capping", new long (info.getCapping ()));
	    res.insert ("period", new long (info.getPeriod ()));
	    res.insert ("vcpus", new long (info.getVCpus ().size ()));

	    return res;
	}

	std::string Report::str (const std::string & format) const {
	    if (format == "json") {
		return utils::json::dump (this-> _content);
	    } else {
		return utils::toml::dump (this-> _content);
	    }
	}
	
    }
    
}
