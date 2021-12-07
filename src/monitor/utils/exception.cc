#include <monitor/utils/exception.hh>


#ifdef linux
#include <cxxabi.h>
#endif

namespace monitor {

    namespace utils {

	exception::exception (const std::string & msg) :
	    msg (msg)
	{}

	void exception::print () const {
	    std::cerr << this-> msg << std::endl;
	}
		    	
    }
    
}

std::ostream & operator << (std::ostream & ss, const monitor::utils::exception & ex) {
    ex.print ();
    return ss;
}
