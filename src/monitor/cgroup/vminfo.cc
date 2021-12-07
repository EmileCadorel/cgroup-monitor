#include <monitor/cgroup/vminfo.hh>
#include <sstream>
#include <fstream>
#include <monitor/utils/log.hh>

namespace fs = std::filesystem;
using namespace monitor::utils;

namespace monitor {
    namespace cgroup {

	VMInfo::VMInfo (const fs::path & path) :
	    _path (path),
	    _name (path.filename ()),
	    _conso (0),
	    _cap (0),
	    _period (0)
	{
	    this-> _t.reset ();
	    this-> update ();
	    this-> _lastConso = this-> _conso;
	}


	const fs::path & VMInfo::getPath () const {
	    return this-> _path;
	}
	
	const std::string & VMInfo::getName () const {
	    return this-> _name;
	}

	unsigned long VMInfo::getMaximumConso () const {
	    return this-> _vcpus.size () * 1000000;
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

	unsigned long VMInfo::getPeriod () const {
	    return this-> _period;
	}

	unsigned long VMInfo::getNbVCpus () const {
	    return this-> _vcpus.size ();
	}

	const std::vector <GroupInfo> & VMInfo::getVCpus () const {
	    return this-> _vcpus;
	}


	void VMInfo::applyCapping (unsigned long nbCycle) {
	    auto cap = (((float) nbCycle) / 1000000.0f) * (float) this-> _period;	    
	    this-> _cap = (unsigned long) cap;
	    auto p = this-> _path / "cpu.cfs_quota_us";	    
	    if (fs::exists (p)) {
		std::ofstream m (p);
		m << this-> _cap;
	    }	   
	}

	
	void VMInfo::update () {	    
	    this-> _lastConso = this-> _conso;
	    this-> _delta = this-> _t.time_since_start ();
	    this-> _t.reset ();
	    this-> _conso = this-> readConso () / 1000;
	    
	    this-> _cap = this-> readCap ();
	    this-> _period = this-> readPeriod ();

	    if (this-> _cap != -1 && this-> getAbsoluteConso () > this-> getAbsoluteCapping ()) {
		logging::strange ("Capping is not respected for VM :", this-> _name, this-> getAbsoluteConso (), this-> getAbsoluteCapping (), this-> _delta);
	    }
	    
	    if (this-> _vcpus.size () == 0) {
		int i = 0;
		while (true) {
		    std::stringstream ss;
		    ss << "vcpu" << i;
		    if (fs::exists (this-> _path / ss.str ())) {
			this-> _vcpus.push_back (GroupInfo (this-> _path / ss.str ()));
		    } else break;
		    i += 1;
		}
	    } else {
		for (int i = 0 ; i < this-> _vcpus.size () ; i++) {
		    this-> _vcpus [i].update (this-> _delta);
		}
	    }
	}
	
	
	unsigned long VMInfo::readConso () const {
	    unsigned long res = 0;
	    auto p = this-> _path / "cpuacct.usage";
	    
	    if (fs::exists (p)) {
		std::ifstream t (p);
		std::stringstream buffer;
		buffer << t.rdbuf();
		buffer >> res;
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
	    auto p = this-> _path / "cpu.cfs_period_us";
	    
	    if (fs::exists (p)) {
		std::ifstream t (p);
		std::stringstream buffer;
		buffer << t.rdbuf();
		buffer >> res;
	    }

	    return res;
	}
	
    }
}


std::ostream & operator<< (std::ostream & s, const monitor::cgroup::VMInfo & info) {
    s << "cgroup (" << info.getPath () << ", conso=" << info.getAbsoluteConso () << ", maxConso=" << info.getMaximumConso () << ", cap=" << info.getCapping () << ", period=" << info.getPeriod () << ", nbCpus=" << info.getNbVCpus () << ", percent=" << info.getPercentageConso () << ")";
    return s;
}
