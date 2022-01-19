#pragma once
#include <libvirt/libvirt.h>
#include <monitor/libvirt/error.hh>
#include <monitor/libvirt/vm.hh>
#include <monitor/concurrency/mutex.hh>
#include <filesystem>

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
	    
	public:

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
	     * Parse the result of the virsh command to get the mac address
	     * @params: 
	     *   - output: the result of the command "virsh domiflist $VM"
	     * @returns: the mac address or ""
	     */
	    std::string parseMacAddress (const std::string & output) const;
	};
    }
    
}
