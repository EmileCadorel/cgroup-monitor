#include <monitor/libvirt/error.hh>

namespace monitor {

    namespace libvirt {

	LibvirtError::LibvirtError (const std::string & msg) :
	    utils::exception (msg)
	{}
	
    }

}
