
/**
 * append_to_wait_handles.c
 */



#include <windows.h>



/**
 * append_to_wait_handles
 * 
 * 
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
        &handles[n_wait_handles], 
        handles, 
        len_handles * sizeof(HANDLE)
    );
    n_wait_handles += len_handles;

    return TRUE;
}