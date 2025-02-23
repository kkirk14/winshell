
/**
 * pwd_builtin.c
 */



#include <windows.h>
#include <iso646.h>
#include "_winshell_private.h"



/**
 * pwd_builtin
 * 
 * Prints the current working directory.
 * 
 * parsed_proc: Contains information parsed from the command that called this
 *              builtin. Not used.
 * startup_info: Contains redirection info. Output is written to hStdOutput.
 *
 * Return Value: Returns TRUE on success. Returns FALSE on failure.
 */
BOOL pwd_builtin(parsed_process_t *parsed_proc, 
                 STARTUPINFO *startup_info) {

    DWORD dw_rc;
    BOOL bool_rc;
    WCHAR pwd[MAX_PATH + 2]; // 1 for '\n', 1 for '\0'
    
    dw_rc = GetCurrentDirectoryW(MAX_PATH + 1, pwd);
    if (dw_rc == 0) {
        print_err(L"pwd_builtin -> GetCurrentDirectory");
        return FALSE;
    }
    pwd[dw_rc] = L'\n';
    pwd[dw_rc + 1] = L'\0'; 

    HANDLE stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout_h == INVALID_HANDLE_VALUE) {
        print_err(L"pwd_builtin -> GetStdHandle");
        return FALSE;
    }

    if (stdout_h == startup_info->hStdOutput) { // no redirection, output to stdout
        bool_rc = WriteConsoleW(
            stdout_h, 
            pwd,
            wcslen(pwd),
            NULL,
            NULL
        );
        if (not bool_rc) {
            print_err(L"pwd_builtin -> WriteConsoleW");
            return FALSE;
        }
    }
    else {
        bool_rc = WriteFile(
            startup_info->hStdOutput,
            pwd,
            wcslen(pwd) * sizeof(WCHAR),
            NULL,
            NULL
        );
        if (not bool_rc) {
            print_err(L"pwd_builtin -> WriteFile");
            return FALSE;
        }
    }
}