#pragma once

#include <Poco/URI.h>
#include <Poco/Exception.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/StreamCopier.h>

#include <optional>
#include <sstream>
#include <string>

namespace handlers {

inline std::string readBody(Poco::Net::HTTPServerRequest& request) {
    std::ostringstream oss;
    Poco::StreamCopier::copyStream(request.stream(), oss);
    return oss.str();
}

inline Poco::JSON::Object::Ptr parseJsonBody(Poco::Net::HTTPServerRequest& request) {
    const std::string body = readBody(request);
    Poco::JSON::Parser parser;
    auto result = parser.parse(body);
    return result.extract<Poco::JSON::Object::Ptr>();
}

inline void sendJson(Poco::Net::HTTPServerResponse& response, Poco::Net::HTTPResponse::HTTPStatus status, const Poco::Dynamic::Var& payload) {
    response.setStatus(status);
    response.setContentType("application/json");
    std::ostream& ostr = response.send();
    Poco::JSON::Stringifier::stringify(payload, ostr);
}

inline std::optional<Poco::Int64> tryExtractId(const std::string& uri, const std::string& prefix) {
    const std::string pathOnly = uri.substr(0, uri.find('?'));
    if (pathOnly.rfind(prefix, 0) != 0) return std::nullopt;
    std::string tail = pathOnly.substr(prefix.size());
    if (tail.empty() || tail == "/") return std::nullopt;
    if (tail[0] == '/') tail = tail.substr(1);
    if (tail.empty()) return std::nullopt;
    try {
        return std::stoll(tail);
    } catch (...) {
        return std::nullopt;
    }
}

inline std::string getQueryParam(const Poco::Net::HTTPServerRequest& request, const std::string& key, const std::string& defaultValue = "") {
    const std::string uri = request.getURI();
    Poco::URI parsed(uri);
    const auto query = parsed.getQuery();
    if (query.empty()) return defaultValue;

    // Manual parse is simpler than depending on Poco's query parameter containers.
    std::size_t start = 0;
    while (start <= query.size()) {
        const std::size_t amp = query.find('&', start);
        const std::string part = query.substr(start, amp == std::string::npos ? std::string::npos : amp - start);
        if (part.empty()) break;

        const std::size_t eq = part.find('=');
        const std::string k = (eq == std::string::npos) ? part : part.substr(0, eq);
        if (k == key) {
            if (eq == std::string::npos) return "";
            return part.substr(eq + 1);
        }
        if (amp == std::string::npos) break;
        start = amp + 1;
    }
    return defaultValue;
}

} // namespace handlers
