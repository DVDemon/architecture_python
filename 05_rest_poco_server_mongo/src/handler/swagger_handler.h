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
  description: REST API server with POCO + MongoDB
  version: 2.0.0
servers:
  - url: /
paths:
  /api/v1/organizations:
    get:
      summary: List organizations
      responses:
        '200':
          description: Success
          content:
            application/json:
              schema:
                type: array
                items:
                  $ref: '#/components/schemas/Organization'
    post:
      summary: Create organization
      requestBody:
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/OrganizationCreate'
      responses:
        '201':
          description: Created
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/Organization'
  /api/v1/organizations/{id}:
    parameters:
      - name: id
        in: path
        required: true
        schema:
          type: integer
          format: int64
    get:
      summary: Get organization by id
      responses:
        '200':
          description: Success
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/Organization'
        '404':
          description: Not found
    put:
      summary: Update organization
      requestBody:
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/OrganizationCreate'
      responses:
        '200':
          description: Updated
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/Organization'
        '404':
          description: Not found
    delete:
      summary: Delete organization
      responses:
        '204':
          description: Deleted
        '404':
          description: Not found
  /api/v1/users:
    get:
      summary: List users
      parameters:
        - name: first_name
          in: query
          required: false
          schema:
            type: string
          description: Case-insensitive substring match on first_name (MongoDB regex)
        - name: last_name
          in: query
          required: false
          schema:
            type: string
          description: Case-insensitive substring match on last_name (MongoDB regex)
        - name: organization
          in: query
          required: false
          schema:
            type: string
          description: Case-insensitive substring match on organization name (MongoDB regex)
      responses:
        '200':
          description: Success
          content:
            application/json:
              schema:
                type: array
                items:
                  $ref: '#/components/schemas/User'
    post:
      summary: Create user
      requestBody:
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/UserCreate'
      responses:
        '201':
          description: Created
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/User'
  /api/v1/users/{id}:
    parameters:
      - name: id
        in: path
        required: true
        schema:
          type: integer
          format: int64
    get:
      summary: Get user by id
      responses:
        '200':
          description: Success
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/User'
        '404':
          description: Not found
    put:
      summary: Update user
      requestBody:
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/UserCreate'
      responses:
        '200':
          description: Updated
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/User'
        '404':
          description: Not found
    delete:
      summary: Delete user
      responses:
        '204':
          description: Deleted
        '404':
          description: Not found
components:
  schemas:
    Organization:
      type: object
      properties:
        id:
          type: integer
          format: int64
        name:
          type: string
        address:
          type: string
      required: [id, name, address]
    OrganizationCreate:
      type: object
      properties:
        name:
          type: string
        address:
          type: string
      required: [name, address]
    User:
      type: object
      properties:
        id:
          type: integer
          format: int64
        first_name:
          type: string
        last_name:
          type: string
        title:
          type: string
        email:
          type: string
        login:
          type: string
        password:
          type: string
        organization_id:
          type: integer
          format: int64
          nullable: true
      required: [id, first_name, last_name, title, email, login, password]
    UserCreate:
      type: object
      properties:
        first_name:
          type: string
        last_name:
          type: string
        title:
          type: string
        email:
          type: string
        login:
          type: string
        password:
          type: string
        organization_id:
          type: integer
          format: int64
          nullable: true
      required: [first_name, last_name, title, email, login, password]
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
