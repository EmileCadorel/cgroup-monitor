#include "daemon.hh"


namespace server {

    Daemon::Daemon () :
	_controller (this-> _libvirt),
	_vms (this-> _libvirt, this-> _controller)
    {}

    void Daemon::start () {
	this-> _libvirt.setKeyPath ("/etc/dio/keys");
	this-> _libvirt.connect ();
	this-> _vms.start ();
	//this-> _controller.start ();
    }

    void Daemon::join () {
	this-> _vms.join ();
	//this-> _controller.kill ();
    }

    void Daemon::kill () {
	this-> _vms.kill ();
	//this-> _controller.kill ();	
	this-> _libvirt.killAllRunningDomains ();
	this-> _libvirt.disconnect ();
    }

}
