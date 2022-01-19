#pragma once

#include <filesystem>
#include <string>

namespace monitor {

    namespace libvirt {

	/**
	 * A libvirt VM is a connected domain to a VM
	 * It is used to get the usage information of the VM, and apply some management on it (e.g. cpu capping, memory capping)
	 */
	class LibvirtVM {

	    /// The id of the vm
	    std::string _id;

	    /// The name of the user
	    std::string _userName;

	    /// The content of the public key
	    std::string _pubKey;
	    
	    /// The path of the img to use for the vm
	    std::filesystem::path _qcow;

	    /// The size of the disk in MB
	    int _disk;

	    /// The number of vcpus of the VM
	    int _vcpus;

	    /// The size of the memory of the vm in MB
	    int _mem;

	    /// The ip address of the VM
	    std::string _ip;

	    /// The mac address of the VM
	    std::string _mac;
	    
	public:

	    /**
	     * @params: 
	     *  - name: the id of the VM
	     */
	    LibvirtVM (const std::string & name);
	    	    
	    /**
	     * @returns: the name of the VM
	     */
	    const std::string & id () const;

	    /**
	     * Set the path of the qcow image to use
	     * @params: 
	     *   - path: the path of the image
	     * @returns: *this
	     */
	    LibvirtVM & qcow (const std::filesystem::path & path);
	    
	    /**
	     * @returns: the path to the qcow file to use
	     */
	    const std::filesystem::path & qcow () const;

	    /**
	     * Set the content of the public for the VM (default is ${HOME}/.ssh/id_rsa.pub)
	     * @params:
	     *    - path: the path of the key file
	     * @returns: *this
	     */
	    LibvirtVM & pubKey (const std::filesystem::path & path);

	    /**
	     * @returns: the content of the public key of the vm
	     */
	    const std::string & pubKey () const;
	    
	    /**
	     * Set the name of the user of the vm (default is phil)
	     * @params:
	     *    - name: the name of the user
	     * @returns: *this
	     */
	    LibvirtVM & user (const std::string & name);

	    /**
	     * @returns: the name of the user
	     */
	    const std::string & user () const;

	    /**
	     * Set the size of the disk of the VM (default is 10_000 MB)
	     * @params: 
	     *   - size: in MB
	     */
	    LibvirtVM & disk (int size);

	    /**
	     * @returns: the size of the disk of the VM in MB
	     */
	    int disk () const;


	    /**
	     * Set the size of the memory of the VM (default is 2G)
	     * @params: 
	     *   - size: in MB
	     */
	    LibvirtVM & memory (int size);

	    /**
	     * @returns: the size of the memory of the VM	      
	     */
	    int memory () const;

	    /**
	     * Set the number of vcpus of the VM (default is 1)
	     * @params: 
	     *    - nb: the number of vcpu of the vm
	     */
	    LibvirtVM & vcpus (int nb);

	    /**
	     * @returns: the number of vcpus of the VM
	     */
	    int vcpus () const;

	    /**
	     * Set the mac address of the VM
	     * @params: 
	     *   - value: the value of the mac address
	     */
	    LibvirtVM & mac (const std::string & value);

	    /**
	     * @returns: the mac address of the VM
	     */
	    const std::string & mac () const;

	    /**
	     * Set the ip address of the VM
	     * @params: 
	     *    - value: the value of the address
	     */
	    LibvirtVM & ip (const std::string & value);

	    /**
	     * @returns: the ip address of the VM
	     */
	    const std::string & ip () const;
	};
	
    }    

}
