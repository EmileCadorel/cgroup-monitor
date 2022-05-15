#include "rapl.hh"

namespace server {

    RaplReader::RaplReader () {
	this-> checkRaplCapabilities ();
    }
    
    bool RaplReader::isEnabled () const {
	return this-> _raplEnabled;
    }
    
    void RaplReader::checkRaplCapabilities () {
	this-> _raplPath0 = "/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj";	
	this-> _raplFile0 = std::ifstream (this-> _raplPath0);

	this-> _raplPath1 = "/sys/class/powercap/intel-rapl/intel-rapl:1/energy_uj";	
	this-> _raplFile1 = std::ifstream (this-> _raplPath1);
	
	std::cout << this-> _raplPath0.c_str () << " " << this-> _raplFile1.is_open () << std::endl;
	if (this-> _raplFile0.is_open ()) {
	    this-> _raplEnabled = true;
	    this-> _raplFile0 >> this-> _raplValue0;
	    this-> _raplFile1 >> this-> _raplValue1;
	} else this-> _raplEnabled = false;
    }

    unsigned long RaplReader::readPP0 () {
	this-> _raplFile0.sync ();
	this-> _raplFile0.seekg (0, this-> _raplFile0.beg);
	unsigned long value;
	this-> _raplFile0 >> value;

	auto res = value - this-> _raplValue0;
	this-> _raplValue0 = value;

	return res;
    }

    unsigned long RaplReader::readPP1 () {
	this-> _raplFile1.sync ();
	this-> _raplFile1.seekg (1, this-> _raplFile1.beg);
	unsigned long value;
	this-> _raplFile1 >> value;

	auto res = value - this-> _raplValue1;
	this-> _raplValue1 = value;

	return res;
    }

}
