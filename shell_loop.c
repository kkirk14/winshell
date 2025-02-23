
/**
 * shell_loop.c
 */



#ifndef UNICODE 
#define UNICODE
#endif



#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include "_winshell_private.h"



static BOOL wait_cmdline_children(job_t *fg_job, DWORD *out_signaled_i) {

    DWORD dw_rc;

    if (fg_job == NULL) { // waiting at prompt: wait for cmdline and children
        dw_rc = WaitForMultipleObjects(
            (DWORD)n_wait_handles,
            wait_handles,
            FALSE,
            INFINITE
        );
        *out_signaled_i = dw_rc - WAIT_OBJECT_0;
    }
    else { // active fg job: ignore cmdline_available
        dw_rc = WaitForMultipleObjects(
            (DWORD)(n_wait_handles - 1),
            wait_handles + 1,
            FALSE,
            INFINITE
        );
        *out_signaled_i = dw_rc - WAIT_OBJECT_0 + 1;
    }
    if (dw_rc == WAIT_FAILED) {
        print_err(
            L"shell_loop.c -> "
            L"wait_for_event -> WaitForMultipleObjects"
        );
        return FALSE;
    }
}



/**
 * shell_loop
 * 
 * Called by main to run the shell loop.
 * Repeatedly reads commands from the console and spawns and manages jobs
 * accordingly.
 */
void shell_loop() {

    DWORD dw_rc;
    BOOL bool_rc;
    int32_t rc;
    WCHAR my_cmdline[MAX_CMDLINE + 1];
    HANDLE stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);

    const WCHAR *prompt = L"winshell> ";

    // Non-NULL when fg job is active, NULL when no fg job active (waiting 
    // at prompt)
    job_t *fg_job = NULL;

    // Create the cmdline reader thread
    cmdline_reader_thread_h = CreateThread(
        NULL,
        0,
        cmdline_reader_tproc,
        NULL,
        0,
        NULL
    );
    if (cmdline_reader_thread_h == INVALID_HANDLE_VALUE) {
        print_err(L"shell_loop -> CreateThread");
        ExitProcess(1);
    }

    while (TRUE) {

        // Print Prompt
        if (fg_job == NULL) {
            bool_rc = WriteFile(
                stdout_h,
                prompt,
                wcslen(prompt) * sizeof(WCHAR),
                NULL, 
                NULL
            );
            if (!bool_rc) {
                print_err(L"shell_loop -> WriteConsoleW prompt");
                ExitProcess(1);
            }
        }

        // Wait for an event
        DWORD signaled_i;
        bool_rc = wait_cmdline_children(fg_job, &signaled_i);
        if (!bool_rc)
            ExitProcess(1);
        HANDLE signaled = wait_handles[signaled_i];

        // cmdline_available was signaled
        if (signaled_i == 0) {

            // Consume cmdline
            dw_rc = WaitForSingleObject(cmdline_lock, INFINITE);
            if (dw_rc == WAIT_FAILED) {
                print_err(L"shell_loop -> WaitForSingleObject");
                ExitProcess(1);
            }
            memcpy(my_cmdline, cmdline, (wcslen(cmdline) + 1) * sizeof(WCHAR));
            bool_rc = ReleaseMutex(cmdline_lock);
            if (!bool_rc) {
                print_err(L"shell_loop -> ReleaseMutex");
                ExitProcess(1);
            }
            
            int32_t job_i = spawn_job(my_cmdline);
            
            // Syscall failure
            if (job_i == SPAWNJOB_SYSCALL_FAILURE) {
                ExitProcess(1);
            }

            // Malformed command (empty pipe)
            else if (job_i == SPAWNJOB_EMPTY_PIPE) {
                const WCHAR *message = L"Error: empty pipe\n";
                WriteFile(
                    GetStdHandle(STD_ERROR_HANDLE),
                    message,
                    wcslen(message) * sizeof(WCHAR),
                    NULL, 
                    NULL
                );
            }

            // Just pressed enter
            else if (job_i == SPAWNJOB_EMPTY_CMDLINE) {
                // Do nothing...
            }

            else if (job_i == SPAWNJOB_EMPTY_JOB) {
                // Do nothing...
            }

            // Job was successfully spawned
            else {
                job_t *job = &jobs[job_i];
                // Add job->proc_hs to wait_handles
                append_to_wait_handles(job->proc_hs, job->n_procs_alive);
                // Is this a fg job?
                if (job->is_foreground) {
                    // Note: We don't signal cmdline reader thread because we 
                    //       don't want it racing with child proc over stdin.
                    //       We'll signal it once the fg_job finishes.
                    fg_job = job;
                }
                else {
                    // TODO: print job info
                }
            }
            
            // Signal cmdline reader thread to continue
            if (fg_job == NULL) {
                bool_rc = SetEvent(cmdline_consumed_e);
                if (!bool_rc) {
                    print_err(L"shell_loop -> SetEvent");
                    ExitProcess(1);
                }
            }
        }

        // Process has terminated
        else {
            
            DWORD signaled_pid = GetProcessId(signaled);
            
            bool_rc = reap_proc(signaled);

            if (!bool_rc) {
                fwprintf(
                    stderr, 
                    L"ERROR: reap_proc pid=%d failed (not found)\n",
                    signaled_pid
                );
                fflush(stderr);
            }
            
            // fg job finished
            if (fg_job != NULL && fg_job->status != RUNNING) {
                fg_job = NULL;
                // Signal cmdline reader thread to continue
                bool_rc = SetEvent(cmdline_consumed_e);
                if (!bool_rc) {
                    print_err(L"shell_loop -> SetEvent");
                    ExitProcess(1);
                }
            }
        }
    }
}
