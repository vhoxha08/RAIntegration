#include "CurlHttpRequester.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <curl/curl.h>

namespace ra {
namespace services {
namespace impl {

CurlHttpRequester::CurlHttpRequester() noexcept
{
    // reference counted by libcurl; safe to call more than once
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

CurlHttpRequester::~CurlHttpRequester() noexcept
{
    curl_global_cleanup();
}

static size_t WriteToTextWriter(char* pData, size_t nSize, size_t nCount, void* pUserData)
{
    auto* pContentWriter = static_cast<TextWriter*>(pUserData);
    const size_t nBytes = nSize * nCount;
    pContentWriter->Write(std::string(pData, nBytes));
    return nBytes;
}

unsigned int CurlHttpRequester::Request(const Http::Request& pRequest, TextWriter& pContentWriter) const
{
    CURL* pCurl = curl_easy_init();
    if (pCurl == nullptr)
        return CurlErrorBase + CURLE_FAILED_INIT;

    std::string sUrl = pRequest.GetUrl();
    const auto& sQueryString = pRequest.GetQueryString();
    if (!sQueryString.empty())
    {
        sUrl.push_back('?');
        sUrl += sQueryString;
    }

    curl_easy_setopt(pCurl, CURLOPT_URL, sUrl.c_str());
    curl_easy_setopt(pCurl, CURLOPT_USERAGENT, m_sUserAgent.c_str());
    curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L); // requests are made from background threads
    curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(pCurl, CURLOPT_ACCEPT_ENCODING, ""); // accept any encoding curl supports

    // WinHTTP follows redirects by default; do the same, but only to http(s)
    curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(pCurl, CURLOPT_MAXREDIRS, 5L);
#if LIBCURL_VERSION_NUM >= 0x075500 // 7.85.0
    curl_easy_setopt(pCurl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(pCurl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
    curl_easy_setopt(pCurl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(pCurl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, WriteToTextWriter);
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &pContentWriter);

    const std::string sContentTypeHeader = "Content-Type: " + pRequest.GetContentType();
    curl_slist* pHeaders = curl_slist_append(nullptr, sContentTypeHeader.c_str());
    curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pHeaders);

    const auto& sPostData = pRequest.GetPostData();
    if (!sPostData.empty())
    {
        curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
        curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, sPostData.data());
        curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(sPostData.length()));
    }

    unsigned int nStatusCode = 0;
    const CURLcode nResult = curl_easy_perform(pCurl);
    if (nResult == CURLE_OK)
    {
        long nHttpStatus = 0;
        curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &nHttpStatus);
        nStatusCode = gsl::narrow_cast<unsigned int>(nHttpStatus);
    }
    else
    {
        nStatusCode = CurlErrorBase + gsl::narrow_cast<unsigned int>(nResult);
    }

    curl_slist_free_all(pHeaders);
    curl_easy_cleanup(pCurl);

    return nStatusCode;
}

bool CurlHttpRequester::IsRetryable(unsigned int nStatusCode) const noexcept
{
    // mirrors WindowsHttpRequester::IsRetryable
    switch (nStatusCode)
    {
        case 0:                                            // Not attempted
        case 200:                                          // Success
        case CurlErrorBase + CURLE_OPERATION_TIMEDOUT:     // Timeout
        case CurlErrorBase + CURLE_COULDNT_RESOLVE_HOST:   // DNS lookup failed
        case CurlErrorBase + CURLE_COULDNT_RESOLVE_PROXY:  // DNS lookup failed (proxy)
        case CurlErrorBase + CURLE_COULDNT_CONNECT:        // Could not connect
        case CurlErrorBase + CURLE_SSL_CONNECT_ERROR:      // Handshake failed
        case CurlErrorBase + CURLE_SEND_ERROR:             // Connection reset while sending
        case CurlErrorBase + CURLE_RECV_ERROR:             // Connection reset while receiving
        case CurlErrorBase + CURLE_GOT_NOTHING:            // Connection closed without a response
        case CurlErrorBase + CURLE_PARTIAL_FILE:           // Lost connection during request
        case CurlErrorBase + CURLE_HTTP2:                  // HTTP/2 framing error
        case CurlErrorBase + CURLE_HTTP2_STREAM:           // HTTP/2 stream error
            return true;

        default:
            return false;
    }
}

std::string CurlHttpRequester::GetStatusCodeText(unsigned int nStatusCode) const
{
    if (nStatusCode >= CurlErrorBase)
        return curl_easy_strerror(static_cast<CURLcode>(nStatusCode - CurlErrorBase));

    switch (nStatusCode)
    {
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Moved Temporarily";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 408: return "Request Time-out";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Large";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "";
    }
}

} // namespace impl
} // namespace services
} // namespace ra
