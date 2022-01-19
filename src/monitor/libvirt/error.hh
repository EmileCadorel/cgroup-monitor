#pragma once
#include <monitor/utils/exception.hh>

namespace monitor {

    namespace libvirt {

	class LibvirtError : public utils::exception {

	public:

	    LibvirtError (const std::string & msg);
	    
	};
	
    }
    
}
