#include <iostream>
#include <monitor/libvirt/client.hh>

using namespace monitor::libvirt;
using namespace monitor;

int main () {
    LibvirtClient client;
    client.connect ();

    client.printDomains ();

    LibvirtVM vm = LibvirtVM ("v0")
	.pubKey ("/home/emile/Documents/lille/frequency/node_manager/keys/key.pub")
	.qcow ("/home/emile/.qcow2/ubuntu-20.04.qcow2");
    
    try {
	client.provision (vm);
    } catch (LibvirtError err) {
	err.print ();
    }

    return 0;
}
