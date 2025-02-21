
/**
 * cmdline_data.c
 */



#include <windows.h>
#include <inttypes.h>
#include <WinDef.h>
#include "_winshell_private.h"



/* cmdline: Contains a line read from stdin waiting to be read by the job 
            spawner thread. */
WCHAR cmdline[MAX_CMDLINE + 1];

/* cmdline_consumed_e: After reading and setting cmdline, the reader thread
                       will wait on this condition before reading another line.
                       The job spawner thread will signal this once it's done 
                       with cmdline. This prevents a (probably very unlikely)
                       race where commands are skipped. */
HANDLE cmdline_consumed_e;

/* cmdline_lock: HANDLE to a kernel mutex that protects the data in this file.
                 Lock this whenever reading and writing cmdline. */
HANDLE cmdline_lock;

/* cmdline_available_e: HANDLE to a kernel event. It is in the signaled state 
                        when there is data in cmdline for spawner thread to
                        read. Use SetEvent to signal this event. */
HANDLE cmdline_available_e;
