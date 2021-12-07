#pragma once

#include <monitor/utils/config.hh>


namespace monitor {

    namespace utils {

	namespace toml {

	    std::string dump (const config::dict & cfg, bool isSuper = true, bool isGlobal = true);
	    
	    /*
	     * Parse a string, and return a configuration
	     */       
	    config::dict parse (const std::string & content);

	    config::dict parse_file (const std::string & path);

	}
    }
    
}
