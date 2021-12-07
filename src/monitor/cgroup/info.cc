#include <monitor/cgroup/info.hh>
#include <sstream>
#include <fstream>

namespace fs = std::filesystem;

namespace monitor {
    namespace cgroup {

	GroupInfo::GroupInfo (const fs::path & path) :
	    _path (path),
	    _name (path.filename ()),
	    _conso (0),
	    _cap (0),
	    _period (0)
	{
	    this-> update (1.0);
	    this-> _lastConso = this-> _conso;
	    this-> _conso = 0;
	}


	const fs::path & GroupInfo::getPath () const {
	    return this-> _path;
	}
	
	const std::string & GroupInfo::getName () const {
	    return this-> _name;
	}

	unsigned long GroupInfo::getConso () const {
	    return this-> _conso - this-> _lastConso;
	}

	long GroupInfo::getCapping () const {
	    return this-> _cap;
	}

	unsigned long GroupInfo::getPeriod () const {
	    return this-> _period;
	}

	
	void GroupInfo::update (float delta) {	    
	    this-> _lastConso = this-> _conso;
	    this-> _conso = this-> readConso ();
	    this-> _cap = this-> readCap ();
	    this-> _period = this-> readPeriod ();	    
	}
	
	
	unsigned long GroupInfo::readConso () const {
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

	long GroupInfo::readCap () const {
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


	unsigned long GroupInfo::readPeriod () const {
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


std::ostream & operator<< (std::ostream & s, const monitor::cgroup::GroupInfo & info) {
    s << "cgroup (" << info.getPath () << ", conso=" << info.getConso () << ", cap=" << info.getCapping () << ", period=" << info.getPeriod () << ")";
    return s;
}
