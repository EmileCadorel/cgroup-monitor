#include <monitor/libvirt/client.hh>
#include <monitor/concurrency/proc.hh>
#include <monitor/utils/log.hh>
#include <sys/stat.h>
#include <fstream>
#include <monitor/concurrency/timer.hh>
#include <unistd.h>
#include <sys/types.h>
#include <nlohmann/json.hpp>

using namespace monitor::utils;
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace monitor {

    namespace libvirt {

	LibvirtClient::LibvirtClient (const char * uri) :
	    _conn (nullptr), _uri (uri)
	{
	    if (getuid()) {
		logging::error ("you are not root. This program will only word if run as root.");
		exit(1);
	    }
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================          CONNECTION          =========================
	 * ================================================================================
	 * ================================================================================
	 */
       
	void LibvirtClient::connect () {
	    // First disconnect, maybe it was connected to something
	    this-> disconnect ();
	    
	    // We need an auth connection to have write access to the domains
	    this-> _conn = virConnectOpenAuth (this-> _uri, virConnectAuthPtrDefault, 0);
	    if (this-> _conn == nullptr) {
		logging::error ("Failed to connect libvirt client to : ", this-> _uri);
		throw LibvirtError ("Connection to hypervisor failed\n");
	    }

	    logging::success ("Libvirt client connected to : ", this-> _uri);
	}

	void LibvirtClient::disconnect () {
	    if (this-> _conn != nullptr) {
		logging::info ("Disconnect libvirt client");
		virConnectClose (this-> _conn);
		this-> _conn = nullptr;
	    }
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================            DOMAIN            =========================
	 * ================================================================================
	 * ================================================================================
	 */
	
	void LibvirtClient::printDomains () const {
	    virDomainPtr * domains = nullptr;	    
	    auto num_domains = virConnectListAllDomains (this-> _conn, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
	    for (int i = 0 ; i < num_domains ; i++) {
		virDomainPtr dom = domains [i];
		auto name = virDomainGetName (dom);
		logging::info ("Running Domain : ", name);
		virDomainFree (dom);
	    }

	    free (domains);
	}
	
	LibvirtVM & LibvirtClient::provision (LibvirtVM & vm, const std::filesystem::path & path) {
	    auto vPath = path / ("v" + vm.id ());
	    
	    // Prepare the different file required for the VM booting
	    this-> createDirAndVMFile (vm, vPath);

	    // install the VM on the host using virsh
	    this-> installVM (vm, vPath);

	    // wait the ip of the VM
	    this-> waitIpVM (vm, vPath);

	    logging::success ("VM is ready at ip : ", vm.ip ());
	    
	    return vm;
	}

	void LibvirtClient::createDirAndVMFile (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    auto qcowFile = vm.qcow ();	    
	    auto osPath = vPath / ("v" + vm.id () + ".qcow2");
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
	    ss << "  shell: /bin/bash" << std::endl;
	    ss << "  ssh-authorized-keys: [" << vm.pubKey () << "]" << std::endl;
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
	    auto proc = concurrency::SubProcess ("qemu-img", {"resize", "v" + vm.id () + ".qcow2", ss.str ()}, vPath.c_str ());
	    proc.start ();
	    if (proc.wait () != 0) {
		std::cout << "ERROR : " << proc.stderr ().read () << std::endl;
		std::cout << "OUT : " << proc.stdout ().read () << std::endl;
	    }
	}

	void LibvirtClient::prepareImage (const LibvirtVM & vm, const std::filesystem::path & vPath) const {
	    auto proc = concurrency::SubProcess ("virt-sysprep", {"-a", "v" + vm.id () + ".qcow2"}, vPath);
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
	    
	    auto proc = concurrency::SubProcess ("virt-install", {"--import", "--name", "v" + vm.id (),
								  "--ram", mem.str (),
								  "--vcpu", cpu.str (),
								  "--disk", "v" + vm.id () + ".qcow2,format=qcow2,bus=virtio",
								  "--disk", "user.iso,device=cdrom",
								  "--network", "bridge=virbr0,model=virtio" ,
								  "--os-type", "linux",
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
	    for (;;) { // retreive the mac address of the vm
		auto proc = concurrency::SubProcess ("virsh", {"domiflist", "v" + vm.id ()}, path);
		proc.start ();
		if (proc.wait () != 0) {
		    timer.sleep (0.5);
		    continue;
		}
		
		mac = this-> parseMacAddress (proc.stdout ().read ());
		if (mac != "") break;
	    }

	    for (;;) { // retreive the ip address of the vm from the mac address
		std::ifstream f ("/var/lib/libvirt/dnsmasq/virbr0.status");
		std::stringstream ss;
		ss << f.rdbuf ();
		auto j = json::parse (ss.str ());
		for (auto & it : j) {
		    if (it["mac-address"] == mac) {
			ip = it ["ip-address"];
			break;
		    }
		}
		if (ip != "") break;
	    }

	    vm.mac (mac).ip (ip);
	}

	std::string LibvirtClient::parseMacAddress (const std::string & output) const {
	    std::stringstream ss;
	    ss << output;
	    int index = 0;
	    for (;;) {
		std::string word;
		ss >> word;
		if (word == "MAC") break;
		else if (word == "") return "";
		else index += 1;
	    }

	    for (;;) {
		std::string word;
		ss >> word;
		if (word.rfind ("-----") != std::string::npos) break;
		else if (word == "") return "";		    
	    }
	    
	    for (int i = 0; i < index ; i++) {
		std::string word;
		ss >> word;		
	    }

	    std::string mac;
	    ss >> mac;

	    return mac;
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
