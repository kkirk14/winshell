
/**
 * append_to_wait_handles.c
 */



#include <windows.h>
#include <inttypes.h>
#include "_winshell_private.h"



/**
 * append_to_wait_handles
 * 
 * Appends an array of HANDLEs to the end of the job_mgt_data wait_handles
 * array.
 * 
 * handles: Array of handles to append
 * len_handles: Number of HANDLEs in the handles array
 * 
 * Return Value: Returns TRUE on success. 
 *               Returns FALSE on failure (realloc failed).
 */
BOOL append_to_wait_handles(HANDLE *handles, int32_t len_handles) {
    
    // Ensure capacity
    if (n_wait_handles + len_handles > cap_wait_handles) {
        int32_t new_cap_wait_handles = cap_wait_handles * 2;
        HANDLE *new_wait_handles = realloc(
            wait_handles,
            new_cap_wait_handles * sizeof(HANDLE)
        );
        if (new_wait_handles == NULL) {
            return FALSE;
        }    
        else {
            wait_handles = new_wait_handles;
            cap_wait_handles = new_cap_wait_handles;
        }
    }

    memmove(
        &wait_handles[n_wait_handles], 
        handles, 
        len_handles * sizeof(HANDLE)
    );
    n_wait_handles += len_handles;

    return TRUE;
}
