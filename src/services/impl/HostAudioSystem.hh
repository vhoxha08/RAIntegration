#ifndef RA_SERVICES_HOST_AUDIO_SYSTEM_HH
#define RA_SERVICES_HOST_AUDIO_SYSTEM_HH
#pragma once

#include "services/IAudioSystem.hh"
#include "services/IFileSystem.hh"
#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

#include <functional>
#include <mutex>

namespace ra {
namespace services {
namespace impl {

/// <summary>
/// IAudioSystem that forwards to the host application. Sounds are silently dropped until the host installs
/// handlers.
/// </summary>
class HostAudioSystem : public IAudioSystem
{
public:
    /// <param name="sUtf8Path">Absolute path of a .wav file, with '/' separators.</param>
    using PlayFileHandler = std::function<void(const std::string& sUtf8Path)>;
    using BeepHandler = std::function<void()>;

    void SetHandlers(PlayFileHandler fPlayFile, BeepHandler fBeep)
    {
        std::lock_guard<std::mutex> lock(m_pMutex);
        m_fPlayFile = std::move(fPlayFile);
        m_fBeep = std::move(fBeep);
    }

    void PlayAudioFile(const std::wstring& sPath) const override
    {
        // paths are relative to the base directory, like WindowsAudioSystem (i.e. "Overlay\\unlock.wav")
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        std::string sFullPath = ra::util::String::Narrow(pFileSystem.BaseDirectory() + sPath);
        for (auto& c : sFullPath)
        {
            if (c == '\\')
                c = '/';
        }

        std::lock_guard<std::mutex> lock(m_pMutex);
        if (m_fPlayFile)
            m_fPlayFile(sFullPath);
    }

    void Beep() const override
    {
        std::lock_guard<std::mutex> lock(m_pMutex);
        if (m_fBeep)
            m_fBeep();
    }

private:
    mutable std::mutex m_pMutex;
    PlayFileHandler m_fPlayFile;
    BeepHandler m_fBeep;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_HOST_AUDIO_SYSTEM_HH
