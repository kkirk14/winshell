
/**
 * handle_arr_remove.c
 */



#include <windows.h>
#include <inttypes.h>



/**
 * handle_arr_remove
 * 
 * Searches for and removes the first instance of to_remove HANDLE from arr 
 * HANDLE array.
 * 
 * arr: HANDLE array to search and modify.
 * in_out_len_arr: When called this should contain the current usage of arr.
 *                 On exit this will contain the new usage of arr. 
 * to_remove: HANDLE to search for and remove.
 * 
 * Return Value: Returns TRUE if to_remove was found and removed.
 *               Returns FALSE if to_remove wasn't found.
 */
BOOL handle_arr_remove(HANDLE *arr, int32_t *in_out_len_arr, HANDLE to_remove) {
    
    int32_t len_arr = *in_out_len_arr;
    
    for (int i = 0; i < len_arr; i++) {
        
        if (arr[i] == to_remove) {
            memmove(&arr[i], &arr[i + 1], (len_arr - i - 1) * sizeof(HANDLE));
            *in_out_len_arr = len_arr - 1;
            return TRUE;
        }
    }

    return FALSE;
}
