#include <monitor/libvirt/controller/memory.hh>
#include <libvirt/libvirt.h>
#include <monitor/libvirt/vm.hh>
#include <monitor/utils/log.hh>

using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

	namespace control {

	    LibvirtMemoryController::LibvirtMemoryController (LibvirtVM & context, int maxHistory) :
		_context (context),
		_max (0),
		_maxHistory (maxHistory),
		_allocated (0)
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
		/// The memory of the VM might have been changed during its boot process
		this-> _max = this-> _context.memory () * 1024;
		this-> _allocated = this-> _max;
		this-> _minGuarantee = std::max ((unsigned long) (this-> _context.memorySLA () * ((double) this-> _max)), (unsigned long) 1048576);
		
		// Set the period otherwise we cannot read
		if (virDomainSetMemoryStatsPeriod (this-> _context._dom, 1, 0) < 0) {
		    logging::error ("Unable to change balloon collection period");
		    exit (-1);
		}
		
		this-> _max -= 128; // we remove 128 because max must be lower than swap max, otherwise the VM can crash !
		this-> _allocated = this-> _max;
	    }
	    
	    void LibvirtMemoryController::update () {		
		virDomainMemoryStatStruct stats [VIR_DOMAIN_MEMORY_STAT_NR];
		auto nr_stats = virDomainMemoryStats (this-> _context._dom, stats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
		
		unsigned long unused, usable;
		for (int i = 0 ; i < nr_stats ; i++) {
		    if (stats [i].tag == VIR_DOMAIN_MEMORY_STAT_RSS) {
			this-> _hostUsed = stats [i].val; 
		    } else if (stats [i].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED) {
			unused = stats [i].val;
		    } else if (stats [i].tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE) {
			usable = stats [i].val;
		    }
		}
		
		this-> _guestUsed = usable - unused;
		if (this-> _hostUsed < this-> _guestUsed) {
		    this-> _swapping = this-> _guestUsed - this-> _hostUsed;
		} else {
		    this-> _swapping = 0;
		}

		this-> addToHistory ();
	    }	    	    

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           GETTERS            =========================
	     * ================================================================================
	     * ================================================================================
	     */


	    unsigned long LibvirtMemoryController::getMinGuarantee () const {
		return this-> _minGuarantee;
	    }
	    
	    unsigned long LibvirtMemoryController::getMaxMemory () const {
		return this-> _max;
	    }

	    
	    unsigned long LibvirtMemoryController::getGuestUsed () const {
		return this-> _guestUsed;
	    }

	    unsigned long LibvirtMemoryController::getHostUsed () const {
		return this-> _hostUsed;
	    }
		

	    unsigned long LibvirtMemoryController::getSwapping () const {
		return this-> _swapping;
	    }

	    unsigned long LibvirtMemoryController::getAllocated () const {
		return this-> _allocated;
	    }
		
	    
	    float LibvirtMemoryController::getAbsolutePercentUsed () const {
		return ((float) (this-> _guestUsed) / (float) this-> _max) * 100.0f;
	    }

	    float LibvirtMemoryController::getRelativePercentUsed () const {
		return ((float) (this-> _guestUsed) / (float) this-> _allocated) * 100.0f;
	    }


	    float LibvirtMemoryController::getSlope () const {
		return this-> _slope;
	    }
	    

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================         CONTROLLING          =========================
	     * ================================================================================
	     * ================================================================================
	     */
	   
	    void LibvirtMemoryController::setAllocatedMemory (unsigned long max) {
		if (this-> _allocated / 1024 / 102 != max / 1024 / 102) {
		    auto alloc = (max / 1024 / 102) * (102 * 1024);
		    this-> _allocated = std::min (alloc, (unsigned long) (this-> _max));
		    logging::info ("VM capping", this-> _context.id (), ":", this-> _allocated, "/", this-> _max);
		    virDomainSetMemoryFlags (this-> _context._dom, this-> _allocated, VIR_DOMAIN_AFFECT_LIVE);
		}
	    }


	    void LibvirtMemoryController::unlimit () {
		this-> setAllocatedMemory (this-> _max);
	    }
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================             LOG              =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    nlohmann::json LibvirtMemoryController::dumpLogs () const {
		nlohmann::json j;
		j["host-usage"] = this-> getHostUsed ();
		j["guest-usage"] = this-> getGuestUsed ();
		j["allocated"] = this-> getAllocated ();
		j["slope"] = this-> getSlope ();
		
		return j;
	    }
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           HISTORY            =========================
	     * ================================================================================
	     * ================================================================================
	     */


	    void LibvirtMemoryController::addToHistory () {
		this-> _history.push_back (this-> getAbsolutePercentUsed ());
		if (this-> _history.size () > this-> _maxHistory) {
		    this-> _history.erase (this-> _history.begin ());
		}		

		if (this-> _history.size () == this-> _maxHistory) this-> computeSlope ();
	    }


	    void LibvirtMemoryController::computeSlope () {
		double sum_x = 0;
		double sum_y = 0;

		for (int x = 0; x < this-> _history.size () ; x++) {
		    sum_y += this-> _history [x];
		    sum_x += x;
		}
		
		auto m_x = sum_x / (double) (this-> _history.size ());
		auto m_y = sum_y / (double) (this-> _history.size ());

		double ss_x = 0.0, sp = 0.0;
		for (int x = 0 ; x < this-> _history.size (); x++) {
		    ss_x += (x - m_x) * (x - m_x);
		    sp += (x - m_x) * (this-> _history [x] - m_y);
		}

		this-> _slope = sp / ss_x;
	    }
	    

	}

	
    }
    
}
