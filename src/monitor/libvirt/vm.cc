#include <monitor/libvirt/vm.hh>
#include <monitor/libvirt/client.hh>
#include <fstream>
#include <iostream>
#include <monitor/utils/log.hh>

using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

	LibvirtVM::LibvirtVM (const utils::config::dict & cfg) :
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
	    this-> _memorySLA = inner.getOr <float> ("memorySLA", 0.5);
	    
	    std::filesystem::path home = getenv ("HOME");	    
	    this-> _pubKey = inner.getOr <std::string> ("ssh_key", "");

	    this-> _dom = nullptr;
	}
	    
	
	LibvirtVM::LibvirtVM (const std::string & name) :
	    _id (name),
	    _userName ("phil"),
	    _disk (10000),
	    _vcpus (1),
	    _mem (2048),
	    _memorySLA (0.5),
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

	LibvirtVM & LibvirtVM::memorySLA (float sla) {
	    this-> _memorySLA = sla;
	    return *this;
	}

	float LibvirtVM::memorySLA () const {
	    return this-> _memorySLA;
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

	control::LibvirtCpuController & LibvirtVM::getCpuController () {
	    return this-> _cpuController;
	}

	const control::LibvirtCpuController & LibvirtVM::getCpuController () const {
	    return this-> _cpuController;
	}

	control::LibvirtMemoryController & LibvirtVM::getMemoryController () {
	    return this-> _memoryController;
	}

	const control::LibvirtMemoryController & LibvirtVM::getMemoryController () const {
	    return this-> _memoryController;
	}	
	
	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================            DOMAIN            =========================
	 * ================================================================================
	 * ================================================================================
	 */


	LibvirtVM::~LibvirtVM () {
	}
	
	
	
    }
    
}
