#include <monitor/cgroup/vminfo.hh>
#include <sstream>
#include <fstream>
#include <monitor/utils/log.hh>
#include <monitor/concurrency/proc.hh>

namespace fs = std::filesystem;
using namespace monitor::utils;

template<typename T>
void pop_front(std::vector<T>& vec)
{
    vec.erase(vec.begin());
}


namespace monitor {
    namespace cgroup {

	VMInfo::VMInfo (const std::string & name, const fs::path & path, unsigned long historyLen, unsigned long baseFreq, bool v2) :
	    _path (path),
	    _name (name),
	    _conso (0),
	    _cap (0),
	    _period (0),
	    _maxhistory (historyLen),
	    _baseFreq (baseFreq),
	    _cgroupV2 (v2)
	{
	    this-> configureMemory ();
	    this-> _t.reset ();
	    this-> update ();
	    this-> _lastConso = this-> _conso;
	    this-> _cap = -1;
	    this-> _period = this-> readPeriod ();
	    this-> _vcpus = 0;
	    
	    int i = 0;
	    while (true) { // Get the vcpus of the VM
		std::stringstream ss; 
		ss << "vcpu" << i;
		if (fs::exists (this-> _path / ss.str ())) {
		    this-> _vcpus += 1;
		} else break;
		i += 1;
	    }
	}


	

	const fs::path & VMInfo::getPath () const {
	    return this-> _path;
	}
	
	const std::string & VMInfo::getName () const {
	    return this-> _name;
	}

	unsigned long VMInfo::getMaximumConso () const {
	    return this-> _vcpus * 1000000;
	}

	unsigned long VMInfo::getAbsoluteConso () const {
	    return ((float) (this-> _conso - this-> _lastConso)) / this-> _delta;
	}

	unsigned long VMInfo::getAbsoluteCapping () const {
	    auto cap = (((float) this-> _cap) * 1000000.0f) / (float) this-> _period;
	    return (unsigned long) cap;
	}

	float VMInfo::getPercentageConso () const {
	    return ((float) this-> getAbsoluteConso ()) / ((float) this-> getMaximumConso ()) * 100.0f;
	}

	float VMInfo::getRelativePercentConso () const {
	    return ((float) this-> getAbsoluteConso ()) / ((float) this-> getAbsoluteCapping ()) * 100.0f;
	}
	
	unsigned long VMInfo::getConso () const {
	    return this-> _conso - this-> _lastConso;
	}

	long VMInfo::getCapping () const {
	    return this-> _cap;
	}

	double VMInfo::getSlope () const {
	    return this-> _slope;
	}

	unsigned long VMInfo::getPeriod () const {
	    return this-> _period;
	}

	unsigned long VMInfo::getNbVCpus () const {
	    return this-> _vcpus;
	}

	unsigned long VMInfo::getBaseFreq () const {
	    return this-> _baseFreq;
	}	
	
	bool VMInfo::update () {	    
	    this-> _lastConso = this-> _conso;
	    bool read = false;
	    if (this-> _cgroupV2) {
		this-> _conso = this-> readConso (read);
	    } else {
		this-> _conso = this-> readConso (read) / 1000;
	    }

	    this-> readMemory (this-> _usedMemory, this-> _allocatedMemory);
	    this-> _delta = this-> _t.time_since_start ();
	    this-> _t.reset ();

	    std::cout << this-> _name << " alloc : " << this-> _allocatedMemory << ", used : " << this-> _usedMemory << std::endl;
	    
	    this-> _history.push_back (this-> getPercentageConso ());
	    if (this-> _history.size () > this-> _maxhistory) {
		pop_front (this-> _history);
	    }

	    this-> _slope = this-> computeSlope (this-> _history);
	    if (this-> _cap != -1 && this-> getAbsoluteConso () > this-> getAbsoluteCapping ()) {
		logging::strange ("Capping is not respected for VM :", this-> _name, this-> getAbsoluteConso (), this-> getAbsoluteCapping (), this-> _delta, this-> getRelativePercentConso ());
	    }
	    
	    return read;
	}

	double VMInfo::computeSlope (const std::vector <double> & values) const {
	    if (values.size () != this-> _maxhistory ) return values.size ();
	    double sum_x = 0;
	    double sum_y = 0;
	    
	    for (int x = 0 ; x < values.size () ; x++) {
		sum_y += values [x];
		sum_x += x;
	    }
	    auto m_x = sum_x / (double) (values.size ());
	    auto m_y = sum_y / (double) (values.size ());
	    double ss_x = 0.0, sp = 0.0;
	    for (int x = 0 ; x < values.size () ; x++) {
		ss_x += (x - m_x) * (x - m_x);
		sp += (x - m_x) * (values [x] - m_y);
	    }
	    
	    return sp/ss_x;
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================    CGROUP READING/WRITING    =========================
	 * ================================================================================
	 * ================================================================================
	 */
	
	void VMInfo::applyCapping (unsigned long nbCycle) {
	    auto cap = (((float) nbCycle) / 1000000.0f) * (float) this-> _period;	    
	    this-> _cap = (unsigned long) cap;
	    if (!this-> _cgroupV2) {
		auto p = this-> _path / "cpu.cfs_quota_us";	    
		if (fs::exists (p)) {
		    std::ofstream m (p);
		    m << this-> _cap;
		}
	    } else {
		auto p = this-> _path / "cpu.max";
		if (fs::exists (p)) {
		    std::ofstream m (p);
		    m << this-> _cap << " " << this-> _period;
		}
	    }
	}

		
	unsigned long VMInfo::readConso (bool & read) {	    
	    unsigned long res = 0;
	    if (!this-> _cgroupV2) {
		auto p = this-> _path / "cpuacct.usage";
		std::ifstream t (p);
		read = t.good ();
		std::stringstream buffer;
		buffer << t.rdbuf();
		buffer >> res;
	    } else {
		auto p = this-> _path / "cpu.stat";
		std::ifstream t (p);
		std::string ignore;
		read = t.good ();
		t >> ignore;
		t >> res;
		t.close ();
	    }
	    return res;
	}

	long VMInfo::readCap () const {
	    long res = -1;
	    auto p = this-> _path / "cpu.cfs_quota_us";
	    
	    if (fs::exists (p)) {
		std::ifstream t (p);
		std::stringstream buffer;
		buffer << t.rdbuf();
		buffer >> res;
	    }

	    return res;
	}


	unsigned long VMInfo::readPeriod () const {
	    unsigned long res = 0;
	    if (!this-> _cgroupV2) {
		auto p = this-> _path / "cpu.cfs_period_us";
		
		if (fs::exists (p)) {
		    std::ifstream t (p);
		    std::stringstream buffer;
		    buffer << t.rdbuf();
		    buffer >> res;
		}
	    } else {
		res = 100000;
	    }

	    return res;
	}

	void VMInfo::configureMemory () const {
	    auto proc = concurrency::SubProcess ("virsh", {"-c", "qemu:///system",  "dommemstat", "v" + this-> _name, "--period", "1", "--live"}, ".");
	    proc.start ();
	    proc.wait ();
	}
	
	
	
	void VMInfo::readMemory (unsigned long & used, unsigned long & allocated) const {
	    auto proc = concurrency::SubProcess ("virsh", {"-c", "qemu:///system", "--readonly", "dommemstat", "v" + this-> _name}, ".");
	    proc.start ();
	    proc.wait ();
	    
	    auto result = proc.stdout ().read ();
	    std::stringstream ss (result);
	    std::string T;
	    while (std::getline (ss, T, '\n')) {
		if (T.rfind ("actual", 0) == 0) {
		    std::stringstream ss2;
		    ss2 << T.substr (7);
		    ss2 >> allocated;
		} else if (T.rfind ("unused", 0) == 0) {
		    std::stringstream ss2;
		    ss2 << T.substr (7);
		    ss2 >> used;
		    used = allocated - used;
		    return;
		}
	    }
	}
	
    }
}


std::ostream & operator<< (std::ostream & s, const monitor::cgroup::VMInfo & info) {
    s << "cgroup (" << info.getPath () << ", conso=" << info.getAbsoluteConso () << ", maxConso=" << info.getMaximumConso () << ", cap=" << info.getCapping () << ", period=" << info.getPeriod () << ", nbCpus=" << info.getNbVCpus () << ", percent=" << info.getPercentageConso () << ", freq=" << info.getBaseFreq () << ")";
    return s;
}
