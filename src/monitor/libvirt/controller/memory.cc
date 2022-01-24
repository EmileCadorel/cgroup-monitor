#include <monitor/libvirt/controller/memory.hh>
#include <libvirt/libvirt.h>
#include <monitor/libvirt/vm.hh>
#include <monitor/utils/log.hh>

using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

	namespace control {

	    LibvirtMemoryController::LibvirtMemoryController (LibvirtVM & context) :
		_context (context),
		_max (context.memory () * 1024)
	    {
	    }

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================          ACQUIRING           =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    void LibvirtMemoryController::enable () {
		// /// Set the period otherwise we cannot read
		if (virDomainSetMemoryStatsPeriod (this-> _context._dom, 1, 0) < 0) {
		    logging::error ("Unable to change balloon collection period");
		    exit (-1);
		}
	    }
	    
	    void LibvirtMemoryController::update () {
		virDomainMemoryStatStruct stats [VIR_DOMAIN_MEMORY_STAT_NR];
		auto nr_stats = virDomainMemoryStats (this-> _context._dom, stats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
		for (int i = 0 ; i < nr_stats ; i++) {
		    if (stats [i].tag == VIR_DOMAIN_MEMORY_STAT_RSS) {
			this-> _used = stats [i].val; 
		    } else if (stats [i].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED) {
			this-> _unused = stats [i].val;
		    }
		}

		auto domainUsed = this-> _max - this-> _unused;
		if (this-> _used < domainUsed) {
		    this-> _swapping = domainUsed - this-> _used;
		} else {
		    this-> _used = domainUsed;
		    this-> _swapping = 0;
		}
		
		logging::info ("VM ", this-> _context.id (), "unused :", this-> _used, "swap :", this-> _swapping, "all :", this-> _swapping + this-> _used);
	    }	    	    
	    

	}

	
    }
    
}
