#include <iostream>
#include "daemon.hh"
#include <signal.h>

server::Daemon dam;

void terminateSigHandler (int) {
    dam.kill ();
    exit (0);
}

int main () {
    signal(SIGINT, &terminateSigHandler);
    
    dam.start ();
    dam.join ();
}
