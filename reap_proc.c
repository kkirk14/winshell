
/**
 * reap_proc.c
 */



#include "_winshell_private.h"
#include <windows.h>



/**
 * reap_proc
 * 
 * Called when a process just terminates to remove it from the job management
 * structures.
 * Cleans up the job_t struct and its resources if this was its last process.
 * 
 * proc_h: HANDLE of the process that just terminated.
 * 
 * Return Value: Returns TRUE on sucess, FALSE on failure.
 */
BOOL reap_proc(HANDLE proc_h) {

    BOOL bool_rc;

    CloseHandle(proc_h);
    bool_rc = handle_arr_remove(wait_handles, &n_wait_handles, proc_h);
    if (!bool_rc)
        return FALSE;

    job_t *job = NULL;
    for (int jobs_i = 0; jobs_i < MAX_JOBS; jobs_i++) {

        job_t *curr_job = &jobs[jobs_i];
        if (curr_job->status != RUNNING)
            continue;

        if (handle_arr_remove(curr_job->proc_hs, 
                              &curr_job->n_procs_alive, 
                              proc_h)) {
            job = curr_job;
            break;
        }
    }

    if (job == NULL)
        return FALSE;

    if (job->n_procs_alive == 0) {
        free(job->proc_hs);
        free(job->cmdline);
        job->status = TERMINATED;
    }

    return TRUE;
}
