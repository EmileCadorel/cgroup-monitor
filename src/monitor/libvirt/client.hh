#pragma once
#include <libvirt/libvirt.h>
#include <monitor/libvirt/error.hh>
#include <monitor/libvirt/vm.hh>
#include <monitor/concurrency/mutex.hh>
#include <filesystem>
#include <map>
#include <monitor/foreign/tinyxml2.h>

namespace monitor {
    
    namespace libvirt {
	
	/**
	 * The libvirt client is used to retreive VM domains
	 * It handles the connection to the qemu system
	 * @good_practice: use only one client for the whole program, maybe this should be a singleton
	 */
	class LibvirtClient {
	    
	    /// The connection to the libvirt server
	    virConnectPtr _conn;

	    /// The uri of the qemu system
	    const char * _uri;

	    /// The mutex used to synchronized the VM provisionning/killing that cannot be done in parallel
	    concurrency::mutex _mutex;

	    /// The list of running VMs
	    std::map <std::string, LibvirtVM> _running;
	    
	public:

	    friend LibvirtVM;
	    
	    /**
	     * @params: 
	     *   - uri: the uri of the qemu system
	     */
	    LibvirtClient (const char * uri = "qemu:///system");

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================          CONNECTION          =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Connect the client
	     * @info: this is a pretty heavy function, it should be done once at the beginning of the program
	     * @throws: 
	     *   - LibvirtError: if the connection failed
	     */
	    void connect ();

	    /**
	     * Close the connection of the client
	     * @info: it can be reconnected afterward
	     */
	    void disconnect ();

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================            DOMAIN            =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    /**
	     * @TEMP
	     * @DEBUG
	     * Print the list of domains on the machine
	     */	    
	    void printDomains () const;

	    /**
	     * Kill all the domains that are running on this machine
	     */
	    void killAllRunningDomains ();
	    
	    /**
	     * Retrieve the XML of the VM vm
	     * @warning: the doc must be freed by hand
	     * @params: 
	     *   - vm: the vm to read (must be provisionned)
	     * @returns: the xml, empty doc if the vm is down
	     */
	    tinyxml2::XMLDocument * getXML (const LibvirtVM & vm) const;

	    /**
	     * @returns: true iif the VM 'name' is running
	     */
	    bool hasVM (const std::string & name);

	    /**
	     * @returns: the VM named 'name'
	     * @throws: 
	     *    - LibvirtError: if the vm does not exists	      
	     */
	    LibvirtVM & getVM (const std::string & name);

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           NETWORK            =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    /**
	     * Nat enabling for the VM
	     * @params: 
	     *  - vm: the vm to nat
	     *  - host: the input port (host port)
	     *  - guest: the output port (guest port)
	     */
	    LibvirtVM & openNat (LibvirtVM & vm, int host, int guest);
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================             DTOR             =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    /**
	     * @alias: this-> disconnect ()
	     */
	    ~LibvirtClient ();


	private :

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================        PROVISIONNING         =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    /**
	     * Create the disk and user data for the VM before provisionning
	     * @params: 
	     *   - vm: the information about the VM to provision
	     *   - destPath: the path of the provisionning
	     * @throws:
	     *   - LibvirtError: if there is an error during the process
	     */
	    void createDirAndVMFile (const LibvirtVM & vm, const std::filesystem::path & destPath) const;

	    /**
	     * Create the user data file used to configure the VM provisionning
	     * User data are the name of the user, and the keyfile used to connect to the VM through ssh
	     * @params: 
	     *   - vm: the vm to being provisionned
	     *   - path: the directory path of the provisionning
	     */
	    void createUserData (const LibvirtVM & vm, const std::filesystem::path & path) const;
	    
	    /**
	     * Create the meta data file used to configure the VM provisionning
	     * Meta data is some data info about the vm (instance name, and hostname)
	     * @params: 
	     *   - vm: the vm to being provisionned
	     *   - path: the directory path of the provisionning
	     */
	    void createMetaData (const LibvirtVM & vm, const std::filesystem::path & path) const;

	    /**
	     * Create the ISO file for the meta and user data of the VM before provisionning
	     * @params: 
	     *   - vm: the vm to being provisionned
	     *   - path: the directory path of the provisionning
	     */
	    void createISOData (const LibvirtVM & vm, const std::filesystem::path & path) const;

	    /**
	     * Resize the image file of the VM
	     * @params: 
	     *   - vm: the vm to being provisionned
	     *   - path: the directory path of the provisionning
	     */
	    void resizeImage (const LibvirtVM & vm, const std::filesystem::path & path) const;

	    /**
	     * Prepare the image disk before provisionning
	     * @params: 
	     *   - vm: the vm to being provisionned
	     *   - path: the directory path of the provisionning
	     */
	    void prepareImage (const LibvirtVM & vm, const std::filesystem::path & path) const;

	    /**
	     * Install the VM using virsh
	     * @synchronized
	     * @params: 
	     *   - vm: the vm to being provisionned
	     *   - path: the directory path of the provisionning
	     */
	    void installVM (const LibvirtVM & vm, const std::filesystem::path & path);

	    /**
	     * Wait until the ip address of the VM is available
	     * @params: 
	     *   - vm: the vm to being provisionned
	     *   - path: the directory path of the provisionning
	     */
	    void waitIpVM (LibvirtVM & vm, const std::filesystem::path & path);

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           KILLING            =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Kill the libvirt domain (forcing its shutdown and destroying)
	     * @params: 
	     *    - vm: the vm to kill
	     */
	    void killDomain (const LibvirtVM & vm) const;

	    /**
	     * Delete the file created during the provisionning
	     * @params: 
	     *   - vm: the vm being killed
	     *   - path: the path of the vm directory
	     */
	    void deleteDirAndVMFile (const LibvirtVM & vm, const std::filesystem::path & path) const;	    
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================            DOMAIN            =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    /**
	     * Retreive the domain of the VM
	     * @warning: the return value must be freed
	     * @info: to avoid any problem, only LibvirtVM should call this function
	     * @params: 
	     *    - name: the name of the VM
	     * @returns: the domain of the VM
	     */
	    virDomainPtr retreiveDomain (const std::string & name);

	    /**
	     * Provision a new VM
	     * @params: 
	     *   - vm: the vm to provision
	     *   - destPath: the destination path of the VM provisionning
	     * @returns: the provisionned VM
	     * @throws:
	     *   - LibvirtError: if the provisionning failed
	     */
	    LibvirtVM & provision (LibvirtVM & vm, const std::filesystem::path & destPath = "/tmp/");	    

	    /**
	     * Kill the VM that is running
	     * @info: delete the associated drives
	     * @params: 
	     *   - vm: the vm to kill
	     *   - path: the location of the installed VM
	     */
	    LibvirtVM & kill (LibvirtVM & vm, const std::filesystem::path & path = "/tmp/");

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           NETWORK            =========================
	     * ================================================================================
	     * ================================================================================
	     */


	    /**
	     * Enable nat routing
	     */
	    void enableNatRouting () const;
	    
	};
    }
    
}
