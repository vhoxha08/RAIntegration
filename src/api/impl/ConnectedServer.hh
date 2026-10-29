#pragma once

#include "ServerBase.hh"

#include "services/Http.hh"

namespace ra {
namespace api {
namespace impl {

class ConnectedServer : public ServerBase
{
public:
    explicit ConnectedServer(const std::string& sHost);

    const char* Name() const noexcept override { return m_sHost.c_str(); }

    api::ResolveHash::Response ResolveHash(const api::ResolveHash::Request& request) override;
    api::UpdateAchievement::Response UpdateAchievement(const api::UpdateAchievement::Request& request) override;
    api::FetchAchievementInfo::Response FetchAchievementInfo(const api::FetchAchievementInfo::Request& request) override;
    api::FetchLeaderboardInfo::Response FetchLeaderboardInfo(const api::FetchLeaderboardInfo::Request& request) override;
    api::UpdateRichPresence::Response UpdateRichPresence(const api::UpdateRichPresence::Request& request) override;
    api::UploadBadge::Response UploadBadge(const api::UploadBadge::Request& request) override;

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
