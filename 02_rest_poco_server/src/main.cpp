#include <iostream>
#include <string>

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Environment.h>
#include <Poco/Logger.h>
#include <Poco/NumberParser.h>
#include <Poco/Prometheus/Counter.h>
#include <Poco/Prometheus/Histogram.h>
#include <Poco/Prometheus/Registry.h>
#include <Poco/Util/ServerApplication.h>

#include "handler/auth_config.h"
#include "handler/router_factory.h"
#include "handler/request_counter.h"

using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;

namespace {

void configureLogging() {
    std::string level = Environment::get("LOG_LEVEL", "information");

    Message::Priority prio = Message::PRIO_INFORMATION;
    if (level == "trace") prio = Message::PRIO_TRACE;
    else if (level == "debug") prio = Message::PRIO_DEBUG;
    else if (level == "information" || level == "info") prio = Message::PRIO_INFORMATION;
    else if (level == "notice") prio = Message::PRIO_NOTICE;
    else if (level == "warning" || level == "warn") prio = Message::PRIO_WARNING;
    else if (level == "error") prio = Message::PRIO_ERROR;
    else if (level == "critical") prio = Message::PRIO_CRITICAL;
    else if (level == "fatal") prio = Message::PRIO_FATAL;
    else if (level == "none") prio = static_cast<Message::Priority>(Message::PRIO_FATAL + 1);

    Logger::root().setLevel(prio);
}

} // namespace

namespace handlers {

Poco::Prometheus::Counter* g_httpRequests = nullptr;
Poco::Prometheus::Counter* g_httpErrors = nullptr;
Poco::Prometheus::Histogram* g_httpDuration = nullptr;
std::string g_jwtSecret;

} // namespace handlers

class ServerApp : public ServerApplication {
protected:
    int main(const std::vector<std::string>&) override {
        configureLogging();
        auto& logger = Logger::get("Server");

        unsigned short port = 8080;
        if (Environment::has("PORT")) {
            try {
                port = static_cast<unsigned short>(NumberParser::parse(Environment::get("PORT")));
            } catch (const Exception& e) {
                logger.warning("Invalid PORT, using default 8080: %s", e.displayText());
            }
        }

        logger.information("Starting server on port %hu", port);

        static Prometheus::Counter httpRequests("http_requests_total");
        httpRequests.help("Total number of HTTP requests");
        handlers::g_httpRequests = &httpRequests;

        static Prometheus::Histogram httpDuration("http_request_duration_seconds");
        httpDuration.help("HTTP request duration in seconds");
        httpDuration.buckets({0.0001, 0.0005, 0.001, 0.0025, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0});
        handlers::g_httpDuration = &httpDuration;

        static Prometheus::Counter httpErrors("http_errors_total");
        httpErrors.help("Total number of HTTP error responses (4xx, 5xx)");
        handlers::g_httpErrors = &httpErrors;

        handlers::g_jwtSecret = Environment::get("JWT_SECRET", "");

        ServerSocket svs(port);
        HTTPServer srv(new handlers::RouterFactory(), svs, new HTTPServerParams);
        srv.start();

        logger.information("Server started. Endpoints: GET /api/v1/helloworld, GET /api/v1/helloworld_jwt, POST /api/v1/auth, GET /metrics, GET /swagger.yaml");

        waitForTerminationRequest();
        srv.stop();

        return Application::EXIT_OK;
    }
};

POCO_SERVER_MAIN(ServerApp)
