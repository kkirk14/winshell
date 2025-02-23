
/**
 * kill_builtin.c
 */



#include <windows.h>
#include <inttypes.h>
#include <wctype.h>
#include <iso646.h>
#include <shlwapi.h>
#include "_winshell_private.h"



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
