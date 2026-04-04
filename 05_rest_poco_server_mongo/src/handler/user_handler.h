#pragma once

#include <Poco/Exception.h>
#include <Poco/JSON/Object.h>
#include <Poco/Logger.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Timestamp.h>

#include "../repository/user_repository.h"
#include "request_counter.h"
#include "rest_utils.h"

namespace handlers {

class UserHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override {
        Poco::Timestamp start;
        if (g_httpRequests) g_httpRequests->inc();
        auto& logger = Poco::Logger::get("Server");

        try {
            repository::UserRepository repository;
            const std::string uri = request.getURI();
            const std::string method = request.getMethod();
            auto id = tryExtractId(uri, "/api/v1/users");

            if (method == "GET" && !id.has_value()) {
                const std::string firstName = getQueryParam(request, "first_name");
                const std::string lastName = getQueryParam(request, "last_name");
                const std::string organization = getQueryParam(request, "organization");

                const bool hasAnyFilter = !firstName.empty() || !lastName.empty() || !organization.empty();
                if (hasAnyFilter) {
                    sendJson(response,
                             Poco::Net::HTTPResponse::HTTP_OK,
                             repository.searchBySubstrings(firstName, lastName, organization));
                } else {
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository.list());
                }
            } else if (method == "POST" && !id.has_value()) {
                auto payload = parseJsonBody(request);
                sendJson(response, Poco::Net::HTTPResponse::HTTP_CREATED, repository.create(payload));
            } else if (method == "GET" && id.has_value()) {
                auto item = repository.getById(*id);
                if (!item.has_value()) {
                    Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                    err->set("error", "user not found");
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, err);
                    if (g_httpErrors) g_httpErrors->inc();
                } else {
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository::UserRepository::toJson(*item));
                }
            } else if (method == "PUT" && id.has_value()) {
                auto payload = parseJsonBody(request);
                auto updated = repository.update(*id, payload);
                if (!updated.has_value()) {
                    Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                    err->set("error", "user not found");
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, err);
                    if (g_httpErrors) g_httpErrors->inc();
                } else {
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository::UserRepository::toJson(*updated));
                }
            } else if (method == "DELETE" && id.has_value()) {
                if (!repository.remove(*id)) {
                    Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                    err->set("error", "user not found");
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, err);
                    if (g_httpErrors) g_httpErrors->inc();
                } else {
                    response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
                    response.send();
                }
            } else {
                Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                err->set("error", "method not allowed");
                sendJson(response, Poco::Net::HTTPResponse::HTTP_METHOD_NOT_ALLOWED, err);
                if (g_httpErrors) g_httpErrors->inc();
            }
        } catch (const Poco::Exception& e) {
            Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
            err->set("error", e.displayText());
            sendJson(response, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, err);
            if (g_httpErrors) g_httpErrors->inc();
        } catch (const std::exception& e) {
            Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
            err->set("error", e.what());
            sendJson(response, Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, err);
            if (g_httpErrors) g_httpErrors->inc();
        }

        Poco::Timespan elapsed = Poco::Timestamp() - start;
        double seconds = static_cast<double>(elapsed.totalMicroseconds()) / 1000000.0;
        if (g_httpDuration) g_httpDuration->observe(seconds);
        logger.information("%d %s %s from %s, %.2f ms",
                           static_cast<int>(response.getStatus()),
                           request.getMethod(),
                           request.getURI(),
                           request.clientAddress().toString(),
                           seconds * 1000.0);
    }
};

} // namespace handlers
