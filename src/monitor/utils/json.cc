#include <monitor/utils/json.hh>
#include <monitor/utils/config.hh>
#include <monitor/utils/range.hh>

namespace monitor {

    namespace utils {

	namespace json {
	   
	    std::string dump (const config::array & cfg) {
		std::stringstream ss;
		ss << "[";
		auto & type = cfg.types ();
		for (auto it : range (0, cfg.size ())) {
		    if (it != 0) ss << ", ";
		    if (type [it] == typeid (float).name ()) ss << cfg.get<float> (it);
		    if (type [it] == typeid (long).name ()) ss << cfg.get<long> (it);
		    if (type [it] == typeid (std::string).name ()) ss << "\"" << cfg.get<std::string> (it) << "\"";
		    if (type [it] == typeid (config::array).name ()) ss << dump (cfg.get<config::array> (it));
		    if (type [it] == typeid (config::dict).name ()) ss << dump (cfg.get<config::dict> (it));		   
		}
		ss << "]";
		return ss.str ();
	    }
	    
	    
	    std::string dump (const config::dict & cfg) {
		std::stringstream ss;
		ss << "{";
		auto type = cfg.types ();
		int i = 0;
		for (auto & it : cfg.keys ()) {
		    if (i != 0) ss << ", ";
		    if (type [it] == typeid (float).name ()) ss << "\"" << it << "\" : " << cfg.get<float> (it);
		    if (type [it] == typeid (long).name ()) ss << "\"" << it << "\" : " << cfg.get<long> (it);
		    if (type [it] == typeid (std::string).name ()) ss << "\"" << it << "\" : \"" << cfg.get<std::string> (it) << "\"";
		    if (type [it] == typeid (config::array).name ()) ss << "\"" << it << "\" : " << dump (cfg.get<config::array> (it));
		    if (type [it] == typeid (config::dict).name ()) ss << "\"" << it << "\" : " << dump (cfg.get<config::dict> (it));
		    
		    i += 1;
		}
		ss << "}";
		return ss.str ();
	    }	    
	    
	}
	
    }

}
