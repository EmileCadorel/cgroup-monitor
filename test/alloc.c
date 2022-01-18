#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.hh"

int main () {
    int * c = (int*) malloc ((long) (1024*1024*1024) * 2); // 4GB
    timer t;
    for (;;) {
	t.sleep (1.0f);
	t.reset ();
	printf ("Memset : ");
	fflush (stdout);
	memset (c, 0, (long) (1024*1024*1024) * 2);
	printf ("Done in %fs \n", t.time_since_start ());
    }
}
