#include <monitor/net/stream.hh>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

namespace monitor {

    namespace net {

	TcpStream::TcpStream (int sock, SockAddrV4 addr) :
	    _sockfd (sock),
	    _addr (addr)
	{
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
		auto buf = new char [len];

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
