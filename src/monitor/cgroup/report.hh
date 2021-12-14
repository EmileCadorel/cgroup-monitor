#pragma once

#include <monitor/cgroup/seek.hh>
#include <monitor/utils/config.hh>
#include <sstream>

namespace monitor {

    namespace cgroup {

	class Report {

	    std::string _content;
	    
	public:

	    Report (const GroupManager & manager, float time);

	    /**
	     * Convert the report into a string
	     */
	    const std::string & str () const;


	private:

	    /**
	     * Transform the market into a report (in this-> _content ["market"])
	     */
	    void reportMarket (std::stringstream & s, const Market & market);
	    
	    /**
	     * Transform the vms infos into a report (in this-> _content ["vms"])
	     */
	    void reportVMs (std::stringstream & s, const std::map <std::string, VMInfo> & vms);

	    /**
	     * Transform a vminfo into
	     */
	    void vmToDict (std::stringstream & s, const std::string & name, const VMInfo & info) const;

	    
	};	

    }    

}
