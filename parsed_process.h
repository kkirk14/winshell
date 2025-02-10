
/**
 * parsed_process.h
 */



#ifndef _PARSED_PROCESS_H
#define _PARSED_PROCESS_H


#include <windows.h>
#include <WinDef.h>


#ifndef MAX_CMDLINE
#define MAX_CMDLINE 32767
#endif



/**
 * parsed_process struct
 * 
 * Contains information for spawning a job process that was parsed from the 
 * job's command line.
 */
typedef struct _parsed_process {

    /* application_name: Name of the executable to run. Pass this to 
                         CreateProcessW as the lpApplicationName argument. */
    WCHAR application_name[MAX_PATH + 1];

    /* cmd_line: Full command line containing application name and arguments.
                 Pass this to CreateProcessW as the lpCommandLine argument. */
    WCHAR cmd_line[MAX_CMDLINE + 1];

    /* in_file: If stdin is to be redirected to a file, this will contain the 
                name of the file. If not, this will be a zero-length string. */
    WCHAR in_file[MAX_PATH + 1];

    /* out_file: If stdout is to be redirected to a file, this will contain the
                 name of the file. If not, this will be a zero-length string. */
    WCHAR out_file[MAX_PATH + 1];

    /* pipe_input: Is stdin piped from the previous process? */
    bool pipe_input;

    /* pipe_output: Is stdout piped to the next process? */
    bool pipe_output;

} parsed_process_t;



#endif
