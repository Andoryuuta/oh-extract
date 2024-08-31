#include <cstdio>
#include <format>
#include <iostream>

#include <windows.h>

// Minimal typedefs to match CPython.
typedef enum
{
    PyGILState_LOCKED,
    PyGILState_UNLOCKED
} PyGILState_STATE;
typedef int(__stdcall *PyRun_SimpleString_t)(const char *);
typedef PyGILState_STATE(__stdcall *PyGILState_Ensure_t)();
typedef void(__stdcall *PyGILState_Release_t)(PyGILState_STATE);

void open_console()
{
    AllocConsole();
    FILE *cin_stream;
    FILE *cout_stream;
    FILE *cerr_stream;
    freopen_s(&cin_stream, "CONIN$", "r", stdin);
    freopen_s(&cout_stream, "CONOUT$", "w", stdout);
    freopen_s(&cerr_stream, "CONOUT$", "w", stderr);

    // From: https://stackoverflow.com/a/45622802 to deal with UTF8 CP:
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000);
}

DWORD MyThread(LPVOID)
{
    open_console();

    // Python is statically linked into the game, so we resolve the symbols
    // directly from our current modules export table.
    HMODULE exe_handle = GetModuleHandleA(NULL);
    PyRun_SimpleString_t PyRun_SimpleString =
        reinterpret_cast<PyRun_SimpleString_t>(GetProcAddress(exe_handle, "PyRun_SimpleString"));
    PyGILState_Ensure_t PyGILState_Ensure =
        reinterpret_cast<PyGILState_Ensure_t>(GetProcAddress(exe_handle, "PyGILState_Ensure"));
    PyGILState_Release_t PyGILState_Release =
        reinterpret_cast<PyGILState_Release_t>(GetProcAddress(exe_handle, "PyGILState_Release"));

    std::cout << std::format("PyRun_SimpleString addr: 0x{:X}", reinterpret_cast<uintptr_t>(PyRun_SimpleString)) << "\n"
              << std::flush;
    std::cout << std::format("PyGILState_Ensure addr: 0x{:X}", reinterpret_cast<uintptr_t>(PyGILState_Ensure)) << "\n"
              << std::flush;
    std::cout << std::format("PyGILState_Release addr: 0x{:X}", reinterpret_cast<uintptr_t>(PyGILState_Release)) << "\n"
              << std::flush;

    // Infinite python REPL loop.
    while (true)
    {
        PyGILState_STATE state = PyGILState_Ensure();

        PyRun_SimpleString("import sys, code\n"

                           // Reopen and map CONOUT and CONIN psuedo-files to python
                           // stdout/err. This is needed as we just allocated a new
                           // console via AllocConsole earlier, which invalidates
                           // any of the prior stdout/stderror handles python had.
                           "sys.stdout = open('CONOUT$', 'w')\n"
                           "sys.stderr = open('CONOUT$', 'w')\n"
                           "sys.stdin = open('CONIN$', 'r')\n"

                           // The game overrides the except hook, so we have to
                           // restore it here if we want to see any the exceptions
                           // in our REPL (syntax error, etc).
                           "sys.excepthook = sys.__excepthook__\n"
                           "code.interact(local=dict(globals(), **locals()))");

        PyGILState_Release(state);
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Spawn a new thread for our python REPL that isn't under the loader lock.
        CreateThread(NULL, 0, MyThread, NULL, 0, NULL);
    }

    return TRUE;
}