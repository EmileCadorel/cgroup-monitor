#pragma once

#include <vector>
#include <map>
#include <filesystem>
#include <monitor/cgroup/info.hh>
#include <monitor/cgroup/vminfo.hh>
#include <monitor/concurrency/timer.hh>
#include <monitor/concurrency/thread.hh>
#include <monitor/concurrency/mutex.hh>
#include <monitor/market.hh>
#include <monitor/net/listener.hh>
#include <monitor/utils/config.hh>
#include <chrono>

namespace monitor {
    namespace cgroup {

	class GroupManager {

	    /// The list of discovered cgroups (independent)
	    std::map <std::string, GroupInfo> _groups;

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

	    /// The thread that waits for client connections
	    concurrency::thread _th;

	    /// The format of the reports
	    std::string _format;

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
	     * Create a group manager that seek for cgroups, and update their informations
	     * @params: 
	     *  - addr: the address server sending the reports
	     *  - frameDur: the duration of a frame
	     */
	    GroupManager (net::SockAddrV4 addr, float frameDur);

	    /**
	     * Run the manager
	     */
	    void run ();

	    /**
	     * @returns: the list of VMs
	     */
	    const std::map <std::string, VMInfo> & getVms () const;

	    /**
	     * @returns: the list of cgroup that are not VMs
	     */
	    const std::map <std::string, GroupInfo> & getGroups () const;

	    /**
	     * @returns: the market of the manager
	     */
	    const Market & getMarket () const;
	    
	private :

	    /**
	     * Update the manager informations 
	     * Update cgroup list, and group infos
	     */
	    void update ();

	    /**
	     * Resursively read the filesystem directory to get all available cgroups
	     */
	    void recursiveUpdate (const std::filesystem::path & current, std::map <std::string, GroupInfo> & result, std::map <std::string, VMInfo> & vmRes);

	    /**
	     * Create a VMInfo from a path
	     */
	    VMInfo updateVM (const std::filesystem::path & path);

	    /**
	     * Wait the correct amount of time, to respect the durations between to frames
	     */
	    void waitFrame ();

	    /**
	     * The loop running the tcp server, waiting for incoming connections
	     */
	    void tcpLoop (concurrency::thread t);

	    /**
	     * Apply the capping of the VMs
	     */
	    void applyResult (const std::map <std::string, unsigned long> & res);

	    /**
	     * Send the report to the connected listeners
	     */
	    void sendReports (std::chrono::duration<double> diff);
	    
	};
	
    }
}
