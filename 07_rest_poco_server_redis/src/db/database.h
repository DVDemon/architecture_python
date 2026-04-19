#pragma once

#include <Poco/Environment.h>
#include <Poco/Exception.h>
#include <Poco/MongoDB/Connection.h>
#include <Poco/MongoDB/Database.h>
#include <Poco/MongoDB/Document.h>
#include <Poco/MongoDB/Element.h>
#include <Poco/MongoDB/OpMsgMessage.h>
#include <Poco/NumberParser.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace db {

inline std::string escapeRegex(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
        case '\\':
        case '^':
        case '$':
        case '.':
        case '|':
        case '?':
        case '*':
        case '+':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
            out.push_back('\\');
            break;
        default:
            break;
        }
        out.push_back(static_cast<char>(c));
    }
    return out;
}

class Database {
public:
    static Database& instance() {
        static Database inst;
        return inst;
    }

    void initialize() {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_connected) return;

        const std::string host = Poco::Environment::get("MONGO_HOST", "localhost");
        const std::string portStr = Poco::Environment::get("MONGO_PORT", "27017");
        int port = 27017;
        try {
            port = Poco::NumberParser::parse(portStr);
        } catch (const Poco::Exception&) {
            port = 27017;
        }

        _dbName = Poco::Environment::get("MONGO_DATABASE", "poco_template");
        _mongoDb = std::make_unique<Poco::MongoDB::Database>(_dbName);
        _connection.connect(host, port);
        _connected = true;
    }

    Poco::MongoDB::Database& mongoDb() {
        initialize();
        return *_mongoDb;
    }

    void send(Poco::MongoDB::OpMsgMessage& req, Poco::MongoDB::OpMsgMessage& resp) {
        initialize();
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _connection.sendRequest(req, resp);
    }

    /// Выполняет fn с удержанием mutex на одном Connection (нужно для OpMsgCursor / getMore).
    template <typename F>
    void withLockedConnection(F&& fn) {
        initialize();
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        std::forward<F>(fn)(_connection);
    }

    Poco::Int64 nextSequence(const std::string& counterKey) {
        Poco::MongoDB::Database& db = mongoDb();
        Poco::SharedPtr<Poco::MongoDB::OpMsgMessage> req = db.createOpMsgMessage();
        req->setCommandName(Poco::MongoDB::OpMsgMessage::CMD_FIND_AND_MODIFY);
        req->body().add("findAndModify", std::string("counters"));
        req->body().add("upsert", true);
        req->body().add("new", true);
        Poco::MongoDB::Document& q = req->body().addNewDocument("query");
        q.add("_id", counterKey);
        Poco::MongoDB::Document& upd = req->body().addNewDocument("update");
        Poco::MongoDB::Document& inc = upd.addNewDocument("$inc");
        inc.add("seq", Poco::Int64(1));

        Poco::MongoDB::OpMsgMessage resp;
        send(*req, resp);
        if (!resp.responseOk()) {
            throw Poco::RuntimeException("MongoDB findAndModify failed: " + resp.body().toString());
        }
        Poco::MongoDB::Document::Ptr val = resp.body().get<Poco::MongoDB::Document::Ptr>("value");
        return val->getInteger("seq");
    }

    void initializeSchema() {
        initialize();
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        if (_schemaInitialized) return;

        Poco::MongoDB::Database::IndexedFields orgId{{"id", true}};
        _mongoDb->createIndex(_connection, "organizations", orgId, "organizations_id_unique",
                              Poco::MongoDB::Database::INDEX_UNIQUE);

        Poco::MongoDB::Database::IndexedFields userId{{"id", true}};
        _mongoDb->createIndex(_connection, "users", userId, "users_id_unique",
                              Poco::MongoDB::Database::INDEX_UNIQUE);

        Poco::MongoDB::Database::IndexedFields userEmail{{"email", true}};
        _mongoDb->createIndex(_connection, "users", userEmail, "users_email_unique",
                              Poco::MongoDB::Database::INDEX_UNIQUE);

        Poco::MongoDB::Database::IndexedFields userLogin{{"login", true}};
        _mongoDb->createIndex(_connection, "users", userLogin, "users_login_unique",
                              Poco::MongoDB::Database::INDEX_UNIQUE);

        _schemaInitialized = true;
    }

private:
    Database() = default;

    std::recursive_mutex _mutex;
    Poco::MongoDB::Connection _connection;
    std::unique_ptr<Poco::MongoDB::Database> _mongoDb;
    std::string _dbName;
    bool _connected{false};
    bool _schemaInitialized{false};
};

} // namespace db
