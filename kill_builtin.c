
/**
 * kill_builtin.c
 */



#include <windows.h>
#include <inttypes.h>
#include <wctype.h>
#include <iso646.h>
#include <shlwapi.h>
#include <stdio.h> // DELETE LATER
#include "_winshell_private.h"



/**
 * first_nonescaped_dquote
 * 
 * Searches a string for a non-escaped (\") double quote L'"'.
 * 
 * str: NULL-terminated string to be searched
 * 
 * Return Value: Returns a pointer to the next real L'"' character.
 *               Returns NULL if there are no L'"' characters.
 */
static WCHAR *first_nonescaped_dquote(const WCHAR *str) {

    // edge case: first character is dquote
    // If we don't treat this as edge case, potential invalid mem access
    if (*str == L'"') {
        return (WCHAR *)str;
    }

    const WCHAR *quote_p,
                *str_p = str + 1;

    do {
        quote_p = wcschr(str_p, L'"');
        str_p = quote_p + 1;
    } while (quote_p && *(quote_p - 1) == L'\\');
    
    return (WCHAR *)quote_p;
}



/**
 * arg_end
 * 
 * Finds the end of the given argument by searching for the first non-quoted
 * whitespace.
 * 
 * cmdline: NULL-string that contains a series of whitespace separated 
 *          arguments.
 * 
 * Return Value: Returns a pointer to the end of the first argument
 */
static WCHAR *arg_end(const WCHAR *cmdline) {

    while (*cmdline != L'\0' && !iswspace(*cmdline)) {
        if (*cmdline == L'"') {
            cmdline = first_nonescaped_dquote(cmdline + 1);
            if (cmdline == NULL)
                return NULL;
        }
        cmdline++;
    }

    return (WCHAR *)cmdline;
}



/**
 * skip_whitespace
 * 
 * Searches a string for the first non-whitespace character.
 * 
 * str: NULL-terminated string to be searched.
 * 
 * Return Value: Returns a pointer to the first non-whitespace character
 *               in str. If str is all whitespace, returns a pointer to the
 *               terminating L'\0'.
 */
static WCHAR *skip_whitespace(const WCHAR *str) {

    const WCHAR *str_p = str;
    for (str_p = str; 
         *str_p != L'\0' && iswspace(*str_p); 
         str_p++) { }
    return (WCHAR *)str_p;
}



/**
 * kill_cmdline_get_jid
 * 
 * Extracts the jid from the kill command line.
 * Fails if no jid is present (command ends or arg isn't number).
 * 
 * Return Value: On success, returns the extracted jid.
 *               Returns -1 on failure.
 */
static int32_t kill_cmdline_get_jid(const WCHAR *kill_cmdline) {
    
    const WCHAR *kill_p = skip_whitespace(kill_cmdline);
    const WCHAR *jid_p = skip_whitespace(arg_end(kill_p));
    if (not iswdigit(*jid_p)) {
        return -1;
    }
    return (int32_t)StrToIntW(jid_p);
}



/**
 * kill_builtin
 * 
 * parsed_proc: Parsed info about the command that called this builtin.
 * startup_info: Should be setup for I/O redirection. Will send output to
 *               the hStdOutput HANDLE.
 * 
 * Return Value: Returns TRUE on success, FALSE on failure.
 */
BOOL kill_builtin(parsed_process_t *parsed_proc, 
                  STARTUPINFO *startup_info) {
    
    BOOL bool_rc;

    // Find the job
    int32_t jid = kill_cmdline_get_jid(parsed_proc->cmd_line);
    if (jid < 0 or jobs[jid].status == GARBAGE) {
        return FALSE;
    }
    job_t *job = &jobs[jid];

    // Create the message string
    job->status = TERMINATED;
    WCHAR *job_str = job_to_str(job);

    // Terminate the job
    bool_rc = terminate_job(job);
    if (not bool_rc) {
        return FALSE;
    }

    // Print message
    HANDLE stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout_h == INVALID_HANDLE_VALUE) {
        print_err(L"kill_builtin -> GetStdHandle");
        return FALSE;
    }

    if (stdout_h == startup_info->hStdOutput) {
        bool_rc = WriteConsoleW(
            stdout_h, 
            job_str, 
            wcslen(job_str),
            NULL, 
            NULL
        );
        if (not bool_rc) {
            print_err(L"kill_builtin -> WriteConsoleW");
        }
        bool_rc = WriteConsoleW(stdout_h, L"\n", 1, NULL, NULL);
        if (not bool_rc) {
            print_err(L"kill_builtin -> WriteConsoleW");
        }
    }
    else {
        bool_rc = WriteFile(
            startup_info->hStdOutput,
            job_str, 
            wcslen(job_str) * sizeof(WCHAR),
            NULL,
            NULL
        );
        if (not bool_rc) {
            print_err(L"kill_builtin -> WriteFile");
        }
        bool_rc = WriteFile(
            startup_info->hStdOutput,
            L"\n",
            1 * sizeof(WCHAR),
            NULL,
            NULL
        );
        if (not bool_rc) {
            print_err(L"kill_builtin -> WriteFile");
        }
    }

    free(job_str);

    return TRUE;
}
