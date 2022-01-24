#include <monitor/libvirt/vm.hh>
#include <monitor/libvirt/client.hh>
#include <fstream>
#include <iostream>
#include <monitor/utils/log.hh>

using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

	LibvirtVM::LibvirtVM (const utils::config::dict & cfg, LibvirtClient & context) :
	    _context (context),
	    _cpuController (*this),
	    _memoryController (*this)
	{
	    auto inner = cfg.get <utils::config::dict> ("vm");
	    this-> _id = inner.get<std::string> ("name");
	    this-> _userName = inner.getOr<std::string> ("user", "phil");
	    this-> _qcow = inner.get<std::string> ("image");
	    this-> _disk = inner.getOr<int> ("disk", 10000);
	    this-> _vcpus = inner.getOr<int> ("vcpus", 1);
	    this-> _mem = inner.getOr <int> ("memory", 2048);
	    this-> _freq = inner.getOr<int> ("frequency", 1000);
	    
	    std::filesystem::path home = getenv ("HOME");	    
	    this-> pubKey (inner.getOr <std::string> ("ssh_key", std::string ((home / ".ssh/id_rsa.pub").c_str ())));

	    this-> _dom = nullptr;
	}
	    
	
	LibvirtVM::LibvirtVM (const std::string & name, LibvirtClient & context) :
	    _id (name),
	    _userName ("phil"),
	    _disk (10000),
	    _vcpus (1),
	    _mem (2048),
	    _context (context),
	    _cpuController (*this),
	    _memoryController (*this)
	{
	    std::filesystem::path home = getenv ("HOME");	    
	    this-> pubKey (home / ".ssh/id_rsa.pub");

	    this-> _dom = nullptr;
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

	int LibvirtVM::freq () const {
	    return this-> _freq;
	}

	LibvirtVM & LibvirtVM::freq (int fr) {
	    this-> _freq = fr;
	    return *this;
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================         CONTROLLERS          =========================
	 * ================================================================================
	 * ================================================================================
	 */

	void LibvirtVM::updateControllers () {
	    this-> _cpuController.update ();
	    this-> _memoryController.update ();
	}

	control::LibvirtCpuController & LibvirtVM::getCpuController () {
	    return this-> _cpuController;
	}

	const control::LibvirtCpuController & LibvirtVM::getCpuController () const {
	    return this-> _cpuController;
	}
	
	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================            DOMAIN            =========================
	 * ================================================================================
	 * ================================================================================
	 */


	void LibvirtVM::provision () {
	    if (this-> _dom == nullptr) {
		this-> _context.provision (*this);
		if (this-> _dom != nullptr) {
		    this-> _memoryController.enable ();
		}
	    } else {
		logging::warn ("VM", this-> _id, "is already provisionned");
	    }
	}

	void LibvirtVM::kill () {
	    if (this-> _dom != nullptr) {
		this-> _context.kill (*this);
		virDomainFree (this-> _dom);
	    } else {
		logging::warn ("VM", this-> _id, "is already down");
	    }
	}

	LibvirtVM::~LibvirtVM () {
	}
	
	
	
    }
    
}
