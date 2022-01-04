#include <monitor/utils/log.hh>
#include <iostream>

namespace monitor {
    namespace utils {
	namespace logging {

	    concurrency::mutex __mutex__;
	    
	    void content_print () {}
	    
	    std::string get_time () {		
		time_t now;
		time(&now);
		char buf[sizeof "2011-10-08 07:07:09"];
		strftime(buf, sizeof buf, "%F %T", gmtime(&now));

		return std::string (buf);
	    }

	}
    }
}
