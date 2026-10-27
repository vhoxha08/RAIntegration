#ifndef RA_SERVICES_HOST_CLIPBOARD_HH
#define RA_SERVICES_HOST_CLIPBOARD_HH
#pragma once

#include "services/IClipboard.hh"

#include "util/Strings.hh"

#include <functional>
#include <mutex>

namespace ra {
namespace services {
namespace impl {

/// <summary>
/// IClipboard that forwards to the host application (which already owns a windowing library such as SDL).
/// Until the host installs handlers, the text is kept in-process so copy/paste inside the toolkit still works.
/// </summary>
class HostClipboard : public IClipboard
{
public:
    using SetTextHandler = std::function<void(const std::string& sUtf8)>;
    using GetTextHandler = std::function<std::string()>;

    void SetHandlers(SetTextHandler fSetText, GetTextHandler fGetText)
    {
        std::lock_guard<std::mutex> lock(m_pMutex);
        m_fSetText = std::move(fSetText);
        m_fGetText = std::move(fGetText);
    }

    void SetText(const std::wstring& sValue) const override
    {
        std::lock_guard<std::mutex> lock(m_pMutex);
        if (m_fSetText)
            m_fSetText(ra::util::String::Narrow(sValue));
        else
            m_sFallback = sValue;
    }

    std::wstring GetText() const override
    {
        std::lock_guard<std::mutex> lock(m_pMutex);
        if (m_fGetText)
            return ra::util::String::Widen(m_fGetText());

        return m_sFallback;
    }

private:
    mutable std::mutex m_pMutex;
    SetTextHandler m_fSetText;
    GetTextHandler m_fGetText;
    mutable std::wstring m_sFallback;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_HOST_CLIPBOARD_HH
