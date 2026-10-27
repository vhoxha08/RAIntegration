/* Force-included by the CMake build on non-MSVC compilers.
 * Stubs out Microsoft-specific annotations so the sources compile unchanged. */
#ifndef RA_MSVC_COMPAT_H
#define RA_MSVC_COMPAT_H

#ifndef _MSC_VER

#ifdef __cplusplus
 #include <cstddef>
 #include <cstdint>
#else
 #include <stddef.h>
 #include <stdint.h>
#endif

/* Win32 handle and integer types used in public interfaces (opaque on other platforms) */
#ifndef _WIN32
 #include "posix_shims/wtypes.h"
#endif

/* SAL annotations */
#define _In_
#define _In_z_
#define _In_opt_
#define _Out_
#define _Out_opt_
#define _Inout_
#define _Inout_opt_
#define _Use_decl_annotations_
#define _Success_(expr)
#define _Printf_format_string_

/* MSVC STL helper macros */
#ifndef _NODISCARD
 #define _NODISCARD [[nodiscard]]
#endif
#ifdef __cplusplus
 #define _FALLTHROUGH [[fallthrough]]
 #define __fallthrough [[fallthrough]]
#endif
#define __FUNC__ __PRETTY_FUNCTION__
#define _CRT_WIDE_(s) L ## s
#define _CRT_WIDE(s) _CRT_WIDE_(s)

#ifdef __cplusplus
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cwchar>

/* CRT functions without a portable spelling */
inline void _wassert(const wchar_t* sMessage, const wchar_t* sFile, unsigned nLine)
{
    std::fwprintf(stderr, L"Assertion failed: %ls (%ls:%u)\n", sMessage, sFile, nLine);
    std::abort();
}

inline int localtime_s(struct tm* pTm, const time_t* pTime)
{
    return localtime_r(pTime, pTm) ? 0 : errno;
}

inline int _wtoi(const wchar_t* sValue)
{
    return static_cast<int>(std::wcstol(sValue, nullptr, 10));
}

/* only the explicit-size overloads are used: (buffer, size, format, ...) */
#define swprintf_s std::swprintf
#define sprintf_s std::snprintf
#endif /* __cplusplus */

#endif /* !_MSC_VER */

#endif /* !RA_MSVC_COMPAT_H */
