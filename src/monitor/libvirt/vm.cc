#include <monitor/libvirt/vm.hh>
#include <fstream>
#include <iostream>

namespace monitor {

    namespace libvirt {

	LibvirtVM::LibvirtVM (const std::string & name) :
	    _id (name),
	    _userName ("phil"),
	    _disk (10000),
	    _vcpus (1),
	    _mem (2048)
	{
	    std::filesystem::path home = getenv ("HOME");	    
	    this-> pubKey (home / ".ssh/id_rsa.pub");
	}	    	
	
	const std::string & LibvirtVM::id () const {
	    return this-> _id;
	}

	LibvirtVM & LibvirtVM::qcow (const std::filesystem::path & path) {
	    this-> _qcow = path;
	    return *this;
	}
	
	const std::filesystem::path & LibvirtVM::qcow () const {
	    return this-> _qcow;
	}

	LibvirtVM & LibvirtVM::user (const std::string & name) {
	    this-> _userName = name;
	    return *this;
	}

	const std::string & LibvirtVM::user () const {
	    return this-> _userName;
	}

	LibvirtVM & LibvirtVM::pubKey (const std::filesystem::path & path) {
	    std::ifstream s (path);
	    std::stringstream ss;
	    ss << s.rdbuf ();
	    this-> _pubKey = ss.str ();
	    this-> _pubKey = this-> _pubKey.substr (0, this-> _pubKey.length () - 1); // remove final \n

	    return *this;
	}

	const std::string & LibvirtVM::pubKey () const {
	    return this-> _pubKey;
	}


	LibvirtVM & LibvirtVM::disk (int size) {
	    this-> _disk = size;
	    return *this;
	}

	int LibvirtVM::disk () const {
	    return this-> _disk;
	}

	
	LibvirtVM & LibvirtVM::memory (int size) {
	    this-> _mem = size;
	    return *this;
	}

	int LibvirtVM::memory () const {
	    return this-> _mem;
	}

	LibvirtVM & LibvirtVM::vcpus (int nb) {
	    this-> _vcpus = nb;
	    return *this;
	}

	int LibvirtVM::vcpus () const {
	    return this-> _vcpus;
	}

	LibvirtVM & LibvirtVM::mac (const std::string & value) {
	    this-> _mac = value;
	    return *this;
	}

	const std::string & LibvirtVM::mac () const {
	    return this-> _mac;
	}

	LibvirtVM & LibvirtVM::ip (const std::string & ip) {
	    this-> _ip = ip;
	    return *this;
	}

	const std::string & LibvirtVM::ip () const {
	    return this-> _ip;
	}
	
	
    }
    
}
