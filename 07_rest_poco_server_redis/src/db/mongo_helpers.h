#pragma once

#include <Poco/Exception.h>
#include <Poco/MongoDB/OpMsgMessage.h>

#include <string>

namespace db {

inline void mongoEnsureOk(Poco::MongoDB::OpMsgMessage& resp, const char* ctx) {
    if (!resp.responseOk()) {
        throw Poco::RuntimeException(std::string(ctx) + ": " + resp.body().toString());
    }
}

} // namespace db
