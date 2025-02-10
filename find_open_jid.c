
/**
 * find_open_jid.c
 */



#include <windows.h>
#include <inttypes.h>
#include "_winshell_private.h"



/**
 * find_open_jid
 *
 * Finds the first jid that's open to be used for a new job.
 *
 * Return Value: Returns the jid on success.
 *               Return -1 if the jobs array is full.
 */
int32_t find_open_jid() {

    for (int jid = 0; jid < MAX_JOBS; jid++) {
        if (jobs[jid].status == GARBAGE) {
            return jid;
        }
    }

    return -1;
}
