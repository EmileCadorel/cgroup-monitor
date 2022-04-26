#include <monitor/libvirt/controller/vcpu.hh>
#include <monitor/libvirt/vm.hh>
#include <libvirt/libvirt.h>
#include <monitor/utils/log.hh>
#include <fstream>

namespace fs = std::filesystem;
using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

	namespace control {

	    LibvirtVCPUController::LibvirtVCPUController (int id, LibvirtVM & context, int maxHistory) :
		_id (id),
		_context (context),
		_maxHistory (maxHistory),
		_cgroup (context.id ()),
		_sumFrequency (0),
		_consumption (0),
		_sumConsumption (0),
		_sumDelta (0.0f),
		_delta (0.0f),
		_nbMicros (0),
		_nominalFreq (context.freq ())
	    {}	    

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================          ACQUIRING           =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    void LibvirtVCPUController::enable () {
		this-> _cgroup.enable (this-> _id);
	    }

	    void LibvirtVCPUController::update (const std::vector<unsigned int> & freq) {			
		this-> _lastMicroConsumption = this-> _microConsumption;
		unsigned int p = this-> _cgroup.readCpu ();
		this-> _microConsumption = this-> _cgroup.readUsage ();
		
		this-> _microDelta = this-> _t.time_since_start ();
		auto deltaUsed = (float (this-> _microConsumption - this-> _lastMicroConsumption) / 1000000.0f);		
		this-> _t.reset ();

		this-> _sumFrequency += (unsigned long) (float (freq[p]) * (deltaUsed / this-> _microDelta));
		this-> _sumConsumption += (this-> _microConsumption - this-> _lastMicroConsumption);
		this-> _sumDelta += this-> _microDelta;
		this-> _nbMicros += 1;
	    }

	    void LibvirtVCPUController::updateBeforeMarket () {
		this-> _lastFrequency = this-> _sumFrequency / this-> _nbMicros;
		this-> _consumption = this-> _sumConsumption;
		this-> _delta = this-> _sumDelta;
		
		this-> _sumConsumption = 0;
		this-> _sumFrequency = 0;
		this-> _nbMicros = 0;
		this-> _sumDelta = 0;
		this-> _allocated = 0;
		this-> _buying = 0;
		
		this-> addToHistory ();
	    }

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           GETTERS            =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    unsigned long LibvirtVCPUController::getFrequency () const {
		return this-> _lastFrequency;
	    }
	    
	    unsigned long LibvirtVCPUController::getConsumption () const {
		return this-> _consumption;
	    }

	    int LibvirtVCPUController::getPeriod () const {
		return this-> _period;
	    }

	    float LibvirtVCPUController::getSlope () const {
		return this-> _slope;
	    }

	    int LibvirtVCPUController::getQuota () const {
		return this-> _quota;
	    }

	    unsigned long LibvirtVCPUController::getAbsoluteConsumption () const {
		return ((float) (this-> _consumption) / this-> _delta);
	    }

	    unsigned long LibvirtVCPUController::getAbsoluteCapping () const {
		auto cap = (((float) this-> _quota) * 1000000.0f) / (float) this-> _period;
		return (unsigned long) cap;
	    }
	    
	    float LibvirtVCPUController::getPercentageConsumption () const {
		return ((float) this-> getAbsoluteConsumption ()) / 1000000.0f * 100.0f;
	    }

	    float LibvirtVCPUController::getRelativePercentConsumption () const {
		return ((float) this-> getAbsoluteConsumption ()) / ((float) this-> getAbsoluteCapping ()) * 100.0f;
	    }

	    unsigned long LibvirtVCPUController::getNominalFreq () const {
		return this-> _nominalFreq;
	    }

	    LibvirtVM & LibvirtVCPUController::vm () {
		return this-> _context;
	    }
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================         CONTROLLING          =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    
	    void LibvirtVCPUController::setQuota (unsigned long nbMicros, unsigned long period) {
		this-> _period = period;
		auto cap = (((float) nbMicros) / 1000000.0f) * (float) this-> _period;    
		this-> _quota = (unsigned long) cap;
		if (nbMicros >= (unsigned long) (1000000.0f * 0.85)) {
		    this-> _cgroup.setLimit (-1, this-> _period);
		} else {
		    this-> _cgroup.setLimit (this-> _quota, this-> _period);
		}		

		// logging::info ("VM capping", this-> _context.id (), this-> _id, ":", (float) this-> _quota / (float) this-> _period * 100.0f, "%", this-> getPercentageConsumption (), "% @", this-> _lastFrequency / 1000, "Mhz");
	    }

	    void LibvirtVCPUController::unlimit () {
		this-> setQuota (-1);
	    }

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================            MARKET            =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    unsigned long & LibvirtVCPUController::allocated () {
		return this-> _allocated;
	    }

	    unsigned long & LibvirtVCPUController::buying () {
		return this-> _buying;
	    }
	    

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================             LOG              =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    nlohmann::json LibvirtVCPUController::dumpLogs () const {
		nlohmann::json j;
		j["cycles"] = this-> getAbsoluteConsumption ();
		j["capping"] = this-> getQuota ();
		j["frequency"] = this-> _lastFrequency;

		return j;
	    }	    

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================            SLOPE             =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    void LibvirtVCPUController::addToHistory () {		
		this-> _history.push_back (this-> getPercentageConsumption ());
		if (this-> _history.size () > this-> _maxHistory) {
		    this-> _history.erase (this-> _history.begin ());
		}
	    
		if (this-> _history.size () == this-> _maxHistory) this-> computeSlope ();
	    }

	    void LibvirtVCPUController::computeSlope () {
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

