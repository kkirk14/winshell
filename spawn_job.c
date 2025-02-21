
/**
 * spawn_job.c
 */



#ifndef UNICODE
#define UNICODE
#endif



#include <stdlib.h>
#include <windows.h>
#include <WinDef.h>
#include <inttypes.h>
#include <wchar.h>
#include <wctype.h>
#include <string.h>
#include <stdio.h>

#include "_winshell_private.h"



/**
 * dup_uninherit_to_inherit
 * 
 * Duplicates uninheritable HANDLE old_h to inheritable copy new_h.
 * old_h is closed.
 * 
 * old_h: Uninheritable HANDLE to be dup-ed.
 * new_h: New inheritable HANDLE will be placed here.
 * 
 * Return Value: Returns TRUE on success, FALSE on failure.
 */
static BOOL dup_uninherit_to_inherit(HANDLE old_h, 
                                     HANDLE *new_h) {
    
    BOOL bool_rc = DuplicateHandle(
        GetCurrentProcess(),
        old_h,
        GetCurrentProcess(),
        new_h,
        0,
        TRUE,
        DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE // old_h will be closed
    );
    return bool_rc;
}



/**
 * create_my_pipe
 * 
 * When we spawn a process that pipes its output, we need to create a pipe 
 * with an uninheritable read end and an inheritable write end.
 * 
 * out_read_pipe: HANDLE to the new pipe's uninheritable read end will be 
 *                placed here.
 * out_write_pipe: HANDLE to the new pipe's inheritable write end will be 
 *                 placed here.
 * 
 * Return Value: Returns TRUE on success, FALSE on failure.
 */
static BOOL create_my_pipe(HANDLE *out_read_pipe, 
                            HANDLE *out_write_pipe) {
    
    BOOL bool_rc;

    HANDLE my_read_pipe, my_write_pipe;

    SECURITY_ATTRIBUTES pipe_sa = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = FALSE
    };

    bool_rc = CreatePipe(
        &my_read_pipe,
        &my_write_pipe,
        &pipe_sa,
        0
    );
    if (!bool_rc) { // Did CreatePipe fail?
        return FALSE;
    }

    HANDLE dup_write_pipe;
    bool_rc = dup_uninherit_to_inherit(my_write_pipe, &dup_write_pipe);
    if (!bool_rc) { // DuplicateHandle failed
        CloseHandle(my_read_pipe);
        CloseHandle(my_write_pipe);
        return FALSE;
    }

    // Return
    *out_read_pipe = my_read_pipe;
    *out_write_pipe = dup_write_pipe;
    return TRUE;
}



/**
 * open_out_file
 * 
 * Opens out_file for output redirection.
 *
 * out_file: File name to open
 * 
 * Return Value: Returns a HANDLE to the file on success.
 *               Returns INVALID_HANDLE_VALUE on failure.
 */
static HANDLE open_out_file(const WCHAR *out_file) {
    SECURITY_ATTRIBUTES sa = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = TRUE
    };
    HANDLE out_file_h = CreateFileW(
        out_file, 
        GENERIC_WRITE,
        0,
        &sa,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (out_file_h == INVALID_HANDLE_VALUE) {
        print_err(L"CreateFileW opening output file");
    }
    return out_file_h;
}



/**
 * open_in_file
 * 
 * Opens in_file for input redirection.
 * 
 * in_file: File name to open
 * 
 * Return Value: Returns a HANDLE to the file on success.
 *               Returns INVALID_HANDLE_VALUE on failure.
 */
static HANDLE open_in_file(const WCHAR *in_file) {
    SECURITY_ATTRIBUTES sa = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = TRUE
    };
    HANDLE in_file_h = CreateFileW(
        in_file,
        GENERIC_READ,
        0,
        &sa,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (in_file_h == INVALID_HANDLE_VALUE) {
        print_err(L"CreateFileW opening input file");
    }
    return in_file_h;
}



/**
 * spawn_job_failure_recover
 * 
 * Called by spawn_job when some kind of error occurs in the middle of 
 * spawning the processes (like syscall error).
 * Frees any resources for the job structs.
 * Terminates all active processes (with TerminateProcess).
 * 
 * job: Contains information on the job that failed - was in the process of 
 *      being built.
 * 
 * Return Value: Returns TRUE on success, FALSE on failure.
 */
static BOOL spawn_job_failure_recover(job_t *job) {

    BOOL bool_rc;
    DWORD dw_rc, exit_code;

    // kill all processes
    for (int i = 0; i < job->n_procs_alive; i++) {
        HANDLE proc_h = job->proc_hs[i];
        DWORD pid = GetProcessId(proc_h);
        bool_rc = TerminateProcess(proc_h, 1);
        if (!bool_rc) {
            print_err(L"spawn_job_failure_recover -> TerminateProcess");
            continue;
        }
        do {
            dw_rc = WaitForSingleObject(proc_h, INFINITE);
            if (dw_rc == WAIT_FAILED) {
                print_err(L"spawn_job_failure_recover -> WaitForSingleObject");
                break;
            }
            bool_rc = GetExitCodeProcess(proc_h, &exit_code);
        } while(bool_rc && exit_code == STILL_ACTIVE);

        CloseHandle(proc_h);
    }

    // Free job resources
    free(job->proc_hs);
    free(job->cmdline);
    job->status = GARBAGE;
}



static void hexdump(const void *addr, DWORD n_bytes) {

    const unsigned char *addr_c = (const unsigned char *)addr;
    int i;

    for (i = 0; i < n_bytes; i++) {
        DWORD mod = i % 16;
        switch (mod) {
        case 15:
            wprintf(L"%02x\n", addr_c[i]);
            break;
        case 7:
            wprintf(L"%02x   ", addr_c[i]);
            break;
        case 0:
            wprintf(L"%04d    %02x ", i, addr_c[i]);
            break;
        default:
            wprintf(L"%02x ", addr_c[i]);
            break;
        }
    }
    if (i % 16 != 0) {
        wprintf(L"\n");
    }
    fflush(stdout);
}   



/**
 * spawn_job
 *
 * Does all the work for spawning the job from the given job_cmdline. 
 * Cleans/parses the cmdline, sets up the pipes and I/O redirection, 
 * spawns the processes, and adds the job struct to the jobs array.
 *
 * Return Value: Returns the jid of the new job on success. 
 *               Returns a negative number on failure:
 *                - SPAWNJOB_EMPTY_PIPE 
 *                - SPAWNJOB_EMPTY_CMDLINE
 *                - SPAWNJOB_SYSCALL_FAILURE
 */
int32_t spawn_job(const WCHAR *job_cmdline) {
    
    BOOL bool_rc;

    STARTUPINFO startup_info = {
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_USESTDHANDLES
    };

    HANDLE my_prev_read_pipe, my_next_read_pipe, // no inherit
           dup_prev_read_pipe, dup_write_pipe; // inherit
    HANDLE in_file_h, out_file_h;

    // Get stdin and stdout
    HANDLE stdin_h = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);

    // Find jid
    int32_t jid = find_open_jid();
    if (jid < 0) {
        // TODO: Print error message: too many jobs
        return -1;
    }
    job_t *job = &jobs[jid];

    // Parse the job cmdline
    int32_t n_procs;
    parsed_process_t *parsed_procs = parse_job_cmdline(
        job_cmdline, 
        &n_procs, 
        &job->is_foreground
    );
    if (parsed_procs == NULL) {
        return n_procs;
    }

    // Allocate the job->proc_hs array
    job->proc_hs = malloc(n_procs * sizeof(HANDLE));

    // Allocate and initialize job->cmdline
    size_t len_job_cmdline = wcslen(job_cmdline);
    job->cmdline = malloc(sizeof(wchar_t) * (len_job_cmdline + 1));
    memcpy(job->cmdline, job_cmdline, sizeof(wchar_t) * len_job_cmdline);
    job->cmdline[len_job_cmdline] = L'\0';
    job->n_procs_alive = 0;

    // Iterate through all processes
    for (int proc_i = 0; proc_i < n_procs; proc_i++) {

        parsed_process_t *curr_parsed_proc = &parsed_procs[proc_i];

        // ---------- Output redirection ----------

        // Output is piped: create a pipe 
        if (curr_parsed_proc->pipe_output) {
            bool_rc = create_my_pipe(&my_next_read_pipe, &dup_write_pipe);
            if (!bool_rc) {
                print_err(L"spawn_job -> create_my_pipe");
                spawn_job_failure_recover(job);
                return SPAWNJOB_SYSCALL_FAILURE;
            }
            startup_info.hStdOutput = dup_write_pipe;
        }

        // We need to redirect output to a file
        else if (curr_parsed_proc->out_file[0] != L'\0') {
            out_file_h = open_out_file(curr_parsed_proc->out_file);
            if (out_file_h != INVALID_HANDLE_VALUE)
                startup_info.hStdOutput = out_file_h;
            else 
                startup_info.hStdOutput = stdout_h;
        }

        // Output just goes to stdout
        else {
            startup_info.hStdOutput = stdout_h;
        }

        // ---------- Input redirection ----------

        // Input is piped: Make previous pipe inheritable
        if (curr_parsed_proc->pipe_input) {
            bool_rc = dup_uninherit_to_inherit(
                my_prev_read_pipe,              // my_prev_read_pipe now closed
                &dup_prev_read_pipe
            );
            if (!bool_rc) {
                CloseHandle(my_prev_read_pipe);
                print_err(L"DuplicateHandle my_prev_read_pipe");
                spawn_job_failure_recover(job);
                return SPAWNJOB_SYSCALL_FAILURE;
            }
            startup_info.hStdInput = dup_prev_read_pipe;
        }

        // Input is redirected to a file
        else if (curr_parsed_proc->in_file[0] != L'\0') {
            in_file_h = open_in_file(curr_parsed_proc->in_file);
            if (in_file_h != INVALID_HANDLE_VALUE)
                startup_info.hStdInput = in_file_h;
            else
                startup_info.hStdInput = stdin_h;
        }

        // Input just comes from stdin
        else {
            startup_info.hStdInput = stdin_h;
        }

        // ---------- Process spawning ----------

        PROCESS_INFORMATION proc_info;

        // Setup CreateProcessW creation flags
        DWORD dwCreationFlags = CREATE_UNICODE_ENVIRONMENT;
        if (job->n_procs_alive == 0) {
            dwCreationFlags |= CREATE_NEW_PROCESS_GROUP;
        }

        // TODO: resolve application_name to absolute path

        // Call CreateProcessW
        bool_rc = CreateProcessW(
            curr_parsed_proc->application_name,
            curr_parsed_proc->cmd_line,
            NULL,
            NULL,
            TRUE,
            dwCreationFlags,
            NULL, // envp,
            NULL, // curr_dir,
            &startup_info,
            &proc_info
        );
        if (!bool_rc) { // CreateProcessW failed
            print_err(L"spawn_job -> CreateProcessW");
            spawn_job_failure_recover(job);
            return SPAWNJOB_SYSCALL_FAILURE;
        }
        else { // CreateProcessW successful
            job->proc_hs[job->n_procs_alive++] = proc_info.hProcess;
            job->status = RUNNING;
            CloseHandle(proc_info.hThread);
        }

        // ---------- Clean up ----------
        if (curr_parsed_proc->pipe_output
             && dup_write_pipe != INVALID_HANDLE_VALUE)
            CloseHandle(dup_write_pipe);
        if (curr_parsed_proc->pipe_input
             && dup_prev_read_pipe != INVALID_HANDLE_VALUE)
            CloseHandle(dup_prev_read_pipe);
        if (curr_parsed_proc->in_file[0] != L'\0'
             && in_file_h != INVALID_HANDLE_VALUE)
            CloseHandle(in_file_h);
        if (curr_parsed_proc->out_file[0] != L'\0'
             && out_file_h != INVALID_HANDLE_VALUE)
            CloseHandle(out_file_h);
        my_prev_read_pipe = my_next_read_pipe;
    }

    free(parsed_procs);

    // Return
    return jid;
}
