#include "rapl.hh"

namespace server {

    RaplReader::RaplReader () {
	this-> checkRaplCapabilities ();
    }
    
    bool RaplReader::isEnabled () const {
	return this-> _raplEnabled;
    }
    
    void RaplReader::checkRaplCapabilities () {
	this-> _raplPath = "/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj";	
	this-> _raplFile = std::ifstream (this-> _raplPath);
	std::cout << this-> _raplPath.c_str () << " " << this-> _raplFile.is_open () << std::endl;
	if (this-> _raplFile.is_open ()) {
	    this-> _raplEnabled = true;
	    unsigned long value;
	    this-> _raplFile >> value;
	    this-> _raplValue = value;
	} else this-> _raplEnabled = false;
    }

    unsigned long RaplReader::read () {
	this-> _raplFile.sync ();
	this-> _raplFile.seekg (0, this-> _raplFile.beg);
	unsigned long value;
	this-> _raplFile >> value;

	auto res = value - this-> _raplValue;
	this-> _raplValue = value;

	return res;
    }

}
