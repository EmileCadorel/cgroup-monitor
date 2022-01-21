#pragma once
#include <monitor/net/_.hh>
#include <monitor/concurrency/_.hh>
#include <monitor/libvirt/_.hh>
#include <filesystem>

namespace server {    
    
    /**
     * This class is used to manage the running VMs on the host
     */
    class VMServer {

	/// The tcp listener accepting connections
	monitor::net::TcpListener _listener;

	/// the id of the thread managing the tcp server
	monitor::concurrency::thread _loopTh;
	
	/// The libvirt connection
	monitor::libvirt::LibvirtClient & _libvirt;

    public:

	/**
	 * @params: 
	 *  - the libvirt client that communicate with libvirt
	 */
	VMServer (monitor::libvirt::LibvirtClient & libvirt);
	
	/**
	 * Start the server thread waiting for new connections
	 */
	void start ();

	/**
	 * Wait for the end of the server 
	 */
	void join ();

	/**
	 * Force the killing of the server
	 */
	void kill ();

    private :

	/**
	 * Main loop accepting new connections
	 */
	void acceptingLoop (monitor::concurrency::thread t);

	/**
	 * Treat the request of a new client
	 */
	void acceptClient (monitor::net::TcpStream & client);

	/**
	 * Treat a provision request
	 */
	void treatProvision (monitor::net::TcpStream & client);

	/**
	 * Treat a kill request
	 */
	void treatKill (monitor::net::TcpStream & client);

	/**
	 * Treat a ip request
	 */
	void treatIp (monitor::net::TcpStream & client);

	/**
	 * Treat a nat request
	 */
	void treatNat (monitor::net::TcpStream & client);
	
	/**
	 * Create the configuration file, in order to access the server from outside process
	 * @params: 
	 *    - path: the directory in which dump the configuration
	 */
	void dumpConfig (const std::filesystem::path & path = "/var/lib/dio") const;
    };

    
}
