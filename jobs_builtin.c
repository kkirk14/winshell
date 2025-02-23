
/**
 * jobs_builtin.c
 */



#include <windows.h>
#include <iso646.h>
#include "_winshell_private.h"



/**
 * jobs_builtin
 * 
 * parsed_proc: Contains parsed information about command line that called
 *              this builtin to be called.
 * startup_info: Contains redirection info - will output to hStdOutput.
 * 
 * Return Value: Returns TRUE on success, returns FALSE on failure.
 */
BOOL jobs_builtin(parsed_process_t *parsed_proc, 
                  STARTUPINFO *startup_info) {
    
    BOOL bool_rc;

    HANDLE stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout_h == INVALID_HANDLE_VALUE) {
        print_err(L"jobs_builtin -> GetStdHandle");
        return FALSE;
    }
    
    for (int i = 0; i < MAX_JOBS; i++) {

        job_t *job = &jobs[i];

        if (job->status == GARBAGE) {
            continue;
        }
        
        WCHAR *job_str = job_to_str(job);
        if (job_str == NULL) 
            continue;
        
        if (stdout_h == startup_info->hStdOutput) {
            bool_rc = WriteConsoleW(stdout_h, job_str, wcslen(job_str), NULL, NULL);
            if (not bool_rc) {
                print_err(L"jobs_builtin -> WriteConsoleW");
                free(job_str);
                return FALSE;
            }
            bool_rc = WriteConsoleW(stdout_h, L"\n", 1, NULL, NULL);
            if (not bool_rc) {
                print_err(L"jobs_builtin -> WriteConsoleW");
                free(job_str);
                return FALSE;
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
                print_err(L"jobs_builtin -> WriteFile");
                free(job_str);
                return FALSE;
            }
            bool_rc = WriteFile(
                startup_info->hStdOutput,
                L"\n",
                1 * sizeof(WCHAR),
                NULL,
                NULL
            );
            if (not bool_rc) {
                print_err(L"jobs_builtin -> WriteFile");
                free(job_str);
                return FALSE;
            }
        }

        free(job_str);

        if (job->status == TERMINATED) {
            free(job->cmdline);
            free(job->proc_hs);
            job->status = GARBAGE;
        }
    }

    return TRUE;
}
