/* Portable stand-in for Microsoft's CppUnitTest.h, used by the CMake build on non-MSVC
 * platforms. Implements the subset of Microsoft::VisualStudio::CppUnitTestFramework the
 * tests use, plus self-registration so TestMain.cpp can discover and run them. */
#ifndef RA_CPPUNITTEST_COMPAT_H
#define RA_CPPUNITTEST_COMPAT_H

#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <functional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace ra_test {

struct TestInfo
{
    const char* sClassName;
    const char* sMethodName;
    const char* sFile;
    int nLine;
    std::function<void()> fRun;
};

inline std::vector<TestInfo>& Registry()
{
    static std::vector<TestInfo> vTests;
    return vTests;
}

template<typename TClass>
struct Registrar
{
    Registrar(const char* sClassName, const char* sMethodName, void (*fInvoke)(TClass&),
              const char* sFile, int nLine)
    {
        Registry().push_back({sClassName, sMethodName, sFile, nLine, [fInvoke]() {
                                  TClass pInstance; // fresh instance per test, like the MS runner
                                  fInvoke(pInstance);
                              }});
    }
};

/* thrown by a failed assertion; caught by the runner */
struct AssertFailedException
{
    std::wstring sMessage;
    const char* sFile;
    int nLine;
};

} // namespace ra_test

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

struct __LineInfo
{
    const char* pszFileName;
    const char* pszFuncName;
    int lineNumber;
};

#define LINE_INFO() &::Microsoft::VisualStudio::CppUnitTestFramework::__LineInfo{__FILE__, __func__, __LINE__}

template<typename T>
class TestClass
{
public:
    typedef T ThisClass;
    virtual ~TestClass() = default;
};

/* ToString: the tests provide explicit specializations for their own enums/types */

template<typename Q>
static std::wstring ToString(const Q& q)
{
    if constexpr (std::is_enum_v<Q>)
        return std::to_wstring(static_cast<long long>(q));
    else if constexpr (std::is_integral_v<Q>)
        return std::to_wstring(q);
    else if constexpr (std::is_floating_point_v<Q>)
        return std::to_wstring(q);
    else
        return L"<object>";
}

template<typename Q>
static std::wstring ToString(const Q* q)
{
    std::wostringstream oss;
    oss << static_cast<const void*>(q);
    return oss.str();
}

template<typename Q>
static std::wstring ToString(Q* q)
{
    return ToString(static_cast<const Q*>(q));
}

template<> inline std::wstring ToString<bool>(const bool& b) { return b ? L"true" : L"false"; }
template<> inline std::wstring ToString<char>(const char& c) { return std::wstring(1, static_cast<wchar_t>(static_cast<unsigned char>(c))); }
template<> inline std::wstring ToString<wchar_t>(const wchar_t& c) { return std::wstring(1, c); }
template<> inline std::wstring ToString<std::string>(const std::string& s)
{
    std::wstring sWide;
    sWide.reserve(s.size());
    for (const char c : s)
        sWide.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
    return sWide;
}
template<> inline std::wstring ToString<std::wstring>(const std::wstring& s) { return s; }
template<> inline std::wstring ToString<char>(const char* s) { return s ? ToString(std::string(s)) : L"(null)"; }
template<> inline std::wstring ToString<wchar_t>(const wchar_t* s) { return s ? std::wstring(s) : L"(null)"; }
template<> inline std::wstring ToString<char>(char* s) { return ToString(static_cast<const char*>(s)); }
template<> inline std::wstring ToString<wchar_t>(wchar_t* s) { return ToString(static_cast<const wchar_t*>(s)); }

class Assert
{
public:
    [[noreturn]] static void Fail(const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        throw ra_test::AssertFailedException{message ? message : L"Assert::Fail",
                                             pLineInfo ? pLineInfo->pszFileName : nullptr,
                                             pLineInfo ? pLineInfo->lineNumber : 0};
    }

    template<typename T>
    static void AreEqual(const T& expected, const T& actual, const wchar_t* message = nullptr,
                         const __LineInfo* pLineInfo = nullptr)
    {
        if (!(expected == actual))
            FailEquality(ToString(expected), ToString(actual), message, pLineInfo);
    }

    static void AreEqual(double expected, double actual, double tolerance, const wchar_t* message = nullptr,
                         const __LineInfo* pLineInfo = nullptr)
    {
        if (std::fabs(expected - actual) > std::fabs(tolerance))
            FailEquality(ToString(expected), ToString(actual), message, pLineInfo);
    }

    static void AreEqual(float expected, float actual, float tolerance, const wchar_t* message = nullptr,
                         const __LineInfo* pLineInfo = nullptr)
    {
        if (std::fabs(expected - actual) > std::fabs(tolerance))
            FailEquality(ToString(expected), ToString(actual), message, pLineInfo);
    }

    static void AreEqual(const char* expected, const char* actual, bool ignoreCase = false,
                         const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (!StringsEqual(std::string(expected ? expected : ""), std::string(actual ? actual : ""), ignoreCase) ||
            (expected == nullptr) != (actual == nullptr))
            FailEquality(ToString(expected), ToString(actual), message, pLineInfo);
    }

    static void AreEqual(const wchar_t* expected, const wchar_t* actual, bool ignoreCase = false,
                         const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (!StringsEqual(std::wstring(expected ? expected : L""), std::wstring(actual ? actual : L""), ignoreCase) ||
            (expected == nullptr) != (actual == nullptr))
            FailEquality(ToString(expected), ToString(actual), message, pLineInfo);
    }

    static void AreEqual(const std::string& expected, const std::string& actual, bool ignoreCase = false,
                         const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (!StringsEqual(expected, actual, ignoreCase))
            FailEquality(ToString(expected), ToString(actual), message, pLineInfo);
    }

    static void AreEqual(const std::wstring& expected, const std::wstring& actual, bool ignoreCase = false,
                         const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (!StringsEqual(expected, actual, ignoreCase))
            FailEquality(expected, actual, message, pLineInfo);
    }

    template<typename T>
    static void AreNotEqual(const T& notExpected, const T& actual, const wchar_t* message = nullptr,
                            const __LineInfo* pLineInfo = nullptr)
    {
        if (notExpected == actual)
            Fail(BuildMessage(L"Assert::AreNotEqual failed. Value:<" + ToString(actual) + L">", message).c_str(),
                 pLineInfo);
    }

    static void AreNotEqual(const std::string& notExpected, const std::string& actual, bool ignoreCase = false,
                            const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (StringsEqual(notExpected, actual, ignoreCase))
            Fail(BuildMessage(L"Assert::AreNotEqual failed. Value:<" + ToString(actual) + L">", message).c_str(),
                 pLineInfo);
    }

    static void AreNotEqual(const std::wstring& notExpected, const std::wstring& actual, bool ignoreCase = false,
                            const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (StringsEqual(notExpected, actual, ignoreCase))
            Fail(BuildMessage(L"Assert::AreNotEqual failed. Value:<" + actual + L">", message).c_str(), pLineInfo);
    }

    static void IsTrue(bool condition, const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (!condition)
            Fail(BuildMessage(L"Assert::IsTrue failed", message).c_str(), pLineInfo);
    }

    static void IsFalse(bool condition, const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (condition)
            Fail(BuildMessage(L"Assert::IsFalse failed", message).c_str(), pLineInfo);
    }

    template<typename T>
    static void IsNull(const T* ptr, const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (ptr != nullptr)
            Fail(BuildMessage(L"Assert::IsNull failed", message).c_str(), pLineInfo);
    }

    template<typename T>
    static void IsNotNull(const T* ptr, const wchar_t* message = nullptr, const __LineInfo* pLineInfo = nullptr)
    {
        if (ptr == nullptr)
            Fail(BuildMessage(L"Assert::IsNotNull failed", message).c_str(), pLineInfo);
    }

private:
    static std::wstring BuildMessage(const std::wstring& sPrefix, const wchar_t* message)
    {
        return (message && *message) ? sPrefix + L" - " + message : sPrefix;
    }

    [[noreturn]] static void FailEquality(const std::wstring& sExpected, const std::wstring& sActual,
                                          const wchar_t* message, const __LineInfo* pLineInfo)
    {
        Fail(BuildMessage(L"Assert::AreEqual failed. Expected:<" + sExpected + L"> Actual:<" + sActual + L">",
                          message).c_str(),
             pLineInfo);
    }

    template<typename TString>
    static bool StringsEqual(const TString& a, const TString& b, bool ignoreCase)
    {
        if (!ignoreCase)
            return a == b;
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
        {
            if (std::towlower(static_cast<wint_t>(a[i])) != std::towlower(static_cast<wint_t>(b[i])))
                return false;
        }
        return true;
    }
};

class Logger
{
public:
    static void WriteMessage(const wchar_t*) noexcept {}
    static void WriteMessage(const char*) noexcept {}
};

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#define TEST_CLASS(className) \
    class className; \
    [[maybe_unused]] static const char* ra_test_class_name(const className*) { return #className; } \
    class className : public ::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<className>

#define TEST_METHOD(methodName) \
    static void ra_test_invoke_##methodName(ThisClass& pInstance) { pInstance.methodName(); } \
    static inline const ::ra_test::Registrar<ThisClass> ra_test_registrar_##methodName{ \
        ra_test_class_name(static_cast<const ThisClass*>(nullptr)), #methodName, \
        &ThisClass::ra_test_invoke_##methodName, __FILE__, __LINE__}; \
    void methodName()

#endif /* !RA_CPPUNITTEST_COMPAT_H */
