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
		
		// /// Set the period otherwise we cannot read
		if (virDomainSetMemoryStatsPeriod (this-> _context._dom, 1, 0) < 0) {
		    logging::error ("Unable to change balloon collection period");
		    exit (-1);
		}

		// /// Set the hard limit of the VM memory usage, to be able to reduce it later on
		virTypedParameterPtr params = nullptr;
		int nparams = 0, maxparams = 0;		
		virTypedParamsAddULLong (&params, &nparams, &maxparams, VIR_DOMAIN_MEMORY_HARD_LIMIT, this-> _max);		
		if (virDomainSetMemoryParameters (this-> _context._dom,
						  params,
						  nparams, 0) < 0) {
		    logging::error ("Failed to set memory hard limit of VM :", this-> _context.id (), ":", this-> _max);
		    exit (-1);
		}
		
		virTypedParamsFree (params, nparams);
		
		// Set the swap hard limit of the VM memory usage, to enable swapping when the hard limit will be reduced
		params = nullptr;
		nparams = 0; maxparams = 0;
		virTypedParamsAddULLong (&params, &nparams, &maxparams, VIR_DOMAIN_MEMORY_SWAP_HARD_LIMIT, this-> _max);		
		if (virDomainSetMemoryParameters (this-> _context._dom,
						  params,
						  nparams, 0) < 0) {
		    logging::error ("Failed to set memory swap limit of VM :", this-> _context.id (), ":", this-> _max);
		    exit (-1);
		}
	       
		virTypedParamsFree (params, nparams);		    		
	    }
	    
	    void LibvirtMemoryController::update () {
		this-> _max = this-> _context.memory () * 1024;
		
		virDomainMemoryStatStruct stats [VIR_DOMAIN_MEMORY_STAT_NR];
		auto s = std::chrono::system_clock::now ();
		auto nr_stats = virDomainMemoryStats (this-> _context._dom, stats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
		auto e = std::chrono::system_clock::now ();
		std::chrono::duration<double> since = e - s;
		std::cout << "Reading : " << since.count () << " " << nr_stats << std::endl;
		unsigned long unused;
		for (int i = 0 ; i < nr_stats ; i++) {
		    if (stats [i].tag == VIR_DOMAIN_MEMORY_STAT_RSS) {
			this-> _hostUsed = stats [i].val; 
		    } else if (stats [i].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED) {
			unused = stats [i].val;
		    }
		}
		
		this-> _guestUsed = this-> _max - unused;
		if (this-> _hostUsed < this-> _guestUsed) {
		    this-> _swapping = this-> _guestUsed - this-> _hostUsed;
		    this-> _guestUsed = this-> _hostUsed;
		} else {
		    this-> _swapping = 0;
		}

		logging::info ("Unused :", unused, "host:", this-> _hostUsed, "guest :", this-> _guestUsed, "max :", this-> _max, "swap :", this-> _swapping);
	    }	    	    

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           GETTERS            =========================
	     * ================================================================================
	     * ================================================================================
	     */


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
		this-> _allocated = max;

		virTypedParameterPtr params = nullptr;
		int nparams = 0, maxparams = 0;
		virTypedParamsAddULLong (&params, &nparams, &maxparams, VIR_DOMAIN_MEMORY_HARD_LIMIT, this-> _allocated * 1024);
		
		if (virDomainSetMemoryParameters (this-> _context._dom,
						  params,
						  nparams, 0) != 0) {
		    logging::error ("Failed to set memory hard limit of VM :", this-> _context.id (), ":", this-> _allocated);
		    exit (-1);
		}
		
		virTypedParamsFree (params, nparams);
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
		j["swapping"] = this-> getSwapping ();
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
