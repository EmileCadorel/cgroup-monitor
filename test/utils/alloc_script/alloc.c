#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.hh"
#include "proc.hh"
#include <iostream>

void printSwapUsage () {
    auto proc = SubProcess ("free", {"-m"}, ".");
    proc.start ();
    proc.wait ();
    std::cout << proc.stdout ().read () << std::endl;
}

int main (int argc, char ** argv) {
    if (argc < 2) {
	fprintf (stderr, "./usage size\n");
	return -1;
    }
    
    int nb = atoi (argv [1]);
    
    int * c = (int*) malloc ((long) (1024*1024) * nb); // 4GB
    timer t;
    for (;;) {
	t.sleep (1.0f);
	t.reset ();
	memset (c, 0, (long) (1024*1024) * nb);
	printf ("Done in %fs \n", t.time_since_start ());
	std::cout << "--------------------------------------" << std::endl;
	printSwapUsage ();
	std::cout << "--------------------------------------" << std::endl;
    }
}
