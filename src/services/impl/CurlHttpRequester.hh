#ifndef RA_SERVICES_CURL_HTTP_REQUESTER_HH
#define RA_SERVICES_CURL_HTTP_REQUESTER_HH
#pragma once

#include "services/IHttpRequester.hh"

namespace ra {
namespace services {
namespace impl {

/// <summary>
/// IHttpRequester backed by libcurl (macOS and Linux).
/// </summary>
/// <remarks>
/// Request() returns the HTTP status code when a response was received. Transport-level failures are returned
/// as <see cref="CurlErrorBase" /> + CURLcode so they can't be confused with HTTP status codes.
/// </remarks>
class CurlHttpRequester : public IHttpRequester
{
public:
    static constexpr unsigned int CurlErrorBase = 10000;

    CurlHttpRequester() noexcept;
    ~CurlHttpRequester() noexcept override;
    CurlHttpRequester(const CurlHttpRequester&) noexcept = delete;
    CurlHttpRequester& operator=(const CurlHttpRequester&) noexcept = delete;
    CurlHttpRequester(CurlHttpRequester&&) noexcept = delete;
    CurlHttpRequester& operator=(CurlHttpRequester&&) noexcept = delete;

    void SetUserAgent(const std::string& sUserAgent) override { m_sUserAgent = sUserAgent; }

    unsigned int Request(const Http::Request& pRequest, TextWriter& pContentWriter) const override;

    bool IsRetryable(unsigned int nStatusCode) const noexcept override;

    std::string GetStatusCodeText(unsigned int nStatusCode) const override;

private:
    std::string m_sUserAgent;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_CURL_HTTP_REQUESTER_HH
