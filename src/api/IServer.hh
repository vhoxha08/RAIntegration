#ifndef RA_API_ISERVER_HH
#define RA_API_ISERVER_HH
#pragma once

#include "api/FetchAchievementInfo.hh"
#include "api/FetchLeaderboardInfo.hh"
#include "api/ResolveHash.hh"
#include "api/UpdateAchievement.hh"
#include "api/UpdateRichPresence.hh"
#include "api/UploadBadge.hh"

namespace ra {
namespace api {

class IServer
{
public:
    virtual const char* Name() const noexcept = 0;

    // === game functions ===
    virtual api::ResolveHash::Response ResolveHash(const api::ResolveHash::Request& request) = 0;
    virtual api::UpdateAchievement::Response UpdateAchievement(const api::UpdateAchievement::Request& request) = 0;
    virtual api::FetchAchievementInfo::Response FetchAchievementInfo(const api::FetchAchievementInfo::Request& request) = 0;
    virtual api::FetchLeaderboardInfo::Response FetchLeaderboardInfo(const api::FetchLeaderboardInfo::Request& request) = 0;
    virtual api::UpdateRichPresence::Response UpdateRichPresence(const api::UpdateRichPresence::Request& request) = 0;

    // === other functions ===
    virtual api::UploadBadge::Response UploadBadge(const api::UploadBadge::Request& request) = 0;

protected:
    IServer() noexcept = default;

public:
    virtual ~IServer() noexcept = default;
    IServer(const IServer&) = delete;
    IServer& operator=(const IServer&) = delete;
    IServer(IServer&&) noexcept = delete;
    IServer& operator=(IServer&&) noexcept = delete;
};

} // namespace api
} // namespace ra

#endif !RA_API_ISERVER_HH
