#pragma once

#include <vector>
#include <map>
#include <filesystem>
#include <monitor/cgroup/vminfo.hh>
#include <monitor/concurrency/timer.hh>
#include <monitor/concurrency/thread.hh>
#include <monitor/concurrency/mutex.hh>
#include <monitor/cgroup/market.hh>
#include <monitor/net/listener.hh>
#include <monitor/utils/config.hh>
#include <chrono>

namespace monitor {
    namespace cgroup {

	class GroupManager {

	    /// The list of discovered cgroups (vms)
	    std::map <std::string, VMInfo> _vms;
	    
	    /// The list of clients connected to the monitor
	    std::vector <net::TcpStream> _clients;

	    /// The mutex for the listener
	    concurrency::mutex _mutex;

	    /// The timer that compute the frame durations
	    concurrency::timer _t;

	    /// The duration of a frame 
	    float _frameDur;

	    /// The duration of a frame in microseconds (to avoid float conversions, when waiting ticks)
	    long _frameDurMicro;

	    /// The market of the manager
	    Market _market;

	    /// True iif running a market
	    bool _withMarket;
	    
	    /// The address of the listener sending reports to connected clients
	    net::SockAddrV4 _addr;

	    /// The address of the listener receiving information about VMs
	    net::SockAddrV4 _clientAddr;

	    /// The thread that waits for client connections
	    concurrency::thread _th;

	    /// The tread waiting for VMs informations
	    concurrency::thread _daemon;

	    /// The format of the reports
	    std::string _format;

	    /// The maximum length of the history of the VMs
	    unsigned long _vmHistory;

	    /// The cpu maximum frequency in MHz
	    int _cpuMaxFreq;

	    /// Cgroup v2 are activated
	    bool _cgroupV2;
	    
	public :

	    /**
	     * Create a group manager from a configuration 
	     * @params: 
	     *  - cfg: the configuration of the manager
	     * @example: 
	     * =========================
	     * [monitor]
	     * addr = "127.0.0.1:8080"
	     * tick = 0.1 # seconds
	     * format = "json" # the format of the reports {"json", "toml"}
	     * =========================
	     */
	    GroupManager (utils::config::dict & cfg);
	    
	    /**
	     * Run the manager
	     */
	    void run ();

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================            GETTER            =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * @returns: the list of VMs
	     */
	    const std::map <std::string, VMInfo> & getVms () const;

	    /**
	     * @returns: the market of the manager
	     */
	    const Market & getMarket () const;
	    
	private :

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================          MONITORING          =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Update the manager informations 
	     * Update cgroup list, and group infos
	     */
	    void update ();


	    /**
	     * Wait the correct amount of time, to respect the durations between to frames
	     */
	    void waitFrame ();

	    /**
	     * Apply the capping of the VMs
	     */
	    void applyResult (const std::map <std::string, unsigned long> & res);
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================         VM SEARCHING         =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Resursively read the filesystem directory to get all available cgroups
	     */
	    void recursiveSearch (const std::filesystem::path & current, const std::string & name, unsigned long freq);

	    /**
	     * Create a VMInfo from a path
	     */
	    void addVM (const std::filesystem::path & path, const std::string & name, unsigned long freq);

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================             NET              =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * The loop running the tcp server, waiting for incoming connections
	     */
	    void reportTcpLoop (concurrency::thread t);

	    /**
	     * The loop running the daemon tcp server, waiting for incoming VM infos
	     */
	    void daemonTcpLoop (concurrency::thread t);

	    /**
	     * Receive the information about a new VM
	     */
	    void receiveVMInfo (net::TcpStream & client);
	    
	    /**
	     * Send the report to the connected listeners
	     */
	    void sendReports (std::chrono::duration<double> diff);
	    
	};
	
    }
}
