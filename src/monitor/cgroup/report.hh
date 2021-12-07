#pragma once

#include <monitor/cgroup/seek.hh>
#include <monitor/utils/config.hh>

namespace monitor {

    namespace cgroup {

	class Report {

	    utils::config::dict _content;
	    
	public:

	    Report (const GroupManager & manager, float time);

	    /**
	     * Convert the report into a string
	     * @params:
	     * - format: the format of the report (json, toml)
	     */
	    std::string str (const std::string & format) const;


	private:

	    /**
	     * Transform the market into a report (in this-> _content ["market"])
	     */
	    void reportMarket (const Market & market);
	    
	    /**
	     * Transform the vms infos into a report (in this-> _content ["vms"])
	     */
	    void reportVMs (const std::map <std::string, VMInfo> & vms);

	    /**
	     * Transform a vminfo into a dict
	     */
	    utils::config::dict vmToDict (const std::string & name, const VMInfo & info) const;

	    
	};	

    }    

}
