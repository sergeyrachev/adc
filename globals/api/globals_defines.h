#if !defined(_GLOBALS_DEFINES_H_)
#define _GLOBALS_DEFINES_H_

#include "stdio.h"
#include "conio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "sys/timeb.h"
#include "cstring"

#include "windows.h"

#pragma warning(disable:4996)

#define safe_delete( __pVoid__ ) {\
    if (__pVoid__) {\
        delete __pVoid__;\
        __pVoid__ = NULL;\
    }\
};

#define safe_delete_array( __pVoid__ ) {\
    if ( __pVoid__ ) {\
        delete[] __pVoid__;\
        __pVoid__ = NULL;\
    }\
};

#define safe_release( __pObject__ ) { \
    if ( __pObject__ ) {  \
        __pObject__->Release(); \
        __pObject__ = NULL;   \
    } \
}

#define safe_copy_string( __pDst__, __pSrc__ ) { \
    safe_delete_array(__pDst__);               \
    if ( __pSrc__ ) { \
        __pDst__ = new char[strlen(__pSrc__) + 1]; \
        strcpy((char*)__pDst__, (char*)__pSrc__); \
    }                                        \
}

int64 inline GetCurrentTimeInMilliseconds()
{
    int64 i64CurrentTime;

#if defined(__linux__) || defined(__APPLE__)
    timeval tv;
    gettimeofday(&tv, NULL);

    i64CurrentTime = (tv.tv_sec * 1000 + tv.tv_usec/1000);
#else 
    struct _timeb tstruct;
    _ftime( &tstruct );
    i64CurrentTime = tstruct.time * 1000 + tstruct.millitm;

#endif

    return i64CurrentTime;
}
 
#define FORMATI64 "%I64d"
         
void inline DEBUG_OUT(const char *fmt, ...)
{
#if defined (_DEBUG) || defined (DEBUG)
    char buf[2048];

    va_list vl;

    sprintf(buf, FORMATI64": ", GetCurrentTimeInMilliseconds());

    va_start(vl,fmt);
    vsprintf(&buf[strlen(buf)], fmt, vl);
    va_end(vl);

    OutputDebugStringA(buf);
#endif
}

inline int16 swap16(int16 x) 
{
    uint8* px = (uint8*)&x;
    int16 y = 0;
    uint8* py = (uint8*)&y;
    py[0] = px[1];
    py[1] = px[0];
    return y;
}

// 
// int8 WaitForUserInput( uint32_t uiMilliseconds )
// {
// #if defined(__linux__) || defined(__APPLE__)
//     struct timeval tv;
//     fd_set fds;
// 
//     tv.tv_sec = uiMilliseconds/1000;
//     tv.tv_usec = (uiMilliseconds % 1000) * 1000;
// 
//     int fd = fileno(stdin);
// 
//     FD_ZERO(&fds);
//     FD_SET(fd, &fds);
// 
//     int res = select(fd+1, &fds, NULL, NULL, uiMilliseconds != MCNET_THREAD_WAIT_INFINITE ? &tv : NULL);
//     if ( res > 0 && FD_ISSET(fd, &fds) ) {
//         return getchar();
//     }
//     return 0;
// #else
//     HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
// 
//     int64 iStartLoop = GetCurrentTimeInMilliseconds();
// 
//     while ( WaitForSingleObject(stdIn, uiMilliseconds ) == WAIT_OBJECT_0 ) {
// 
//         DWORD cNumRead = 0;
//         INPUT_RECORD irInBuf;
// 
//         if ( ReadConsoleInput( stdIn, &irInBuf, 1, &cNumRead) ) {
// 
//             if ( irInBuf.EventType == KEY_EVENT && irInBuf.Event.KeyEvent.bKeyDown ) {
//                 return irInBuf.Event.KeyEvent.uChar.AsciiChar;
//             }
//         }
// 
//         int64 iNow = GetCurrentTimeInMilliseconds();
// 
//         if ( iNow - iStartLoop >= uiMilliseconds ) {
//             break;
//         }
//     };
//     return 0;
// #endif
// }

#endif //guard
