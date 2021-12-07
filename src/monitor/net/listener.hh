#pragma once

#include <monitor/net/addr.hh>
#include <monitor/net/stream.hh>


namespace monitor {

    namespace net {


	class TcpListener {

	    int _sockfd = 0;

	    SockAddrV4 _addr;

	    /// The binded port
	    short _port;

	public :
	    
	    TcpListener (SockAddrV4 addr);

	    /**
	     * Start the tcp listener
	     */
	    void start ();

	    /**
	     * Accept incoming connexions
	     */
	    TcpStream accept ();

	    /**
	     * Close the tcp listener
	     */	    
	    void close ();
	    
	    /**
	     * @returns: the binded port
	     */
	    unsigned short port () const;

	private : 

	    void bind ();


	    
	};
	

    }    

}
