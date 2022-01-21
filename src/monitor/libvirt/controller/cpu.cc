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
		if (this-> _context._dom != nullptr) {
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
	    }


	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           GETTERS            =========================
	     * ================================================================================
	     * ================================================================================
	     */


	    unsigned long LibvirtCpuController::getConsumption () const {
		return this-> _consumption;
	    }

	    int LibvirtCpuController::getPeriod () const {
		return this-> _period;
	    }

	    float LibvirtCpuController::getSlope () const {
		return this-> _slope;
	    }

	    int LibvirtCpuController::getQuota () const {
		return this-> _quota;
	    }


	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================         CONTROLLING          =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    
	    void LibvirtCpuController::setQuota (int nbMicros, int period) {
		this-> _period = period;
		this-> _quota = nbMicros;

		if (this-> _context._dom != nullptr) {
		    int npar = 0, maxpar = 0;
		    virTypedParameterPtr par = nullptr;
		    if (virTypedParamsAddLLong (&par, &npar, &maxpar,
						"global_quota",
						this-> _quota) != 0) {
			logging::error ("Failed to create parameters for quota :", this-> _context.id ());
		    }

		    if (virTypedParamsAddULLong (&par, &npar, &maxpar,
						 "global_period",
						 this-> _period) != 0) {
			logging::error ("Failed to create parameters for period :", this-> _context.id ());
		    }

		    if (virDomainSetSchedulerParameters (this-> _context._dom,
							 par,
							 npar) != 0) {
			logging::error ("Failed to set sched parameters :", this-> _context.id ());	       
		    }		
		
		    virTypedParamsFree (par, npar);
		}
	    }

	    void LibvirtCpuController::unlimit () {
		this-> setQuota (-1);
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
