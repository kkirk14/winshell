
/**
 * shell_loop.c
 */



#ifndef UNICODE 
#define UNICODE
#endif



#include <windows.h>
#include <wchar.h>
#include "_winshell_private.h"


/* CAP_STDIN_BUF: Capacity of the stdin_buf array. */
#define CAP_STDIN_BUF MAX_CMDLINE * 10

/* stdin_buf: Buffer will contain user input for a single job command line
              as its received between (potentially) separate calls to 
              ReadFile. */
static WCHAR stdin_buf[CAP_STDIN_BUF];

/* n_stdin_buf: Current usage of the stdin_buf buffer. */
static int32_t n_stdin_buf = 0;

static WCHAR job_cmdline[MAX_CMDLINE + 1];



/* READSTDIN_NOCMDLINE: Bytes were successfully read but a full command line 
                        hasn't formed yet. */
#define READSTDIN_NOCMDLINE 0

/* READSTDIN_JOBCMDLINE: Bytes were successfully read and a full command line
                         was formed. */
#define READSTDIN_JOBCMDLINE 1

/* READSTDIN_ERROR: An error occurred while reading. */
#define READSTDIN_ERROR -1



/**
 * read_stdin
 * 
 * Reads bytes from stdin into the stdin_buf buffer. 
 * Checks if a L'\n' character was read - if so the command line is 
 * copied into job_cmdline and those bytes are removed from stdin_buf.
 * 
 * Return Value: 
 */
static int32_t read_stdin() {

    BOOL bool_rc;
    HANDLE stdin_h = wait_handles[0];

    // call ReadFile
    DWORD bytes_read;
    WCHAR *read_into = &stdin_buf[n_stdin_buf];
    bool_rc = ReadFile(
        stdin_h,
        read_into,
        CAP_STDIN_BUF - n_stdin_buf,
        &bytes_read,
        NULL
    );
    if (!bool_rc) {
        print_err(L"read_stdin -> ReadFile");
        return READSTDIN_ERROR;
    }
    n_stdin_buf += (int32_t)bytes_read;
    
    // Check for L'\n'
    WCHAR *newline_p = wmemchr(
        read_into, 
        L'\n', 
        bytes_read / sizeof(WCHAR)
    );
    if (newline_p != NULL) { // L'\n' found
        // Copy to job_cmdline
        int32_t n_job_cmdline = newline_p - stdin_buf;
        memcpy(job_cmdline, stdin_buf, n_job_cmdline * sizeof(WCHAR));
        job_cmdline[n_job_cmdline] = L'\0';
        // Shift the rest of stdin_buf down
        memmove(
            stdin_buf, 
            newline_p + 1, 
            n_stdin_buf - n_job_cmdline - 1
        );
        return READSTDIN_JOBCMDLINE;
    }
    else { // L'\n' not found
        return READSTDIN_NOCMDLINE;
    }
}



/**
 * shell_loop
 * 
 * TODO: add description
 */
void shell_loop() {

    DWORD dw_rc;
    BOOL bool_rc;
    int32_t rc;

    while (TRUE) {

        // call WaitForMultipleObjects
        dw_rc = WaitForMultipleObjects(
            (DWORD)n_wait_handles,
            wait_handles,
            FALSE,
            INFINITE
        );
        if (dw_rc == WAIT_FAILED) {
            print_err(L"WaitForMultipleObjects");
            ExitProcess(1);
        }
        DWORD signaled_i = dw_rc - WAIT_OBJECT_0;
        HANDLE signaled = wait_handles[signaled_i];

        // Stdin was signaled
        if (signaled_i == 0) {
            rc = read_stdin();
            if (rc == READSTDIN_ERROR) {
                ExitProcess(1);
            }
            else if (rc == READSTDIN_JOBCMDLINE) {

            }
        }

        // Process has terminated
        else {
            bool_rc = reap_proc(signaled);
            if (!bool_rc) {
                fwprintf(
                    stderr, 
                    L"ERROR: reap_proc pid=%d failed (not found)\n",
                    GetProcessId(signaled)
                );
                fflush(stderr);
            }
            else { // DELETE LATER
                fwprintf(
                    stderr, 
                    L"successfully reaped pid=%d\n",
                    GetProcessId(signaled)
                );
                fflush(stderr);
            }
        }
    }
}
