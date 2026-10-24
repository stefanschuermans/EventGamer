#include "shutdown.h"

#include <csignal>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace eventgamer {
namespace {

volatile std::sig_atomic_t signal_received = 0;

void handle_signal(int signal) { signal_received = signal; }

#ifdef _WIN32
volatile LONG console_shutdown = 0;
HANDLE cleanup_complete = nullptr;

BOOL WINAPI handle_console_event(DWORD event) {
    switch (event) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        InterlockedExchange(&console_shutdown, 1);
        return TRUE;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        InterlockedExchange(&console_shutdown, 1);
        // Windows terminates the process when this handler returns. Give main
        // time to unregister, within the console shutdown deadline.
        WaitForSingleObject(cleanup_complete, 4000);
        return TRUE;
    default:
        return FALSE;
    }
}

void notify_cleanup_complete() { SetEvent(cleanup_complete); }
#endif

} // namespace

void install_shutdown_handlers() {
#ifdef _WIN32
    cleanup_complete = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!cleanup_complete) {
        throw std::runtime_error("Cannot create shutdown completion event");
    }
    if (!SetConsoleCtrlHandler(handle_console_event, TRUE)) {
        CloseHandle(cleanup_complete);
        throw std::runtime_error("Cannot install console shutdown handler");
    }
    std::atexit(notify_cleanup_complete);
    for (const int signal : {SIGINT, SIGTERM}) {
        if (std::signal(signal, handle_signal) == SIG_ERR) {
            throw std::runtime_error("Cannot install shutdown signal handler");
        }
    }
#else
    struct sigaction action{};
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    for (const int signal : {SIGTERM, SIGINT, SIGPIPE, SIGQUIT}) {
        if (sigaction(signal, &action, nullptr) != 0) {
            throw std::runtime_error("Cannot install shutdown signal handler");
        }
    }
#endif
}

bool shutdown_requested() {
#ifdef _WIN32
    return signal_received != 0 || InterlockedCompareExchange(&console_shutdown, 0, 0) != 0;
#else
    return signal_received != 0;
#endif
}

} // namespace eventgamer
