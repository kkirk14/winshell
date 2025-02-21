
/**
 * cmdline_reader_tproc.c
 */



#include <windows.h>
#include "_winshell_private.h"



/* exited: FALSE while shell is running, TRUE when the shell is exiting (and
           cmdline reader thread needs to terminated). */
BOOL exited = FALSE;

/* exited_lock: HANDLE to kernel mutex that protects the exited variable. */
HANDLE exited_lock;
           
/* exited_e: HANDLE to a kernel event that will be signaled when the shell is 
             about to exit. This is how the main (job-spawning) thread will 
             notify the cmdline reader thread that it should terminate. */
HANDLE exited_e;



/**
 * cmdline_reader_tproc
 * 
 * This is the thread procedure of the thread that reads commands from stdin
 * and places them in the cmdline buffer.
 */
DWORD WINAPI cmdline_reader_tproc(void *arg) {
    
    DWORD dw_rc;
    BOOL bool_rc;

    HANDLE stdin_exited_hs[] = {
        GetStdHandle(STD_INPUT_HANDLE),
        exited_e
    };

    // lock exited_lock for loop condition check
    dw_rc = WaitForSingleObject(exited_lock, INFINITE);
    if (dw_rc == WAIT_FAILED) {
        print_err(L"cmdline_reader_tproc -> WaitForSingleObject exited_lock");
        ExitProcess(1);
    }

    while (!exited) {

        ReleaseMutex(exited_lock);

        // Wait for stdin or exit
        dw_rc = WaitForMultipleObjects(
            2,
            stdin_exited_hs,
            FALSE,
            INFINITE
        );
        if (dw_rc == WAIT_FAILED) {
            print_err(L"cmdline_reader_tproc -> WaitForMultipleObjects");
            ExitProcess(1);
        }
        DWORD signaled_i = dw_rc - WAIT_OBJECT_0;

        // exited_e was signaled
        if (signaled_i == 1) {
            WaitForSingleObject(exited_lock, INFINITE);
            if (exited) {
                ReleaseMutex(exited_lock);
                ExitThread(0);
            }
            ReleaseMutex(exited_lock);
        }

        // Lock cmdline_lock and read stdin into cmdline buffer
        HANDLE stdin_h = stdin_exited_hs[0];
        DWORD wchars_read;

        dw_rc = WaitForSingleObject(cmdline_lock, INFINITE);
        if (dw_rc == WAIT_FAILED) {
            print_err(L"cmdline_read_tproc -> WaitForSingleObject cmdline_lock");
            ExitProcess(1);
        }

        bool_rc = ReadConsoleW(stdin_h, cmdline, MAX_CMDLINE, &wchars_read, NULL);
        if (!bool_rc) {
            print_err(L"cmdline_reader_tproc -> ReadConsoleW");
            ExitProcess(1);
        }

        cmdline[wchars_read - 2] = L'\0'; // wchars_read - 2 is the '\r\n' char

        // Signal job spawner that cmdline is avaiable
        bool_rc = SetEvent(cmdline_available_e);
        if (!bool_rc) {
            print_err(L"cmdline_reader_tproc -> SetEvent cmdline_available_e");
            ExitProcess(1);
        }
        bool_rc = ReleaseMutex(cmdline_lock);
        if (!bool_rc) {
            print_err(L"cmdline_reader_tproc -> ReleaseMutex cmdline_lock");
        }
        
        // Wait for the job spawner to finish using cmdline
        dw_rc = WaitForSingleObject(cmdline_consumed_e, INFINITE);
        if (dw_rc == WAIT_FAILED) {
            print_err(
                L"cmdline_reader_tproc -> "
                L"WaitForSingleObject cmdline_consumed_e"
            );
            ExitProcess(1);
        }

        // lock exited_lock for loop condition check
        dw_rc = WaitForSingleObject(exited_lock, INFINITE);
        if (dw_rc == WAIT_FAILED) {
            print_err(L"cmdline_reader_tproc -> WaitForSingleObject exited_lock");
            ExitProcess(1);
        }
    }
}
