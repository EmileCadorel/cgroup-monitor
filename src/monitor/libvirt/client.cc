#include <monitor/libvirt/client.hh>
#include <monitor/concurrency/proc.hh>
#include <monitor/utils/log.hh>
#include <sys/stat.h>
#include <fstream>
#include <monitor/concurrency/timer.hh>
#include <unistd.h>
#include <sys/types.h>
#include <nlohmann/json.hpp>
#include <monitor/utils/xml.hh>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <monitor/utils/log.hh>

using namespace monitor::utils;
using namespace tinyxml2;
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace monitor {

    namespace libvirt {

	LibvirtClient::LibvirtClient (const char * uri) :
	    _conn (nullptr), _uri (uri)
	{
	    if (getuid()) {
		logging::error ("you are not root. This program will only work if run as root.");
		exit(1);
	    }
	    
	    this-> enableNatRouting ();
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================          CONNECTION          =========================
	 * ================================================================================
	 * ================================================================================
	 */

	void LibvirtClient::setKeyPath (const std::filesystem::path & path) {
	    this-> _keyPath = path;
	    std::ifstream f (path / "key.pub");
	    std::stringstream ss;
	    ss << f.rdbuf ();
	    this-> _pubKey = ss.str ();
	    this-> _pubKey = this-> _pubKey.substr (0, this-> _pubKey.length () - 1); // remove final \n
	    f.close ();
	}
       
	void LibvirtClient::connect () {
	    // First disconnect, maybe it was connected to something
	    this-> disconnect ();
	    
	    // We need an auth connection to have write access to the domains
	    this-> _conn = virConnectOpenAuth (this-> _uri, virConnectAuthPtrDefault, 0);
	    if (this-> _conn == nullptr) {
		logging::error ("Failed to connect libvirt client to :", this-> _uri);
		throw LibvirtError ("Connection to hypervisor failed\n");
	    }

	    logging::success ("Libvirt client connected to :", this-> _uri);
	    this-> killAllRunningDomains ();
	}

	void LibvirtClient::disconnect () {
	    if (this-> _conn != nullptr) {
		virConnectClose (this-> _conn);
		this-> _conn = nullptr;
		
		logging::info ("Libvirt client disconnected");
	    }
	}


	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================         CONTROLLERS          =========================
	 * ================================================================================
	 * ================================================================================
	 */

	void LibvirtClient::updateVCPUControllers () {
	    auto speed = this-> readCPUFrequency ();
	    
	    for (auto & vm : this-> _running) {
		for (auto & vt : vm-> getVCPUControllers ()) {
		    vt.update (speed);
		}
	    }
	}

	void LibvirtClient::updateVCPUBeforeMarket () {
	    for (auto & vm : this-> _running) {
		for (auto & vt : vm-> getVCPUControllers ()) {
		    vt.updateBeforeMarket ();
		}
	    }
	}
	
	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================            DOMAIN            =========================
	 * ================================================================================
	 * ================================================================================
	 */

	void LibvirtClient::killAllRunningDomains () {
	    virDomainPtr * domains = nullptr;
	    auto num_domains = virConnectListAllDomains (this-> _conn, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
	    for (int i = 0 ; i < num_domains ; i++) {
		virDomainPtr dom = domains [i];
		auto name = virDomainGetName (dom);
		if (std::string (name)[0] == 'v') {
		    logging::info ("Killing VM:", name + 1);
		    virDomainDestroy (dom);
		    virDomainUndefine (dom);
		    virDomainFree (dom);
		}
	    }

	    free (domains);
	    this-> _running.clear ();
	}
	
	void LibvirtClient::printDomains () const {
	    virDomainPtr * domains = nullptr;	    
	    auto num_domains = virConnectListAllDomains (this-> _conn, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
	    for (int i = 0 ; i < num_domains ; i++) {
		virDomainPtr dom = domains [i];
		auto name = virDomainGetName (dom);
		logging::info ("Running Domain :", name);
		virDomainFree (dom);
	    }

	    free (domains);
	}

	virDomainPtr LibvirtClient::retreiveDomain (const std::string & name) {
	    return virDomainLookupByName (this-> _conn, (name).c_str ());	   
	}

	XMLDocument* LibvirtClient::getXML (const LibvirtVM & vm) const {
	    XMLDocument * doc = new XMLDocument ();
	    if (vm._dom != nullptr) {
		char * xml = virDomainGetXMLDesc (vm._dom, 0);
		
		doc-> Parse (xml);
		
		free (xml);
		
		return doc;
	    } else {
		return doc;
	    }
	}

	bool LibvirtClient::hasVM (const std::string & name) {
	    this-> _mutex.lock ();
	    bool found = false;
	    for (auto & vm : this-> _running) {
		if (vm-> id () == name) {
		    found = true;
		    break;
		}
	    }
	    this-> _mutex.unlock ();
	    
	    return found;
	}

	LibvirtVM* LibvirtClient::getVM (const std::string & name) {
	    this-> _mutex.lock ();
	    LibvirtVM * ret = nullptr;
	    for (auto & vm : this-> _running) {
		if (vm-> id () == name) {
		    ret = vm;
		    break;
		}
	    }
	    this-> _mutex.unlock ();
	    
	    return ret;
	}

	void LibvirtClient::removeVM (const std::string & name) {
	    std::vector <LibvirtVM*> res;
	    this-> _mutex.lock ();
	    LibvirtVM * ret = nullptr;
	    for (auto & vm : this-> _running) {
		if (vm-> id () != name) {
		    res.push_back (vm);
		    break;
		}
	    }
	    this-> _mutex.unlock ();
	    this-> _running = std::move (res);	    
	}
	

	std::vector <LibvirtVM*> & LibvirtClient::getRunningVMs () {
	    return this-> _running;
	}

       	
	const LibvirtVM * LibvirtClient::provision (const utils::config::dict & cfg, const std::filesystem::path & path) {
	    auto vm = new LibvirtVM (cfg);
	    try {
		this-> kill (vm-> id ());
	    
		auto vPath = path / (vm-> id ());
	    
		// Prepare the different file required for the VM booting
		this-> createDirAndVMFile (*vm, vPath);

		// install the VM on the host using virsh
		this-> installVM (*vm, vPath);

		// wait the ip of the VM
		this-> waitIpVM (*vm, vPath);

		logging::success ("VM", vm-> id (), "is ready at ip : ", vm-> ip ());
	    
		vm-> _dom = this-> retreiveDomain (vm-> id ());
	    
		// for (auto &it : vm-> getVCPUControllers ()) {
		//     it.enable ();
		// }
			    
		this-> _running.push_back (vm);

		return vm;
	    } catch (LibvirtError err) {
		delete vm;
		throw err;
	    }
	}


	void LibvirtClient::kill (const std::string & vm, const std::filesystem::path & path) {
	    auto v = this-> getVM (vm);
	    if (v != nullptr) {
		auto vPath = path / (v-> id ());
		
		// Kill the domain
		this-> killDomain (*v);

		// Destroy the vm file, and associated disks
		this-> deleteDirAndVMFile (*v, vPath);

		logging::success ("VM", v-> id (), "is killed");

		this-> removeVM (vm);
		delete v;		
	    }
	}
	
	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================    PROVISION INNER FUNCS     =========================
	 * ================================================================================
	 * ================================================================================
	 */

	void LibvirtClient::createDirAndVMFile (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    auto qcowFile = vm.qcow ();	    
	    auto osPath = vPath / (vm.id () + ".qcow2");
	    try {
		fs::create_directories (vPath);
		fs::copy_file (qcowFile, osPath, fs::copy_options::overwrite_existing);
		::chmod (osPath.c_str (), 0777);

		// Create the iso containing user data 
		this-> createUserData (vm, vPath);
		this-> createMetaData (vm, vPath);
		this-> createISOData (vm, vPath);

		// Resize the image disk size according to user demand
		this-> resizeImage (vm, vPath);

		// Prepare the image for booting
		this-> prepareImage (vm, vPath);
		
	    } catch (std::exception & e) {
		throw LibvirtError (e.what ());
	    }
	}

	void LibvirtClient::createUserData (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    auto userPath = vPath / "user-data";
	    
	    std::stringstream ss;
	    ss << "#cloud-config" << std::endl << "---" << std::endl;
	    ss << "users:" << std::endl;
	    ss << "- name: " << vm.user () << std::endl;
	    ss << "  password: " << vm.user () << std::endl;
	    ss << "  shell: /bin/bash" << std::endl;
	    ss << "  ssh-authorized-keys: [" << vm.pubKey () << "," << this-> _pubKey << "]" << std::endl;
	    ss << "  groups: wheel" << std::endl;
	    ss << "  sudo: ['ALL=(ALL) NOPASSWD:ALL']" << std::endl;
	    ss << "  lock_passwd: 'false'" << std::endl;

	    std::ofstream outFile (userPath);
	    outFile << ss.str ();
	    outFile.close ();

	    ::chmod (userPath.c_str (), 0644);
	}

	void LibvirtClient::createMetaData (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    auto metaPath = vPath / "meta-data";

	    std::stringstream ss;
	    ss << "{instance-id: v" << vm.id () << ", local-hostname: v" << vm.id () << "}" << std::endl;

	    std::ofstream outFile (metaPath);
	    outFile << ss.str ();
	    outFile.close ();

	    ::chmod (metaPath.c_str (), 0644);
	}


	void LibvirtClient::createISOData (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    auto proc = concurrency::SubProcess ("mkisofs", {"-o", "user.iso", "-V", "cidata", "-J", "-r", "user-data", "meta-data"}, vPath.c_str ());
	    proc.start ();
	    if (proc.wait () != 0) {
		std::cout << "ERROR : " << proc.stderr ().read () << std::endl;
		std::cout << "OUT : " << proc.stdout ().read () << std::endl;
	    }
	}

	void LibvirtClient::resizeImage (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    std::stringstream ss;
	    ss  << "+" << vm.disk () << "M";
	    auto proc = concurrency::SubProcess ("qemu-img", {"resize", vm.id () + ".qcow2", ss.str ()}, vPath.c_str ());
	    proc.start ();
	    if (proc.wait () != 0) {
		std::cout << "ERROR : " << proc.stderr ().read () << std::endl;
		std::cout << "OUT : " << proc.stdout ().read () << std::endl;
	    }
	}

	void LibvirtClient::prepareImage (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    auto proc = concurrency::SubProcess ("virt-sysprep", {"-a", vm.id () + ".qcow2"}, vPath);
	    proc.start ();
	    if (proc.wait () != 0) {
		std::cout << "ERROR : " << proc.stderr ().read () << std::endl;
		std::cout << "OUT : " << proc.stdout ().read () << std::endl;
	    }
	}

	void LibvirtClient::createSwapDisk (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    std::stringstream mem;
	    mem << vm.memory () << "M";
	    auto proc = concurrency::SubProcess ("qemu-img", {"create", "-f", "raw", "swap-space.img", mem.str ()}, vPath);
	    proc.start ();
	    
	    if (proc.wait () != 0) {
		std::cout << "ERROR : " << proc.stderr ().read () << std::endl;
		std::cout << "OUT : " << proc.stdout ().read () << std::endl;
	    }	    
	}
	
	void LibvirtClient::installVM (const LibvirtVM & vm, const std::filesystem::path & path) {
	    std::stringstream mem, cpu;
	    mem << vm.memory ();
	    cpu << vm.vcpus ();
	    
	    auto proc = concurrency::SubProcess ("virt-install", {"--import", "--name", vm.id (),
								  "--ram", mem.str (),
								  "--vcpu", cpu.str (),
								  "--disk", vm.id () + ".qcow2,format=qcow2,bus=virtio",
								  "--disk", "user.iso,device=cdrom",
								  "--network", "bridge=virbr0,model=virtio" ,
								  "--os-variant", vm.os (),
								  "--virt-type", "kvm",
								  "--noautoconsole"},
		path);
	    this-> _mutex.lock ();
	    
	    proc.start ();
	    if (proc.wait () != 0) {
		std::cout << "ERROR : " << proc.stderr ().read () << std::endl;
		std::cout << "OUT : " << proc.stdout ().read () << std::endl;
	    }
	    this-> _mutex.unlock ();
								  
	}

	void LibvirtClient::waitIpVM (LibvirtVM & vm, const std::filesystem::path & path) {
	    concurrency::timer timer;
	    std::string mac = "", ip = "";
	    virDomainPtr dom = nullptr;
	    for (;;) {
		dom = virDomainLookupByName (this-> _conn, (vm.id ()).c_str ());
		if (dom != nullptr) break;
	    }
	    
	    for (;;) { // retreive the mac address of the vm
		char * xml = virDomainGetXMLDesc (dom, 0);
		if (xml != nullptr) {
		    try {
			XMLDocument doc;
			doc.Parse (xml);
			delete xml;
		    
			auto macXML = utils::findInXML (doc.RootElement (), {"devices", "interface", "mac"});
			if (macXML != nullptr) {
			    mac = macXML-> Attribute ("address");
			    if (mac != "") break;
			}
		    } catch (...) {
		    }
		}
	    }

	    for (;;) { // retreive the ip address of the vm from the mac address
		std::ifstream f ("/var/lib/libvirt/dnsmasq/virbr0.status");
		std::stringstream ss;
		ss << f.rdbuf ();
		f.close ();
		try {
		    auto j = json::parse (ss.str ());
		    
		    for (auto & it : j) {
			if (it["mac-address"] == mac) {
			    ip = it ["ip-address"];
			    break;
			}
		    }
		    if (ip != "") break;
		} catch (...) {}
	    }

	    vm.mac (mac).ip (ip);
	}
	
	void LibvirtClient::attachSwapDisk (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    auto proc = concurrency::SubProcess ("virsh", {"attach-disk", vm.id (), (vPath / "swap-space.img").c_str (), "--target", "vdb", "--persistent"}, vPath);

	    proc.start ();
	    
	    if (proc.wait () != 0) {
		std::cout << "ERROR : " << proc.stderr ().read () << std::endl;
		std::cout << "OUT : " << proc.stdout ().read () << std::endl;
	    }	    	    
	}


	void LibvirtClient::mountSwap (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    std::stringstream ss;
	    ss << vm.user () << "@" << vm.ip ();

	    auto chproc = concurrency::SubProcess ("chmod", {"400", (this-> _keyPath / "key").c_str ()}, vPath);
	    chproc.start ();
	    if (chproc.wait () != 0) {
		std::cout << "ERROR : " << chproc.stderr ().read () << std::endl;
		std::cout << "OUT : " << chproc.stdout ().read () << std::endl;
		exit (-1);
	    }
	    
	    for (;;) {
		auto proc = concurrency::SubProcess ("ssh", {"-o", "StrictHostKeyChecking=no", ss.str (), "-i", (this-> _keyPath / "key").c_str (), "sudo mkswap /dev/vdb ; sudo swapon /dev/vdb"}, vPath);
		proc.start ();
		
		if (proc.wait () != 0) {		    
		    concurrency::timer t;
		    t.sleep (0.5);
		} else {
		    break;
		}
	    } 
	}
	
	
	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================     KILLING INNER FUNCS      =========================
	 * ================================================================================
	 * ================================================================================
	 */

	void LibvirtClient::killDomain (const LibvirtVM & vm) const {
	    auto dom = vm._dom;
	    virDomainDestroy (dom);
	    virDomainUndefine (dom);
	}

	void LibvirtClient::deleteDirAndVMFile (const LibvirtVM & vm, const std::filesystem::path & path) const {
	    auto isoFile = path / ("user.iso");
	    auto metaFile = path / ("meta-data");
	    auto userFile = path / ("user-data");
	    auto qcowFile = path / (vm.id () + ".qcow2");
	    auto swapFile = path / ("swap-space.img");

	    ::remove (isoFile.c_str ());
	    ::remove (metaFile.c_str ());
	    ::remove (userFile.c_str ());	    
	    ::remove (qcowFile.c_str ());
	    ::remove (swapFile.c_str ());
	}


	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================           NETWORK            =========================
	 * ================================================================================
	 * ================================================================================
	 */


	LibvirtVM & LibvirtClient::openNat (LibvirtVM & vm, int host, int guest) {
	    if (vm.ip () != "") {
		std::stringstream hport, gport;
		hport << host;
		gport << guest;
	    
		auto proc = concurrency::SubProcess ("iptables", {"-t", "nat", "-I", "PREROUTING", "-p", "tcp", "--dport", hport.str (), "-j", "DNAT", "--to", vm.ip () + ":" + gport.str ()}, ".");
		proc.start ();
		proc.wait ();

		auto proc2 = concurrency::SubProcess ("iptables", {"-t", "nat", "-I", "OUTPUT", "-p", "tcp", "--dport", hport.str (), "-j", "DNAT", "--to", vm.ip () + ":" + gport.str ()}, ".");
		proc2.start ();
		proc2.wait ();

		// auto proc3 = concurrency::SubProcess ("iptables", {"-t", "nat", "-I", "INPUT", "-p", "tcp", "--dport", hport.str (), "-j", "DNAT", "--to", vm.ip () + ":" + gport.str ()}, ".");
		// proc3.start ();
		// proc3.wait ();
	    }

	    return vm;
	}

	void LibvirtClient::enableNatRouting () const {	    
	    struct ifaddrs *addresses;
	    if (getifaddrs(&addresses) == -1)
	    {
		return ;
	    }	    
	    
	    struct ifaddrs *address = addresses;
	    while(address)
	    {
		int family = address->ifa_addr->sa_family;
		if (family == AF_INET && std::string (address->ifa_name) != "virbr0")
		{
		    char ap[100];
		    const int family_size = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
		    getnameinfo(address->ifa_addr,family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
		    std::string ip_face = std::string (ap);
		    
		    auto proc = concurrency::SubProcess ("iptables", {"-I", "FORWARD", "-m", "state", "-o", ip_face, "-d", "192.168.122.0/24", "--state", "NEW,RELATED,ESTABLISHED", "-j", "ACCEPT"}, ".");
		    proc.start ();
		    proc.wait ();
		    logging::info ("NAT routing enabled for interface ip :", ip_face);
		}
		address = address->ifa_next;
	    }
	    
	    freeifaddrs(addresses);
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================        CPU FREQUENCY         =========================
	 * ================================================================================
	 * ================================================================================
	 */


	const std::vector <unsigned int> & LibvirtClient::readCPUFrequency () {
	    if (this-> _cpuPath.size () != get_nprocs ()) {
		for (auto i = 0 ; i < get_nprocs (); i++) {
		    std::stringstream path;
		    path << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_cur_freq";
		    this-> _cpuPath.push_back (path.str ());
		    this-> _cpuFreq.push_back (0);
		}
	    }
	    
	    for (auto i = 0 ; i < this-> _cpuFreq.size (); i++) {
		std::ifstream f (this-> _cpuPath[i]);
		std::stringstream buffer;
		std::string res;
		buffer << f.rdbuf();
		unsigned int p;
		buffer >> this-> _cpuFreq[i];
		f.close ();
	    }

	    return this-> _cpuFreq;
	}

	const std::vector <unsigned int> & LibvirtClient::getLastCPUFrequency () {
	    return this-> _cpuFreq;
	}
	
	
	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================             DTOR             =========================
	 * ================================================================================
	 * ================================================================================
	 */
	
	LibvirtClient::~LibvirtClient () {
	    this-> disconnect ();
	}
	
    }
    
}
