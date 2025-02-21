
/**
 * _winshell_private.h
 *
 * Declarations and includes for winshell local code.
 */



#ifndef _WINSHELL_PRIVATE_H
#define _WINSHELL_PRIVATE_H



#ifndef UNICODE
#define UNICODE
#endif



#include <inttypes.h>



#include "job.h"
#include "parsed_process.h"



/* MAX_CMDLINE: Maximum number of characters that can be in a processes' 
                command line. */
#define MAX_CMDLINE 32767



/* MAX_JOBS: The maximum number of jobs that can be active at the same time. */
#define MAX_JOBS 4096



/* MAX_PROCS_PER_JOB: The maximum number of processes that a single job can 
                      have. */
#define MAX_PROCS_PER_JOB 4096



/* SPAWNJOB_EMPTY_PIPE: This error code means that a job cmdline contained an 
                        pipe: "sleep 4 | | sleep 4" */
#define SPAWNJOB_EMPTY_PIPE -2

/* SPAWNJOB_EMPTY_CMDLINE: This error code means that the user just provided
                           whitespace. */
#define SPAWNJOB_EMPTY_CMDLINE -3

/* SPAWNJOB_SYSCALL_FAILURE: A syscall failed. */
#define SPAWNJOB_SYSCALL_FAILURE -4



/* wait_handles: Points to heap-allocated array of handles. This is the array
                 that will be used in the shell loop's WaitForMultipleObjects
                 call - contains stdin and all running processes of all jobs.
                 Note: When a process terminates it must be removed from this
                       arr. */
extern HANDLE *wait_handles;

/* n_wait_handles: Current usage of the wait_handles array. */
extern int32_t n_wait_handles;

/* cap_wait_handles: Current capacity of the wait_handles array. */
extern int32_t cap_wait_handles;


/* jobs: Static-duration array of jobs - the data needed to manage each job is 
         contained somewhere in this array. */
extern job_t jobs[];



/* cmdline: Contains a line read from stdin waiting to be read by the job 
            spawner thread. */
extern WCHAR cmdline[];

/* cmdline_consumed_e: After reading and setting cmdline, the reader thread
                       will wait on this condition before reading another line.
                       The job spawner thread will signal this once it's done 
                       with cmdline. This prevents a (probably very unlikely)
                       race where commands are skipped. */
extern HANDLE cmdline_consumed_e;
                        
/* cmdline_lock: HANDLE to a kernel mutex that protects the data in this file.
                 Lock this whenever reading and writing cmdline. */
extern HANDLE cmdline_lock;
                        
/* cmdline_available_e: HANDLE to a kernel event. It is in the signaled state  */
extern HANDLE cmdline_available_e;



/* exited: FALSE while shell is running, TRUE when the shell is exiting (and
           cmdline reader thread needs to terminated). */
extern BOOL exited;

/* exited_lock: HANDLE to kernel mutex that protects the exited variable. */
extern HANDLE exited_lock;

/* exited_e: HANDLE to a kernel event that will be signaled when the shell is 
              about to exit. This is how the main (job-spawning) thread will 
              notify the cmdline reader thread that it should terminate. */
extern HANDLE exited_e;



/**
 * find_open_jid
 *
 * Finds the first jid that's open to be used for a new job.
 *
 * Return Value: Returns the jid on success.
 *               Return -1 if the jobs array is full.
 */
int32_t find_open_jid();


/**
 * init_winshell
 * 
 * Does some initialization tasks that must be done before the main shell loop
 * is run:
 *  - initializes the jobs array
 */
int init_winshell();



/**
 * append_to_wait_handles
 * 
 * Appends an array of HANDLEs to the end of the job_mgt_data wait_handles
 * array.
 * 
 * handles: Array of handles to append
 * len_handles: Number of HANDLEs in the handles array
 * 
 * Return Value: Returns TRUE on success. 
 *               Returns FALSE on failure (realloc failed).
 */
BOOL append_to_wait_handles(HANDLE *handles, int32_t len_handles);



/**
 * print_err
 * 
 * Prints err message to stderr based on GetLastError code.
 */
void print_err();



/**
 * handle_arr_remove
 * 
 * Searches for and removes the first instance of to_remove HANDLE from arr 
 * HANDLE array.
 * 
 * arr: HANDLE array to search and modify.
 * in_out_len_arr: When called this should contain the current usage of arr.
 *                 On exit this will contain the new usage of arr. 
 * to_remove: HANDLE to search for and remove.
 * 
 * Return Value: Returns TRUE if to_remove was found and removed.
 *               Returns FALSE if to_remove wasn't found.
 */
BOOL handle_arr_remove(HANDLE *arr, int32_t *in_out_len_arr, HANDLE to_remove);



/**
 * reap_proc
 * 
 * Called when a process just terminates to remove it from the job management
 * structures.
 * Cleans up the job_t struct and its resources if this was its last process.
 * 
 * proc_h: HANDLE of the process that just terminated.
 * 
 * Return Value: Returns TRUE on sucess, FALSE on failure.
 */
BOOL reap_proc(HANDLE proc_h);



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
                                    BOOL *out_is_foreground);



/**
 * spawn_job
 *
 * Does all the work for spawning the job from the given job_cmdline. 
 * Cleans/parses the cmdline, sets up the pipes and I/O redirection, 
 * spawns the processes, and adds the job struct to the jobs array.
 * 
 * Note: This function will alter the buffer at job_cmdline.
 *
 * Return Value: Returns the jid of the new job on success. 
 *               Returns -1 on failure.
 */
int32_t spawn_job(const WCHAR *job_cmdline);



/**
 * shell_loop
 * 
 * Called by main to run the shell loop.
 * Repeatedly reads commands from the console and spawns and manages jobs
 * accordingly.
 */
void shell_loop();



/**
 * cmdline_reader_tproc
 * 
 * This is the thread procedure of the thread that reads commands from stdin
 * and places them in the cmdline buffer.
 */
DWORD WINAPI cmdline_reader_tproc(void *arg);



// ifndef _WINSHELL_PRIVATE_H
#endif
