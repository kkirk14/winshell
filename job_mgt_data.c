
/**
 * job_mgt_data.c
 *
 * Declarations for various static-duration variables for managing and keeping
 * track of jobs and processes.
 */



#include <windows.h>
#include "_winshell_private.h"



/* wait_handles: Points to heap-allocated array of handles. This is the array
                 that will be used in the shell loop's WaitForMultipleObjects
                 call - contains stdin and all running processes of all jobs.
                 Note: When a process terminates it must be removed from this
                       arr. */
HANDLE *wait_handles;



/* n_wait_handles: Current usage of the wait_handles array. */
int32_t n_wait_handles;



/* cap_wait_handles: Current capacity of the wait_handles array. */
int32_t cap_wait_handles;



/* jobs: Array of job_t's. */
job_t jobs[MAX_JOBS];
