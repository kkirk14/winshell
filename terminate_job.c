
/**
 * terminate_job.c
 */



#include <windows.h>
#include "_winshell_private.h"



/**
 * terminate_job
 * 
 * Terminates a job, probably before it's finished.
 * Frees any resources for the job structs.
 * Terminates all active processes (with TerminateProcess).
 * 
 * job: Contains information on the job that failed - was in the process of 
 *      being built.
 * 
 * Return Value: Returns TRUE on success, FALSE on failure.
 */
BOOL terminate_job(job_t *job) {

    BOOL bool_rc;
    DWORD dw_rc, exit_code;

    // kill all processes
    for (int i = 0; i < job->n_procs_alive; i++) {
        HANDLE proc_h = job->proc_hs[i];
        DWORD pid = GetProcessId(proc_h);
        bool_rc = TerminateProcess(proc_h, 1);
        if (!bool_rc) {
            print_err(L"terminate_job -> TerminateProcess");
            continue;
        }
        do {
            dw_rc = WaitForSingleObject(proc_h, INFINITE);
            if (dw_rc == WAIT_FAILED) {
                print_err(L"terminate_job -> WaitForSingleObject");
                break;
            }
            bool_rc = GetExitCodeProcess(proc_h, &exit_code);
        } while(bool_rc && exit_code == STILL_ACTIVE);

        CloseHandle(proc_h);
        // Try to remove process HANDLE from wait_handles (may already be removed)
        handle_arr_remove(wait_handles, &n_wait_handles, proc_h);
    }

    // Free job resources
    free(job->proc_hs);
    free(job->cmdline);
    job->status = GARBAGE;

    return TRUE;
}