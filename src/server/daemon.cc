#include "daemon.hh"


namespace server {

    Daemon::Daemon () {
    }

    void Daemon::start () {
	this-> _vms.start ();
    }

    void Daemon::join () {
	this-> _vms.join ();
    }

    void Daemon::kill () {
	this-> _vms.kill ();
    }

}
