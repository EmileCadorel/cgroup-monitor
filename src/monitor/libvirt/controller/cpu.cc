#include <monitor/libvirt/controller/cpu.hh>
#include <monitor/libvirt/vm.hh>
#include <libvirt/libvirt.h>
#include <monitor/utils/log.hh>
#include <fstream>

namespace fs = std::filesystem;
using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

	namespace control {

	    LibvirtCpuController::LibvirtCpuController (LibvirtVM & context, int maxHistory) :
		_context (context),
		_maxHistory (maxHistory)
	    {
		std::ifstream f ("/sys/fs/cgroup/cgroup.controllers");
		this-> _cgroupV2 = f.good ();
		f.close ();		
	    }


	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================          ACQUIRING           =========================
	     * ================================================================================
	     * ================================================================================
	     */


	    void LibvirtCpuController::enable () {
		if (this-> _cgroupV2) {
		    auto path = fs::path ("/sys/fs/cgroup/machine.slice");
		    for (;;) {
			this-> _cgroupPath = this-> recursiveSearch (path, "v" + this-> _context.id ());
			if (this-> _cgroupPath.u8string ().length () != 0) break;
			this-> _t.sleep (0.1);
		    } 
		} else {
		    auto path = fs::path ("/sys/fs/cgroup/cpu/machine.slice");
		    for (;;) {
			this-> _cgroupPath = this-> recursiveSearch (path, "v" + this-> _context.id ());
			if (this-> _cgroupPath.u8string ().length () != 0) break;
			this-> _t.sleep (0.1);
		    }
		}

		std::cout << this-> _cgroupPath.c_str () << std::endl;
	    }
	    
	    void LibvirtCpuController::update () {			
		this-> _lastConsumption = this-> _consumption;
		this-> _consumption = this-> readConsumption ();
		if (!this-> _cgroupV2) this-> _consumption /= 1000;

		this-> _delta = this-> _t.time_since_start ();
		this-> _t.reset ();
		
		this-> addToHistory ();
	    }


	    unsigned long LibvirtCpuController::readConsumption () const {
		unsigned long res = 0;
		if (!this-> _cgroupV2) {
		    auto p = this-> _cgroupPath / "cpuacct.usage";
		    std::ifstream t (p);
		    std::stringstream buffer;
		    buffer << t.rdbuf();
		    buffer >> res;
		    t.close ();
		} else {
		    auto p = this-> _cgroupPath / "cpu.stat";
		    std::ifstream t (p);
		    std::string ignore;
		    t >> ignore;
		    t >> res;
		    t.close ();
		}
		return res;
	    }

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           GETTERS            =========================
	     * ================================================================================
	     * ================================================================================
	     */


	    unsigned long LibvirtCpuController::getConsumption () const {
		return this-> _consumption - this-> _lastConsumption;
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

	    unsigned long LibvirtCpuController::getAbsoluteConsumption () const {
		return ((float) (this-> _consumption - this-> _lastConsumption)) / this-> _delta;
	    }

	    unsigned long LibvirtCpuController::getMaximumConsumption () const {
		return this-> _context.vcpus () * 1000000;
	    }

	    unsigned long LibvirtCpuController::getAbsoluteCapping () const {
		auto cap = (((float) this-> _quota) * 1000000.0f) / (float) this-> _period;
		return (unsigned long) cap;
	    }
	    
	    float LibvirtCpuController::getPercentageConsumption () const {
		return ((float) this-> getAbsoluteConsumption ()) / ((float) this-> getMaximumConsumption ()) * 100.0f;
	    }

	    float LibvirtCpuController::getRelativePercentConsumption () const {
		return ((float) this-> getAbsoluteConsumption ()) / ((float) this-> getAbsoluteCapping ()) * 100.0f;
	    }
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================         CONTROLLING          =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    
	    void LibvirtCpuController::setQuota (unsigned long nbMicros, unsigned long period) {
		this-> _period = period;
		auto cap = (((float) nbMicros) / 1000000.0f) * (float) this-> _period;	    
		this-> _quota = (unsigned long) cap;

		logging::info ("VM capping", this-> _context.id (), ":", this-> _quota, "*", this-> _period);
		if (!this-> _cgroupV2) {
		    auto p = this-> _cgroupPath / "cpu.cfs_quota_us";	    
		    if (fs::exists (p)) {
			std::ofstream m (p);
			m << this-> _quota;
			m.close ();
		    }
		} else {
		    auto p = this-> _cgroupPath / "cpu.max";
		    if (fs::exists (p)) {
			std::ofstream m (p);
			m << this-> _quota << " " << this-> _period;
			m.close ();
		    }
		}
	    }

	    void LibvirtCpuController::unlimit () {
		this-> setQuota (-1);
	    }

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================             LOG              =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    nlohmann::json LibvirtCpuController::dumpLogs () const {
		nlohmann::json j;
		j["host-usage"] = this-> getPercentageConsumption ();
		j["cycles"] = this-> getAbsoluteConsumption ();
		j["relative-usage"] = this-> getRelativePercentConsumption ();
		j["capping"] = this-> getQuota ();
		j["period"] = this-> getPeriod ();
		j["slope"] = this-> getSlope ();

		return j;
	    }	    

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================            SLOPE             =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    void LibvirtCpuController::addToHistory () {		
		this-> _history.push_back (this-> getPercentageConsumption ());
		if (this-> _history.size () > this-> _maxHistory) {
		    this-> _history.erase (this-> _history.begin ());
		}
	    
		if (this-> _history.size () == this-> _maxHistory) this-> computeSlope ();
	    }

	    void LibvirtCpuController::computeSlope () {
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

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================            CGROUP            =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    std::filesystem::path LibvirtCpuController::recursiveSearch (const fs::path & path, const std::string & name) {
		if (fs::is_directory (path)) {
		    if (path.u8string ().find(name) != std::string::npos) {
			return path;
		    } else {
			for (const auto & entry : fs::directory_iterator(path)) {
			    if (fs::is_directory (entry.path ())) {
				auto ret = this-> recursiveSearch (fs::path (entry.path ()), name);
				if (ret != "") return ret;
			    }
			}	    
		    }
		}

		return ""; 
	    }

	}

    }
}
