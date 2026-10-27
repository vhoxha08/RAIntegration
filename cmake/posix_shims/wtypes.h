/* Stand-in for the Windows <wtypes.h> on non-Windows builds.
 * RAInterface/RA_Interface.h includes it only for these handle and integer types;
 * window and menu handles are opaque pointers supplied by the host. */
#ifndef RA_POSIX_SHIM_WTYPES_H
#define RA_POSIX_SHIM_WTYPES_H

#include <stdint.h>

#include <wchar.h>

typedef void* HWND;
typedef void* HMENU;
typedef void* HDC;
typedef intptr_t LPARAM;
typedef uintptr_t WPARAM;
typedef unsigned char BYTE;
typedef int BOOL;
#ifndef TRUE
 #define TRUE 1
#endif
#ifndef FALSE
 #define FALSE 0
#endif
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef const wchar_t* LPCWSTR;

typedef struct tagRECT
{
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;

#endif /* RA_POSIX_SHIM_WTYPES_H */
