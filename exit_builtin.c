
/**
 * exit_builtin.c
 */



#include <windows.h>
#include <inttypes.h>
#include <iso646.h>
#include "_winshell_private.h"



/**
 * exit_builtin
 * 
 * Kills all running processes and terminates the shell.
 * This function doesn't return - the process terminates.
 * 
 * parsed_proc: Parsed info from command that called this builtin, not
 *              used.
 * startup_info: Redirection info, not used.
 */
void exit_builtin(parsed_process_t *parsed_proc, 
                  STARTUPINFO *startup_info) {
    
    BOOL bool_rc;
    DWORD dw_rc;
    
    // Kill all jobs
    for (int32_t job_i = 0; job_i < MAX_JOBS; job_i++) {
        job_t *job = &jobs[job_i];
        if (job->status == GARBAGE) 
            continue;
        terminate_job(job);
    }

    // Signal the cmdline reader thread
    dw_rc = WaitForSingleObject(exited_lock, INFINITE);
    if (dw_rc == WAIT_FAILED) {
        print_err(L"exit_builtin -> WaitForSingleObject exited_lock");
        ExitProcess(1);
    }
    exited = TRUE;
    bool_rc = SetEvent(exited_e);
    if (not bool_rc) {
        print_err(L"exit_builtin -> SetEvent exited_e");
        ExitProcess(1);
    }
    bool_rc = SetEvent(cmdline_consumed_e);
    if (not bool_rc) {
        print_err(L"exit_builtin -> SetEvent cmdline_consumed_e");
    }
    bool_rc = ReleaseMutex(exited_lock);
    if (not bool_rc) {
        print_err(L"exit_builtin -> ReleaseMutex exited_lock");
        ExitProcess(1);
    }
    dw_rc = WaitForSingleObject(cmdline_reader_thread_h, INFINITE);
    if (dw_rc == WAIT_FAILED) {
        print_err(L"exit_builtin -> WaitForSingleObject cmdline_reader_thread_h");
        ExitProcess(1);
    }

    // Print goodbye message
    bool_rc = WriteConsoleW( // idc if this fails
        GetStdHandle(STD_OUTPUT_HANDLE),
        L"see ya!\n",
        wcslen(L"see ya!\n"),
        NULL,
        NULL
    );
    Sleep(4000);
    ExitProcess(0);
}