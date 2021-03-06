#include <monitor/net/stream.hh>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <monitor/utils/log.hh>

namespace monitor {

    namespace net {

	TcpStream::TcpStream (int sock, SockAddrV4 addr) :
	    _sockfd (sock),
	    _addr (addr)
	{
	}

	TcpStream::TcpStream (SockAddrV4 addr) :
	    _sockfd (0),
	    _addr (addr)
	{
	}

	void TcpStream::connect () {
	    this-> close ();
	    this-> _sockfd = socket (AF_INET, SOCK_STREAM, 0);
	    if (this-> _sockfd == -1) {
		std::cout << "Error creating socket" << std::endl;
		exit (-1);
	    }

	    sockaddr_in sin = { 0 };
	    sin.sin_addr.s_addr = this-> _addr.ip ().toN ();
	    sin.sin_port = htons(this-> _addr.port ());
	    sin.sin_family = AF_INET;

	    if (::connect (this-> _sockfd, (sockaddr*) &sin, sizeof (sockaddr_in)) != 0) {
		std::cout << "Error connecting socket" << std::endl;
		exit (-1);
	    }	    
	}
	
	
	bool TcpStream::sendInt (unsigned long i) {
	    if (this-> _sockfd != 0) {
		if (write (this-> _sockfd, &i, sizeof (unsigned long)) == -1) {
		    this-> _sockfd = 0;
		    return false;
		}
		return true;
	    }

	    return false;
	}

	bool TcpStream::send (const std::string & msg) {
	    if (this-> _sockfd != 0) {
		if (write (this-> _sockfd, msg.c_str (), msg.length () * sizeof (char)) == -1) {
		    this-> _sockfd = 0;
		    return false;
		}
		return true;
	    }
	    return false;
	}

	std::string TcpStream::receive (unsigned long len) {
	    if (this-> _sockfd != 0) {
		auto buf = new char [len + 1];

		auto r = read (this-> _sockfd, buf, len * sizeof (char));
		if (r == -1) {
		    this-> _sockfd = 0;
		} else buf [r] = '\0';
		
		auto ret = std::string (buf);
		delete buf;

		return ret;
	    }

	    return "";	    
	}

	unsigned long TcpStream::receiveInt () {
	    unsigned long res = 0;
	    if (this-> _sockfd != 0) {
		auto r = read (this-> _sockfd, &res, sizeof (unsigned long));
		if (r == -1) {
		    this-> _sockfd = 0;
		}
	    }

	    return res;
	}


	SockAddrV4 TcpStream::addr () const {
	    return this-> _addr;
	}
	
	void TcpStream::close  () {
	    if (this-> _sockfd != 0) {
		::close (this-> _sockfd);
		this-> _sockfd = 0;
	    }
	}
	
    }

}
