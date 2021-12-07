#pragma once
#include <monitor/net/addr.hh>
#include <string>


namespace monitor {

    namespace net {


	class TcpStream {

	    int _sockfd = 0;

	    SockAddrV4 _addr;

	public :

	    TcpStream (int sock, SockAddrV4 addr);

	    /**
	     * Send a message through the stream
	     * @returns: true, iif the send was successful
	     */
	    bool send (const std::string & msg);

	    /**
	     * @returns: the address of the stream
	     */
	    SockAddrV4 addr () const;
	    
	    /**
	     * Receive a message from the stream
	     */
	    std::string receive (unsigned long size);
	    

	    void close ();

	};
	
	
    }    

}
