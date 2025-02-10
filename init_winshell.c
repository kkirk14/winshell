
/**
 * init_winshell.c
 */



#include <windows.h>
#include <inttypes.h>
#include "_winshell_private.h"



/**
 * init_winshell
 * 
 * Does some initialization tasks that must be done before the main shell loop
 * is run:
 *  - initializes the jobs array
 */
int init_winshell() {
    
    // Initialize jobs array
    for (int32_t i = 0; i < MAX_JOBS; i++) {
        jobs[i].jid = i;
        jobs[i].status = GARBAGE;
    }

    // Allocate and initialize the wait_handles array
    cap_wait_handles = 1 + MAX_JOBS * 3;
    n_wait_handles = 1;
    wait_handles = malloc(cap_wait_handles * sizeof(HANDLE));
    wait_handles[0] = GetStdHandle(STD_INPUT_HANDLE);
}
