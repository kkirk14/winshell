
/**
 * cd_builtin.c
 */



#include <windows.h>
#include <iso646.h>
#include "_winshell_private.h"



/**
 * cd_builtin
 * 
 * Changes the current working directory.
 * 
 * parsed_proc: Contains information about command line that called this 
 *              builtin. The directory to move to will be extracted from this.
 * startup_info: Contains I/O redirection information. Not used.
 * 
 * Return Value: Returns TRUE on success. Returns FALSE on failure.
 */
BOOL cd_builtin(parsed_process_t *parsed_proc, 
                STARTUPINFO *startup_info) {

    BOOL bool_rc;
    WCHAR my_new_dir[MAX_PATH + 1];

    const WCHAR *cd_p = skip_whitespace(parsed_proc->cmd_line);
    const WCHAR *new_dir_p = skip_whitespace(arg_end(cd_p));
    const WCHAR *new_dir_end_p = arg_end(new_dir_p);

    // Directory not provided
    if (new_dir_end_p == new_dir_p) {
        bool_rc = WriteConsoleW(
            GetStdHandle(STD_ERROR_HANDLE),
            L"directory not provided\n",
            wcslen(L"directory not provided\n"),
            NULL,
            NULL
        );
        if (not bool_rc) {
            print_err(L"cd_builtin -> WriteConsoleW");
            return FALSE;
        }
        return FALSE;
    }

    DWORD len_new_dir = (DWORD)(new_dir_end_p - new_dir_p);

    // Directory is too long
    if (len_new_dir > MAX_PATH) {
        bool_rc = WriteConsoleW(
            GetStdHandle(STD_ERROR_HANDLE),
            L"directory too long\n",
            wcslen(L"directory too long\n"),
            NULL,
            NULL
        );
        if (not bool_rc) {
            print_err(L"cd_builtin -> WriteConsoleW");
            return FALSE;
        }
        return FALSE;
    }
    
    // Copy and add NULL terminator
    memcpy(
        my_new_dir, 
        new_dir_p, 
        len_new_dir * sizeof(WCHAR)
    );
    my_new_dir[len_new_dir] = L'\0';
    
    bool_rc = SetCurrentDirectoryW(my_new_dir);
    if (not bool_rc 
         and (GetLastError() == ERROR_INVALID_NAME 
               or GetLastError() == ERROR_FILE_NOT_FOUND)) {
        bool_rc = WriteConsoleW(
            GetStdHandle(STD_ERROR_HANDLE),
            L"Directory not found\n",
            wcslen(L"Directory not found\n"),
            NULL,
            NULL
        );
        if (not bool_rc) {
            print_err(L"cd_builtin -> WriteConsoleW");
            return FALSE;
        }
    }
    else if (not bool_rc) {
        print_err(L"cd_builtin -> SetCurrentDirectoryW");
        return FALSE;
    }

    return TRUE;
}
