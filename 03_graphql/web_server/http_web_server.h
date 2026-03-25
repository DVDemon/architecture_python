#ifndef HTTPWEBSERVER_H
#define HTTPWEBSERVER_H

#include "Poco/DateTimeFormat.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Util/ServerApplication.h"

#include "http_request_factory.h"

using Poco::DateTimeFormat;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::ServerSocket;
using Poco::Util::Application;
using Poco::Util::ServerApplication;

class HTTPWebServer : public Poco::Util::ServerApplication
{
protected:
    int main([[maybe_unused]] const std::vector<std::string> &args)
    {
        std::string format(
            config().getString("HTTPWebServer.format",
                               DateTimeFormat::SORTABLE_FORMAT));

        ServerSocket svs(Poco::Net::SocketAddress("0.0.0.0", 8080));
        HTTPServer srv(new HTTPRequestFactory(format),
                       svs, new HTTPServerParams);
        srv.start();
        waitForTerminationRequest();
        srv.stop();

        return Application::EXIT_OK;
    }
};
#endif // !HTTPWEBSERVER