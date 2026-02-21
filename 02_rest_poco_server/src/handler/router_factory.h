#pragma once

#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>

#include "auth_handler.h"
#include "hello_world_handler.h"
#include "helloworld_jwt_handler.h"
#include "metrics_handler.h"
#include "not_found_handler.h"
#include "swagger_handler.h"

namespace handlers {

class RouterFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    Poco::Net::HTTPRequestHandler* createRequestHandler(
        const Poco::Net::HTTPServerRequest& request) override {
        const std::string& uri = request.getURI();
        const std::string& method = request.getMethod();

        if (uri == "/api/v1/helloworld" && method == "GET") {
            return new HelloWorldHandler();
        }
        if (uri == "/api/v1/helloworld_jwt" && method == "GET") {
            return new HelloWorldJwtHandler();
        }
        if (uri == "/api/v1/auth" && method == "POST") {
            return new AuthHandler();
        }
        if (uri == "/swagger.yaml" && method == "GET") {
            return new SwaggerHandler();
        }
        if (uri == "/metrics" && method == "GET") {
            return new MetricsHandler();
        }
        return new NotFoundHandler();
    }
};

} // namespace handlers
