#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Logger.h>
#include <Poco/Timestamp.h>
#include <Poco/JWT/Token.h>
#include <Poco/JWT/Signer.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Stringifier.h>

#include "auth_config.h"
#include "request_counter.h"

#include <sstream>

namespace handlers {

class HelloWorldJwtHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response) override {
        Poco::Timestamp start;
        if (g_httpRequests) g_httpRequests->inc();

        std::string authHeader = request.get("Authorization", "");
        if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            response.setContentType("application/json");
            std::ostream& ostr = response.send();
            ostr << R"({"error":"Bearer token required"})";
            if (g_httpErrors) g_httpErrors->inc();
            Poco::Timespan duration = Poco::Timestamp() - start;
            double seconds = static_cast<double>(duration.totalMicroseconds()) / 1000000.0;
            if (g_httpDuration) g_httpDuration->observe(seconds);
            Poco::Logger::get("Server").warning("401 GET /api/v1/helloworld_jwt from %s - Bearer token required, %.2f ms",
                                                request.clientAddress().toString(), seconds * 1000.0);
            return;
        }

        std::string jwt = authHeader.substr(7);

        if (g_jwtSecret.empty()) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            response.setContentType("application/json");
            std::ostream& ostr = response.send();
            ostr << R"({"error":"JWT_SECRET not configured"})";
            if (g_httpErrors) g_httpErrors->inc();
            Poco::Timespan duration = Poco::Timestamp() - start;
            double seconds = static_cast<double>(duration.totalMicroseconds()) / 1000000.0;
            if (g_httpDuration) g_httpDuration->observe(seconds);
            Poco::Logger::get("Server").error("500 GET /api/v1/helloworld_jwt from %s - JWT_SECRET not configured, %.2f ms",
                                              request.clientAddress().toString(), seconds * 1000.0);
            return;
        }

        try {
            Poco::JWT::Signer signer(g_jwtSecret);
            Poco::JWT::Token token = signer.verify(jwt);

            std::string username = token.getSubject();
            Poco::Timestamp iat = token.getIssuedAt();

            Poco::JSON::Object json;
            json.set("message", "Hello, World!");
            json.set("username", username);
            json.set("issued_at", static_cast<Poco::Int64>(iat.epochMicroseconds() / 1000000));

            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
            response.setContentType("application/json");
            std::ostream& ostr = response.send();
            Poco::JSON::Stringifier::stringify(json, ostr);
        } catch (const std::exception&) {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_UNAUTHORIZED);
            response.setContentType("application/json");
            std::ostream& ostr = response.send();
            ostr << R"({"error":"invalid or expired token"})";
            if (g_httpErrors) g_httpErrors->inc();
        }

        Poco::Timespan duration = Poco::Timestamp() - start;
        double seconds = static_cast<double>(duration.totalMicroseconds()) / 1000000.0;
        if (g_httpDuration) g_httpDuration->observe(seconds);
        auto& logger = Poco::Logger::get("Server");
        int status = response.getStatus();
        if (status == 200) {
            logger.information("200 GET /api/v1/helloworld_jwt from %s, %.2f ms",
                              request.clientAddress().toString(), seconds * 1000.0);
        } else {
            logger.warning("%d GET /api/v1/helloworld_jwt from %s - invalid or expired token, %.2f ms",
                           status, request.clientAddress().toString(), seconds * 1000.0);
        }
    }
};

} // namespace handlers
