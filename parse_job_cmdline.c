
/**
 * parse_job_cmdline.c
 */



#ifndef UNICODE
#define UNICODE
#endif



#include <windows.h>
#include <WinDef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <wchar.h>
#include "_winshell_private.h"



/**
 * find_real_dquote
 * 
 * Searches a string for a non-escaped (\") double quote L'"'.
 * 
 * str: NULL-terminated string to be searched
 * 
 * Return Value: Returns a pointer to the next real L'"' character.
 *               Returns NULL if there are no L'"' characters.
 */
static WCHAR *find_real_dquote(const WCHAR *str) {

    // edge case: first character is dquote
    // If we don't treat this as edge case, potential invalid mem access
    if (*str == L'"') {
        return (WCHAR *)str;
    }

    const WCHAR *quote_p,
                *str_p = str + 1;

    do {
        quote_p = wcschr(str_p, L'"');
    } while (quote_p && *(quote_p - 1) == L'\\');
    
    return (WCHAR *)quote_p;
}



/**
 * nonquoted_wcschr
 * 
 * Finds the first non-quoted instance of a given character in a string.
 * All instances of c within "double quotes" will be ignored.
 * 
 * str: NULL-terminated to be searched.
 * c: Character to search for.
 * 
 * Return Value: Returns a pointer to the first non-quoted instance of c in 
 *               str. Returns NULL if non-quoted c isn't present in str.
 */
static WCHAR *nonquoted_wcschr(const WCHAR *str, WCHAR c) {
    
    const WCHAR *str_p = str, 
          *quote_p, 
          *c_p;
    BOOL is_quoted = FALSE;
    
    do {
        c_p = wcschr(str_p, c);
        quote_p = find_real_dquote(str_p);
        while (c_p && quote_p && quote_p < c_p) {
            is_quoted = !is_quoted;
            quote_p = find_real_dquote(quote_p + 1);
        }
        str_p = quote_p + 1;
    } while(c_p && is_quoted);

    return (WCHAR *)c_p;
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
 * separate_procs
 * 
 * Iterates through the NULL-terminated job_cmdline string and returns 
 * information for separating all the separate process command lines.
 *
 * job_cmdline: Contains the whole command line for the job
 * out_proc_cmdlines: This buffer will be filled with pointers to the start of 
 *                    each process cmdline.
 * out_len_proc_cmdlines: This buffer will be filled with the lengths of each
 *                        process cmdline.
 * 
 * Return Value: Returns the number of processes in the job. 
 */
static int32_t separate_procs(const WCHAR *job_cmdline, 
                              const WCHAR **out_proc_cmdlines,
                              size_t *out_len_proc_cmdlines) {
    
    int32_t n_procs = 0;
    const WCHAR *job_cmdline_p = job_cmdline,
                *proc_cmdline_p, *pipe_p; 
    if (*skip_whitespace(job_cmdline_p) == L'\0') {
        return 0;
    }
    
    do {
        // Find start and end of proc cmdline
        proc_cmdline_p = skip_whitespace(job_cmdline_p);
        pipe_p = nonquoted_wcschr(proc_cmdline_p, L'|');
        // Set the start and length values
        out_proc_cmdlines[n_procs] = proc_cmdline_p;
        if (pipe_p != NULL) 
            out_len_proc_cmdlines[n_procs] = pipe_p - proc_cmdline_p;
        else 
            out_len_proc_cmdlines[n_procs] = wcslen(proc_cmdline_p);
        // Check if the cmdline is empty
        if (out_len_proc_cmdlines[n_procs] == 0) {
            return -1;
        }
        n_procs++;
        job_cmdline_p = pipe_p + 1;
    } while(pipe_p != NULL);

    return n_procs;
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
            cmdline = find_real_dquote(cmdline + 1);
            if (cmdline == NULL)
                return NULL;
        }
        cmdline++;
    }

    return (WCHAR *)cmdline;
}



/**
 * set_file_redirection
 * 
 * Sets the given parsed process struct's in_file and out_file members
 * based on its command line.
 * Also adjusts the cmd_line member so that the string doesn't include the 
 * redirection arguments.
 * 
 * parsed_proc: parsed_process_t that already has cmd_line initialized.
 */
static void set_file_redirection(parsed_process_t *parsed_proc) {

    WCHAR *in_arrow_p, *out_arrow_p;
    in_arrow_p = nonquoted_wcschr(parsed_proc->cmd_line, L'<');
    out_arrow_p = nonquoted_wcschr(parsed_proc->cmd_line, L'>');

    if (in_arrow_p != NULL) {
        // Find and copy the file name
        WCHAR *in_file_p = skip_whitespace(in_arrow_p + 1);
        WCHAR *in_file_end = arg_end(in_file_p);
        if (in_file_end == NULL)
            parsed_proc->in_file[0] = L'\0';
        else {
            memcpy(
                parsed_proc->in_file, 
                in_file_p, 
                sizeof(WCHAR) * (in_file_end - in_file_p)
            );
            parsed_proc->in_file[in_file_end - in_file_p] = L'\0';
        }
        // Add the new terminating character
        *in_arrow_p = L'\0';
    }
    else {
        parsed_proc->in_file[0] = L'\0';
    }

    if (out_arrow_p != NULL) {
        // Find and copy the out file name
        WCHAR *out_file_p = skip_whitespace(out_arrow_p + 1);
        WCHAR *out_file_end = arg_end(out_file_p);
        if (out_file_end == NULL)
            parsed_proc->out_file[0] = L'\0';
        else {
            memcpy(
                parsed_proc->out_file,
                out_file_p,
                sizeof(WCHAR) * (out_file_end - out_file_p)
            );
            parsed_proc->out_file[out_file_end - out_file_p] = L'\0';
        }
        // Add the new terminating character
        *out_arrow_p = L'\0';
    }
    else {
        parsed_proc->out_file[0] = L'\0';
    }
}



/**
 * is_foreground
 * 
 * Determines whether job_cmdline is a background or foreground job.
 * Finds the last argument and tests whether it equals L"&".
 * Replaces the L"&" with L"\0" if it's found.
 * 
 * job_cmdline: Points to the job cmdline.
 * 
 * Return Value: Returns TRUE if job is foreground, FALSE if it's background.
 */
static BOOL is_foreground(WCHAR *job_cmdline) {

    WCHAR *this_arg_p, *next_arg_p = job_cmdline;

    do {
        this_arg_p = next_arg_p;
        next_arg_p = arg_end(next_arg_p);
        if (next_arg_p == NULL)
            return FALSE;
        next_arg_p = skip_whitespace(next_arg_p);
    } while(*next_arg_p != L'\0');
    
    const WCHAR *this_arg_end = arg_end(this_arg_p);
    if (this_arg_end - this_arg_p == 1 && *this_arg_p == L'&') {
        *this_arg_p = L'\0';
        return FALSE;
    }
    else {
        return TRUE;
    }
}



/**
 * parse_job_cmdline
 * 
 * Parses a job command line and returns the information needed to spawn
 * the processes.
 * 
 * job_cmdline: Job command line inputted by user.
 * out_n_procs: Number of processes to be spawned will be put here. This is 
 *              also the size of the returned array.
 * out_is_foreground: Whether this is a foreground job will be placed here.
 * 
 * Return Value: Returns a poitner to a heap-allocated array of parsed 
 *               processes.
 *               If an error occurs, NULL will be returned and one of these
 *               error codes will be placed in out_n_procs:
 *                - SPAWNJOB_EMPTY_PIPE
 *                - SPAWNJOB_EMPTY_CMDLINE
 */
parsed_process_t *parse_job_cmdline(const WCHAR *job_cmdline,
                                    int32_t *out_n_procs,
                                    BOOL *out_is_foreground) {

    const WCHAR *proc_cmdlines[MAX_PROCS_PER_JOB];
    size_t len_proc_cmdlines[MAX_PROCS_PER_JOB];
    
    int32_t n_procs = separate_procs(
        job_cmdline, 
        proc_cmdlines, 
        len_proc_cmdlines
    );

    if (n_procs < 0) {
        *out_n_procs = SPAWNJOB_EMPTY_PIPE;
        return NULL;
    }
    else if (n_procs == 0) {
        *out_n_procs = SPAWNJOB_EMPTY_CMDLINE;
        return NULL;
    }

    parsed_process_t *parsed_procs = 
        malloc(sizeof(parsed_process_t) * n_procs);
    
    for (int i = 0; i < n_procs; i++) {

        parsed_process_t *parsed_proc = &parsed_procs[i];

        // cmd_line
        memcpy(
            parsed_proc->cmd_line, 
            proc_cmdlines[i], 
            len_proc_cmdlines[i] * sizeof(WCHAR)
        );
        parsed_proc->cmd_line[len_proc_cmdlines[i]] = L'\0';

        // application_name
        WCHAR *space_p = nonquoted_wcschr(parsed_proc->cmd_line, L' ');
        size_t len_application_name = 
            space_p ? space_p - parsed_proc->cmd_line : len_proc_cmdlines[i];
        memcpy(
            parsed_proc->application_name, 
            parsed_proc->cmd_line, 
            len_application_name * sizeof(WCHAR)
        );
        parsed_proc->application_name[len_application_name] = L'\0'; 

        // foreground?
        *out_is_foreground = is_foreground(parsed_proc->cmd_line);

        // in_file and out_file
        set_file_redirection(parsed_proc);

        // pipe_input and pipe_output
        parsed_proc->pipe_input = i > 0;
        parsed_proc->pipe_output = i < n_procs - 1;
    }
    
    // Return
    *out_n_procs = n_procs;
    return parsed_procs;
}
