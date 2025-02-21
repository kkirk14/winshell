
/**
 * init_winshell.c
 */



#ifndef UNICODE
#define UNICODE 
#endif



#include <windows.h>
#include <synchapi.h>
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

    // Initialize cmdline_data synchronization objects
    cmdline_lock = CreateMutexW(NULL, FALSE, NULL);
    if (cmdline_lock == NULL) {
        print_err(L"cmdline_lock CreateMutexW");
        return -1;
    }
    cmdline_available_e = CreateEventW(
        NULL, 
        FALSE, // This is autoreset event (Wait resets signal state)
        FALSE, 
        NULL
    );
    if (cmdline_available_e == NULL) {
        print_err(L"cmdline_available_e CreateEventW");
        return -1;
    }
    cmdline_consumed_e = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (cmdline_consumed_e == NULL) {
        print_err(L"init_winshell -> CreateEventW cmdline_consumed_e");
        ExitProcess(1);
    }

    // Initialize exited synchronization objects
    exited_lock = CreateMutexW(NULL, FALSE, NULL);
    if (exited_lock == NULL) {
        print_err(L"init_winshell -> CreateMutexW exited_lock");
        return -1;
    }
    exited_e = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (exited_lock == NULL) {
        print_err(L"init_winshell -> CreateEventW exited_e");
        return -1;
    }

    // Allocate and initialize the wait_handles array
    cap_wait_handles = 1 + MAX_JOBS * 3;
    n_wait_handles = 1;
    wait_handles = malloc(cap_wait_handles * sizeof(HANDLE));
    wait_handles[0] = cmdline_available_e;

    return 0;
}
