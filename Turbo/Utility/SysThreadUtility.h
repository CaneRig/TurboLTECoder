#include <thread>
#pragma once

// sets current thread to maximum priority
int SysSetHighestPriority(std::thread::native_handle_type);

#if _WIN32
#include <Windows.h>

int SysSetHighestPriority(std::thread::native_handle_type handle) {
	return SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL);
}
#elif __linux__
#include <pthread.h>

int SysSetHighestPriority(std::thread::native_handle_type handle) {
    sched_param param;
    param.sched_priority = { sched_get_priority_max(SCHED_FIFO) };
    //sched_get_priority_max(SCHED_FIFO)};

    return pthread_setschedparam(handle, SCHED_FIFO, &param);
}

#else
#error UNSUPPORTED SYSTEM
#endif 