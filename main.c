
/**
 * main.c
 */



#ifndef UNICODE
#define UNICODE
#endif



#include <windows.h>
#include <stdio.h>
#include "_winshell_private.h"




/**
 * wWinMain
 *
 * Execution starts here.
 */
int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    PWSTR cmdLine,
                    int nCmdShow) {

    WCHAR buf[1];

    init_winshell();

    shell_loop();
    
    /*
    WCHAR job_cmdline[] = L"C:\\Users\\vek26\\programming\\c\\my_cat.exe ..\\lorem.txt | "
                          L"C:\\Users\\vek26\\AppData\\Local\\Programs\\Python\\Python39\\python.exe ..\\replace_space.py | "
                          L"C:\\Users\\vek26\\AppData\\Local\\Programs\\Python\\Python39\\python.exe ..\\replace_dash.py";
    */
    WCHAR job_cmdline[] = L"C:\\Users\\vek26\\programming\\c\\my_cat.exe < C:\\Users\\vek26\\programming\\c\\lorem.txt | "
                          L"C:\\Python313\\python.exe C:\\Users\\vek26\\programming\\c\\replace_space.py | "
                          L"C:\\Python313\\python.exe C:\\Users\\vek26\\programming\\c\\replace_dash.py > .\\_lorem_.txt";
    int32_t rc = spawn_job(job_cmdline);
    wprintf(L"spawn_job rc: %d\n", rc);
    
    fgetc(stdin);
    
    return 0;
}
