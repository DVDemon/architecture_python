#ifndef GRAPHQLHANDLER_H
#define GRAPHQLHANDLER_H

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"

#include <graphqlservice/JSONResponse.h>

#include "../../database/ServiceMock.h"

#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>

#include <iostream>
#include <iterator>
#include <stdexcept>

using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

class GraphQLHandler : public HTTPRequestHandler
{
public:
    GraphQLHandler(const std::string &format) : _format(format)
    {
    }

    void handleRequest([[maybe_unused]] HTTPServerRequest &request,
                       [[maybe_unused]] HTTPServerResponse &rsp)
    {
        rsp.setChunkedTransferEncoding(true);
        rsp.setContentType("text/json");

        std::ostream &ostr = rsp.send();
        auto service = graphql::database::object::GetService();

        try
        {
            std::istream_iterator<char> start{request.stream() >> std::noskipws}, end{};
            std::string input{start, end};
            std::cout << input << std::endl;

            std::string queryStr;
            std::string contentType = request.getContentType();
            if (contentType.find("application/json") != std::string::npos)
            {
                Poco::JSON::Parser parser;
                Poco::Dynamic::Var result = parser.parse(input);
                Poco::JSON::Object::Ptr obj = result.extract<Poco::JSON::Object::Ptr>();
                if (obj->has("query"))
                    queryStr = obj->getValue<std::string>("query");
                else
                    throw std::runtime_error("JSON body must contain 'query' field");
            }
            else
            {
                queryStr = std::move(input);
            }

            graphql::peg::ast query = graphql::peg::parseString(std::move(queryStr));

            if (!query.root)
            {
                std::cerr << "Unknown error!" << std::endl;
                std::cerr << std::endl;
            }

            ostr << graphql::response::toJSON(service->resolve({query, ""}).get())
                 << std::endl;
        }
        catch (const std::runtime_error &ex)
        {
            std::cerr << "exception:" << ex.what() << std::endl;
        }
    }

private:
    std::string _format;
};
#endif // !WEBPAGEHANDLER_H