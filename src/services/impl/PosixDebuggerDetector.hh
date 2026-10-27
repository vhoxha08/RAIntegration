#ifndef RA_SERVICES_POSIX_DEBUGGER_DETECTOR_HH
#define RA_SERVICES_POSIX_DEBUGGER_DETECTOR_HH
#pragma once

#include "services/IDebuggerDetector.hh"

namespace ra {
namespace services {
namespace impl {

/// <summary>
/// IDebuggerDetector for macOS and Linux. Mirrors WindowsDebuggerDetector: reports a traced process (release
/// builds only) or a running memory editor.
/// </summary>
class PosixDebuggerDetector : public IDebuggerDetector
{
public:
    bool IsDebuggerPresent() const override;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_POSIX_DEBUGGER_DETECTOR_HH
