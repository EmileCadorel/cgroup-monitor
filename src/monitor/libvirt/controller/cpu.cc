#include <monitor/libvirt/controller/cpu.hh>
#include <monitor/libvirt/vm.hh>
#include <libvirt/libvirt.h>
#include <monitor/utils/log.hh>

using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

	namespace control {

	    LibvirtCpuController::LibvirtCpuController (LibvirtVM & context, int maxHistory) :
		_context (context),
		_maxHistory (maxHistory)
	    {}


	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================          ACQUIRING           =========================
	     * ================================================================================
	     * ================================================================================
	     */


	    void LibvirtCpuController::update () {		
		int stats = VIR_DOMAIN_STATS_CPU_TOTAL;
		
		virDomainStatsRecordPtr *records = NULL;
		virDomainStatsRecordPtr *next;

		virDomainPtr doms[] = {this-> _context._dom, nullptr};
		int i = virDomainListGetStats (doms, stats, &records, 0);
		if (i > 0) {
		    next = records;
		    while (*next) {
			auto v = *next;
			for (int i = 0; i < v-> nparams; i++) {
			    if (std::string (v-> params[i].field) == "cpu.time") {
				this-> addToHistory (v-> params[i].value.ul / 1000); // the time is written in nanosecond
				logging::info ("VM ", this->_context.id (), "slope :", this-> _slope, this-> _consumption);
			    }
			}
			
			next++;
		    }
		}

		virDomainStatsRecordListFree (records);
	    }
	    	


	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           PRIVATE            =========================
	     * ================================================================================
	     * ================================================================================
	     */
	
	    void LibvirtCpuController::addToHistory (unsigned long val) {
		this-> _history.push_back (val);
		if (this-> _history.size () > this-> _maxHistory) {
		    this-> _history.erase (this-> _history.begin ());
		}
	    
		if (this-> _history.size () == this-> _maxHistory) this-> computeSlope ();
		if (this-> _history.size () >= 2) {
		    this-> _consumption = this-> _history[this-> _history.size () - 1] - this-> _history [this-> _history.size () - 2];
		}
	    }

	    void LibvirtCpuController::computeSlope () {
		double max = (unsigned long) this-> _context.vcpus () * (unsigned long) 1000000;
		double sum_x = 0;
		double sum_y = 0;

		for (int x = 1 ; x < this-> _history.size () ; x++) {
		    auto perc = ((double) this-> _history [x] - this-> _history [x - 1]) / max;
		    sum_y += perc;
		    sum_x += x;
		}
		
		auto m_x = sum_x / (double) (this-> _history.size ());
		auto m_y = sum_y / (double) (this-> _history.size ());

		double ss_x = 0.0, sp = 0.0;
		for (int x = 1 ; x < this-> _history.size (); x++) {
		    auto perc = ((double) this-> _history [x] - this-> _history [x - 1]) / max ;
		    ss_x += (x - m_x) * (x - m_x);
		    sp += (x - m_x) * (perc - m_y);
		}

		this-> _slope = sp / ss_x;
	    }

	    
	}

    }
}
