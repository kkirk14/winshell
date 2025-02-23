
/**
 * job_to_str.c
 */



#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "_winshell_private.h"



#define INIT_CAP 2



/**
 * job_status_to_str
 * 
 * status:
 * 
 * Return Value: Returns a pointer to a data section string containing
 *               the job status.
 */
static const WCHAR *job_status_to_str(job_status_t status) {
    switch(status) {
    case RUNNING:
        return L"RUNNING";
        break;
    case TERMINATED:
        return L"TERMINATED";
        break;
    case GARBAGE:
        return L"GARBAGE";
        break;
    default: // This should never be reached
        return L"ERROR";
        break;
    }
}



/**
 * job_to_str
 * 
 * Creates a string description for a given job.
 * 
 * job: job_t object to describe.
 * 
 * Return Value: Returns a pointer to a heap-allocated NULL-terminated string 
 *               containing the description for the job. Must be freed with 
 *               free().
 *               Returns NULL on error.
 */
WCHAR *job_to_str(const job_t *job) {
    
    WCHAR *job_desc = malloc((INIT_CAP + 1) * sizeof(WCHAR));
    if (job_desc == NULL) 
        return NULL;
    
    int len = snwprintf(
        job_desc,
        INIT_CAP,
        L"[%d] %s\t\t%s",
        job->jid,
        job_status_to_str(job->status),
        job->cmdline
    );

    if (len > INIT_CAP) {
        job_desc = realloc(job_desc, (len + 1) * sizeof(WCHAR));
        if (job_desc == NULL) {
            free(job_desc);
            return NULL;
        }
        len = snwprintf(
            job_desc,
            len + 1,
            L"[%d] %s\t\t%s",
            job->jid,
            job_status_to_str(job->status),
            job->cmdline
        );
    }

    return job_desc;
}
