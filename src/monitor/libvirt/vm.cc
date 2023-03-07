#include <monitor/libvirt/vm.hh>
#include <monitor/libvirt/client.hh>
#include <fstream>
#include <iostream>
#include <monitor/utils/log.hh>

using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

		LibvirtVM::LibvirtVM (const utils::config::dict & cfg)
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
			this-> _os = inner.get<std::string> ("os");
	    
			std::filesystem::path home = getenv ("HOME");
			this-> _pubKey = inner.getOr <std::string> ("ssh_key", "");

			this-> _dom = nullptr;
			this-> _money = 0;

			for (int i = 0 ; i < this-> _vcpus ; i++) {
				this-> _vcpuControllers.push_back (control::LibvirtVCPUController (i, *this));
			}
		}
	    
	
		LibvirtVM::LibvirtVM (const std::string & name) :
			_id (name),
			_userName ("phil"),
			_disk (10000),
			_vcpus (1),
			_mem (2048),
			_memorySLA (0.5)
		{
			std::filesystem::path home = getenv ("HOME");
			this-> pubKey (home / ".ssh/id_rsa.pub");

			this-> _dom = nullptr;
			this-> _money = 0;
	    
			for (int i = 0 ; i < this-> _vcpus ; i++) {
				this-> _vcpuControllers.push_back (control::LibvirtVCPUController (i, *this));
			}
		}
	
		const std::string & LibvirtVM::id () const {
			return this-> _id;
		}

		const std::string & LibvirtVM::os () const {
			return this-> _os;
		}

		LibvirtVM& LibvirtVM::os (const std::string & os) {
			this-> _os = os;
			return *this;
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

		const std::vector <control::LibvirtVCPUController> & LibvirtVM::getVCPUControllers () const {
			return this-> _vcpuControllers;
		}

		std::vector <control::LibvirtVCPUController> & LibvirtVM::getVCPUControllers () {
			return this-> _vcpuControllers;
		}


		void LibvirtVM::pinCores (const std::string & cores) {
			for (auto & c : this-> _vcpuControllers) {
				c.setCorePins (cores);
			}
		}

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================            MARKET            =========================
		 * ================================================================================
		 * ================================================================================
		 */

		unsigned long & LibvirtVM::money () {
			return this-> _money;
		}

		void LibvirtVM::applyMarketAllocation (unsigned long period) {
			// logging::info ("VM Money :", this-> _id, this-> _money);
			for (auto & c : this-> _vcpuControllers) {
				c.setQuota (c.allocated (), period);
			}
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
