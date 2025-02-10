
/**
 * job.h
 *
 * job_t struct defined here.
 */



#ifndef _JOB_STRUCT_H
#define _JOB_STRUCT_H



#include <windows.h>
#include <inttypes.h>
#include <stdbool.h>



/**
 * job_status_t 
 *
 * Job is RUNNING while 1 or more of its processes are alive, it becomes
 * TERMINATED once its last processes exits.
 * GARBAGE means that this job (in the job array) can be overwritten.
 */
typedef enum _job_status {
    RUNNING,
    TERMINATED,
    GARBAGE
} job_status_t;



/**
 * job_t struct
 *
 * Contains all information needed for managing the process of a single job.
 */
typedef struct _job {
    
    /* jid: Unique int users will use to identify this job. This is this job's 
            index in the job_mgt_data.c jobs array. */
    int32_t jid;

    /* status: RUNNING while >= 1 processes alive, TERMINATED once all
               processes dead. We only keep the job around for telling user
               job terminated on next "jobs" call. */
    job_status_t status;

    /* foreground: Is this job a foreground job? */
    BOOL is_foreground;

    /* n_procs_alive: Number of processes still running - this is the size of 
                      the handles array. */
    int32_t n_procs_alive;

    /* handles: Points to heap-allocated array that has the handles of all 
                running processes. 
                Note: Remove processes from this arr when it terminates. */
    HANDLE *proc_hs;

    /* cmdline: Points to heap-allocated string of the command that spawned 
                this job. We only keep this around for printing on "jobs" call. */
    wchar_t *cmdline;

} job_t;



// ifndef _JOB_STRUCT_H
#endif
