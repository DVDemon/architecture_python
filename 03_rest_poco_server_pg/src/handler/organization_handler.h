#pragma once

#include <Poco/Data/DataException.h>
#include <Poco/Exception.h>
#include <Poco/JSON/Object.h>
#include <Poco/Logger.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Timestamp.h>

#include "../repository/organization_repository.h"
#include "request_counter.h"
#include "rest_utils.h"

namespace handlers {

class OrganizationHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override {
        Poco::Timestamp start;
        if (g_httpRequests) g_httpRequests->inc();
        auto& logger = Poco::Logger::get("Server");

        try {
            repository::OrganizationRepository repository;
            const std::string uri = request.getURI();
            const std::string method = request.getMethod();
            auto id = tryExtractId(uri, "/api/v1/organizations");

            if (method == "GET" && !id.has_value()) {
                sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository.list());
            } else if (method == "POST" && !id.has_value()) {
                auto payload = parseJsonBody(request);
                sendJson(response, Poco::Net::HTTPResponse::HTTP_CREATED, repository.create(payload));
            } else if (method == "GET" && id.has_value()) {
                auto item = repository.getById(*id);
                if (!item.has_value()) {
                    Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                    err->set("error", "organization not found");
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, err);
                    if (g_httpErrors) g_httpErrors->inc();
                } else {
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository::OrganizationRepository::toJson(*item));
                }
            } else if (method == "PUT" && id.has_value()) {
                auto payload = parseJsonBody(request);
                auto updated = repository.update(*id, payload);
                if (!updated.has_value()) {
                    Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                    err->set("error", "organization not found");
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, err);
                    if (g_httpErrors) g_httpErrors->inc();
                } else {
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository::OrganizationRepository::toJson(*updated));
                }
            } else if (method == "DELETE" && id.has_value()) {
                if (!repository.remove(*id)) {
                    Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                    err->set("error", "organization not found");
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
