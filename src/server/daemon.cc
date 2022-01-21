#include "daemon.hh"


namespace server {

    Daemon::Daemon () :
	_vms (this-> _libvirt),
	_controller (this-> _libvirt)
    {}

    void Daemon::start () {
	this-> _libvirt.connect ();
	this-> _vms.start ();
	this-> _controller.start ();
    }

    void Daemon::join () {
	this-> _vms.join ();
	this-> _controller.kill ();
    }

    void Daemon::kill () {
	this-> _vms.kill ();
	this-> _controller.kill ();	
	this-> _libvirt.killAllRunningDomains ();
	this-> _libvirt.disconnect ();
    }

}
