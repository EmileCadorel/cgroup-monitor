#pragma once
#include <monitor/net/addr.hh>
#include <string>


namespace monitor {

    namespace net {

	class TcpListener;
	class TcpStream {

	    /// The socket of the stream
	    int _sockfd = 0;

	    /// The address connected or not depending on _sockfd
	    SockAddrV4 _addr;

	private :

	    friend TcpListener;
	    
	    /**
	     * Construction of a stream from an already connected socket
	     * @warning: should be used only by the listener
	     */
	    TcpStream (int sock, SockAddrV4 addr);
	    
	public :

	    /**
	     * Construction of a stream for a given tcp address
	     * @warning: does not connect the stream (cf. this-> connect);
	     */
	    TcpStream (SockAddrV4 addr);
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           GETTERS            =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    /**
	     * @returns: the address of the stream
	     */
	    SockAddrV4 addr () const;

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           CONNECT            =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Connect the stream as a client
	     * @info: use the addr given in the constructor
	     * @warning: close the current stream if connected to something
	     */
	    void connect ();	    
	    
	    /**
	     * Close the stream if connected
	     */
	    void close ();

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================        SEND / RECEIVE        =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Send a int into the stream
	     * @params: 
	     *   - i: the int to send
	     */
	    bool sendInt (unsigned long i);
	    
	    
	    /**
	     * Send a message through the stream
	     * @returns: true, iif the send was successful
	     */
	    bool send (const std::string & msg);
	    
	    /**
	     * Receive a message from the stream
	     * @params: 
	     *   - size: the size of the string to receive
	     */
	    std::string receive (unsigned long size);

	    /**
	     * Receive an int from the stream
	     */
	    unsigned long receiveInt ();


	};
	
	
    }    

}
