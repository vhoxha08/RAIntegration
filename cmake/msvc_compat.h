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
/* rcheevos/src/util/rc_compat.h treats any compiler without __STDC_VERSION__ (i.e. C++ on
 * clang/gcc) as C89 and redirects these to rc_* fallbacks - rc_snprintf ignores the buffer
 * size. Defining them as themselves makes its "#ifndef name" guards keep the real functions. */
#define snprintf snprintf
#define strcasecmp strcasecmp
#define strncasecmp strncasecmp
#define strdup strdup

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cwchar>
#include <cstring>
#include <string>

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

inline int wctomb_s(int* pRetValue, char* sBuffer, size_t /*nBufferSize*/, wchar_t wc)
{
    const int nResult = std::wctomb(sBuffer, wc);
    if (pRetValue)
        *pRetValue = nResult;
    return (nResult < 0) ? EILSEQ : 0;
}

/* "NAME=value" form only */
inline int _putenv(const char* sAssignment)
{
    const char* pEquals = std::strchr(sAssignment, '=');
    if (!pEquals)
        return -1;
    const std::string sName(sAssignment, pEquals);
    return setenv(sName.c_str(), pEquals + 1, 1);
}

#define _tzset tzset

/* only the explicit-size overloads are used: (buffer, size, format, ...) */
#define swprintf_s std::swprintf
#define sprintf_s std::snprintf
#endif /* __cplusplus */

#endif /* !_MSC_VER */

#endif /* !RA_MSVC_COMPAT_H */
