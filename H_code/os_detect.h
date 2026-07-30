#ifndef OS_DETECT_H
#define OS_DETECT_H

#if defined(_WIN32) || defined(_WIN64)
    #define SF_OS_WINDOWS
#elif defined(__linux__)
    #define SF_OS_LINUX
#else
    #define SF_OS_UNKNOWN
#endif

#endif