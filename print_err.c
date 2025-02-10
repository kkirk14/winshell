
/**
 * print_err.c
 */



#include <windows.h>



/**
 * print_err
 * 
 * Prints err message to stderr based on GetLastError code.
 */
void print_err(WCHAR *err_name) {

    WCHAR *err_message;
    HANDLE stderr_h = GetStdHandle(STD_ERROR_HANDLE);

    size_t len_err_buf = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
         | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&err_message,
        0,
        NULL
    );

    WriteFile(stderr_h, err_name, wcslen(err_name) * sizeof(WCHAR), NULL, NULL);
    WriteFile(stderr_h, L": ", wcslen(L": ") * sizeof(WCHAR), NULL, NULL);
    WriteFile(stderr_h, err_message, len_err_buf * sizeof(WCHAR), NULL, NULL);
    LocalFree(err_message);
}
