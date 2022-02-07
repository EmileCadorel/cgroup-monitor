#pragma once

namespace monitor {

    namespace libvirt {
	
	/**
	 * The protocol between server and client for VM provisionning
	 */
	enum VMProtocol {
	    PROVISION = 0, // Provision a new VM
	    KILL, // Kill an existing VM
	    OK, // Positive response 
	    ERR, // Error response
	    IP, // Ask or Send the ip of a VM, (different action for server or client)
	    NAT, // Ask an new port opening
	    RESET_COUNTERS, // Reset the markets counters
	};

	enum VMProtocolError {
	    ALREADY_EXISTS = 0, // VM already exists, so cannot be provisionned again
	    NOT_FOUND, // VM was not found
	    PROTOCOL, // The protocol is not respected
	};

    }

}
