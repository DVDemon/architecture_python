#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Logger.h>
#include <Poco/Timestamp.h>

#include "request_counter.h"

#include <string>

namespace handlers {

inline const std::string SWAGGER_YAML = R"(openapi: 3.0.3
info:
  title: Poco Template Server API
  description: Simple REST API server built with POCO C++
  version: 1.0.0
servers:
  - url: /
paths:
  /api/v1/helloworld:
    get:
      summary: Hello World
      description: Returns a simple greeting message
      operationId: getHelloWorld
      responses:
        '200':
          description: Success
          content:
            application/json:
              schema:
                type: object
                properties:
                  message:
                    type: string
                    example: "Hello, World!"
  /api/v1/auth:
    post:
      summary: Get JWT token
      description: Authenticate with Basic auth, receive JWT token
      operationId: postAuth
      security:
        - basicAuth: []
      responses:
        '200':
          description: Success, returns JWT token
          content:
            application/json:
              schema:
                type: object
                properties:
                  token:
                    type: string
        '401':
          description: Authentication required
  /api/v1/helloworld_jwt:
    get:
      summary: Hello World (JWT protected)
      description: Returns greeting with token data. Requires Bearer token.
      operationId: getHelloWorldJwt
      security:
        - bearerAuth: []
      responses:
        '200':
          description: Success
          content:
            application/json:
              schema:
                type: object
                properties:
                  message:
                    type: string
                  username:
                    type: string
                  issued_at:
                    type: integer
                    format: int64
        '401':
          description: Invalid or missing token
components:
  securitySchemes:
    basicAuth:
      type: http
      scheme: basic
    bearerAuth:
      type: http
      scheme: bearer
      bearerFormat: JWT
)";

class SwaggerHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response) override {
        Poco::Timestamp start;
        if (g_httpRequests) g_httpRequests->inc();

        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
        response.setContentType("application/x-yaml");
        response.setContentLength(static_cast<std::streamsize>(SWAGGER_YAML.size()));
        std::ostream& ostr = response.send();
        ostr << SWAGGER_YAML;

        Poco::Timespan elapsed = Poco::Timestamp() - start;
        double seconds = static_cast<double>(elapsed.totalMicroseconds()) / 1000000.0;
        if (g_httpDuration) g_httpDuration->observe(seconds);
        auto& logger = Poco::Logger::get("Server");
        logger.information("200 GET /swagger.yaml from %s, %.2f ms",
                          request.clientAddress().toString(), seconds * 1000.0);
    }
};

} // namespace handlers
