#pragma once

#include <Poco/Exception.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/MongoDB/Array.h>
#include <Poco/MongoDB/Document.h>
#include <Poco/MongoDB/Element.h>
#include <Poco/MongoDB/OpMsgCursor.h>
#include <Poco/MongoDB/OpMsgMessage.h>
#include <Poco/MongoDB/RegularExpression.h>
#include <Poco/Nullable.h>

#include "../cache/user_redis_cache.h"
#include "../db/database.h"
#include "../db/mongo_helpers.h"

#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace repository {

struct User {
    Poco::Int64 id{0};
    std::string firstName;
    std::string lastName;
    std::string title;
    std::string email;
    std::string login;
    std::string password;
    Poco::Nullable<Poco::Int64> organizationId;
};

namespace {

using namespace Poco::MongoDB;

inline RegularExpression::Ptr regexI(const std::string& pattern) {
    return new RegularExpression(pattern, "i");
}

inline bool organizationExists(Poco::Int64 id) {
    db::Database& d = db::Database::instance();
    Poco::MongoDB::Database& mdb = d.mongoDb();
    Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("organizations");
    req->setCommandName(OpMsgMessage::CMD_FIND);
    req->body().add("limit", 1).addNewDocument("filter").add("id", id);
    OpMsgMessage resp;
    d.send(*req, resp);
    db::mongoEnsureOk(resp, "organization lookup");
    return !resp.documents().empty();
}

inline User userFromDoc(const Document::Ptr& doc) {
    User user;
    user.id = doc->getInteger("id");
    user.firstName = doc->get<std::string>("first_name");
    user.lastName = doc->get<std::string>("last_name");
    user.title = doc->get<std::string>("title");
    user.email = doc->get<std::string>("email");
    user.login = doc->get<std::string>("login");
    user.password = doc->get<std::string>("password");
    if (doc->exists("organization_id") && !doc->isType<NullValue>("organization_id")) {
        user.organizationId = doc->getInteger("organization_id");
    }
    return user;
}

} // namespace

class UserRepository {
public:
    /// Десериализация пользователя из JSON (тот же формат, что и в API / Mongo).
    static User userFromJson(const Poco::JSON::Object::Ptr& o) {
        User user;
        user.id = o->getValue<Poco::Int64>("id");
        user.firstName = o->getValue<std::string>("first_name");
        user.lastName = o->getValue<std::string>("last_name");
        user.title = o->getValue<std::string>("title");
        user.email = o->getValue<std::string>("email");
        user.login = o->getValue<std::string>("login");
        user.password = o->getValue<std::string>("password");
        if (o->has("organization_id") && !o->isNull("organization_id")) {
            user.organizationId = o->getValue<Poco::Int64>("organization_id");
        }
        return user;
    }

    Poco::JSON::Object::Ptr create(const Poco::JSON::Object::Ptr& payload) {
        User user = fromPayload(payload);
        if (!user.organizationId.isNull() && !organizationExists(user.organizationId.value())) {
            throw Poco::InvalidArgumentException("organization not found");
        }

        const Poco::Int64 id = db::Database::instance().nextSequence("users");
        user.id = id;

        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_INSERT);

        Document::Ptr doc = new Document();
        doc->add("id", id);
        doc->add("first_name", user.firstName);
        doc->add("last_name", user.lastName);
        doc->add("title", user.title);
        doc->add("email", user.email);
        doc->add("login", user.login);
        doc->add("password", user.password);
        if (user.organizationId.isNull()) {
            doc->add("organization_id", NullValue());
        } else {
            doc->add("organization_id", user.organizationId.value());
        }
        req->documents().push_back(doc);

        OpMsgMessage resp;
        d.send(*req, resp);
        if (!resp.responseOk()) {
            Poco::Int64 code = 0;
            try {
                code = resp.body().getInteger("code");
            } catch (const Poco::Exception&) {
            }
            if (code == 11000) {
                throw Poco::InvalidArgumentException("duplicate key (email or login must be unique)");
            }
            throw Poco::RuntimeException("user insert failed: " + resp.body().toString());
        }
        // Write-through: после записи в MongoDB кладём актуальную копию в Redis (TTL в кэше).
        {
            std::ostringstream oss;
            toJson(user)->stringify(oss);
            cache::UserRedisCache::instance().putJson(user.id, oss.str());
        }
        return toJson(user);
    }

    Poco::JSON::Array::Ptr list() {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        const std::string dbName = mdb.name();

        d.withLockedConnection([&](Poco::MongoDB::Connection& conn) {
            Poco::MongoDB::OpMsgCursor cursor(dbName, "users");
            cursor.setBatchSize(1000);
            cursor.query().setCommandName(OpMsgMessage::CMD_FIND);
            Poco::MongoDB::Document& body = cursor.query().body();
            body.addNewDocument("sort").add("id", 1);
            body.addNewDocument("filter");

            Poco::MongoDB::OpMsgMessage& cresp = cursor.next(conn);
            while (true) {
                db::mongoEnsureOk(cresp, "user list");
                for (const auto& doc : cresp.documents()) {
                    result->add(toJson(userFromDoc(doc)));
                }
                if (cursor.cursorID() == 0) break;
                cresp = cursor.next(conn);
            }
        });
        return result;
    }

    Poco::JSON::Array::Ptr searchBySubstrings(std::string firstNameSubstr,
                                              std::string lastNameSubstr,
                                              std::string organizationNameSubstr) {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        const bool hasFirst = !firstNameSubstr.empty();
        const bool hasLast = !lastNameSubstr.empty();
        const bool hasOrg = !organizationNameSubstr.empty();

        std::vector<Poco::Int64> orgIds;
        if (hasOrg) {
            db::Database& d = db::Database::instance();
            Poco::MongoDB::Database& mdb = d.mongoDb();
            Poco::SharedPtr<OpMsgMessage> oreq = mdb.createOpMsgMessage("organizations");
            oreq->setCommandName(OpMsgMessage::CMD_FIND);
            oreq->body()
                .add("limit", 100000)
                .addNewDocument("filter")
                .add("name", regexI(db::escapeRegex(organizationNameSubstr)));
            OpMsgMessage oresp;
            d.send(*oreq, oresp);
            db::mongoEnsureOk(oresp, "organization search");
            for (const auto& doc : oresp.documents()) {
                orgIds.push_back(doc->getInteger("id"));
            }
            if (orgIds.empty()) {
                return result;
            }
        }

        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        const std::string dbName = mdb.name();

        d.withLockedConnection([&](Poco::MongoDB::Connection& conn) {
            Poco::MongoDB::OpMsgCursor cursor(dbName, "users");
            cursor.setBatchSize(1000);
            cursor.query().setCommandName(OpMsgMessage::CMD_FIND);
            Poco::MongoDB::Document& body = cursor.query().body();
            body.addNewDocument("sort").add("id", 1);
            Poco::MongoDB::Document& filter = body.addNewDocument("filter");

        if (hasFirst && !hasLast && !hasOrg) {
            filter.add("first_name", regexI(db::escapeRegex(firstNameSubstr)));
        } else if (!hasFirst && hasLast && !hasOrg) {
            filter.add("last_name", regexI(db::escapeRegex(lastNameSubstr)));
        } else if (!hasFirst && !hasLast && hasOrg) {
            Poco::MongoDB::Document& inDoc = filter.addNewDocument("organization_id");
            Poco::MongoDB::Array& arr = inDoc.addNewArray("$in");
            for (Poco::Int64 oid : orgIds) {
                arr.add(oid);
            }
        } else if (hasFirst && hasLast && !hasOrg) {
            Poco::MongoDB::Array& andArr = filter.addNewArray("$and");
            Document::Ptr c1 = new Document();
            c1->add("first_name", regexI(db::escapeRegex(firstNameSubstr)));
            andArr.add(c1);
            Document::Ptr c2 = new Document();
            c2->add("last_name", regexI(db::escapeRegex(lastNameSubstr)));
            andArr.add(c2);
        } else if (hasFirst && !hasLast && hasOrg) {
            Poco::MongoDB::Array& andArr = filter.addNewArray("$and");
            Document::Ptr c1 = new Document();
            c1->add("first_name", regexI(db::escapeRegex(firstNameSubstr)));
            andArr.add(c1);
            Document::Ptr c2 = new Document();
            Poco::MongoDB::Document& inDoc = c2->addNewDocument("organization_id");
            Poco::MongoDB::Array& arr = inDoc.addNewArray("$in");
            for (Poco::Int64 oid : orgIds) {
                arr.add(oid);
            }
            andArr.add(c2);
        } else if (!hasFirst && hasLast && hasOrg) {
            Poco::MongoDB::Array& andArr = filter.addNewArray("$and");
            Document::Ptr c1 = new Document();
            c1->add("last_name", regexI(db::escapeRegex(lastNameSubstr)));
            andArr.add(c1);
            Document::Ptr c2 = new Document();
            Poco::MongoDB::Document& inDoc = c2->addNewDocument("organization_id");
            Poco::MongoDB::Array& arr = inDoc.addNewArray("$in");
            for (Poco::Int64 oid : orgIds) {
                arr.add(oid);
            }
            andArr.add(c2);
        } else { // hasFirst && hasLast && hasOrg
            Poco::MongoDB::Array& andArr = filter.addNewArray("$and");
            Document::Ptr c1 = new Document();
            c1->add("first_name", regexI(db::escapeRegex(firstNameSubstr)));
            andArr.add(c1);
            Document::Ptr c2 = new Document();
            c2->add("last_name", regexI(db::escapeRegex(lastNameSubstr)));
            andArr.add(c2);
            Document::Ptr c3 = new Document();
            Poco::MongoDB::Document& inDoc = c3->addNewDocument("organization_id");
            Poco::MongoDB::Array& arr = inDoc.addNewArray("$in");
            for (Poco::Int64 oid : orgIds) {
                arr.add(oid);
            }
            andArr.add(c3);
        }

            Poco::MongoDB::OpMsgMessage& cresp = cursor.next(conn);
            while (true) {
                db::mongoEnsureOk(cresp, "user search");
                for (const auto& doc : cresp.documents()) {
                    result->add(toJson(userFromDoc(doc)));
                }
                if (cursor.cursorID() == 0) break;
                cresp = cursor.next(conn);
            }
        });
        return result;
    }

    /// Read-through: при попадании в Redis данные не читаются из MongoDB.
    std::optional<User> getById(Poco::Int64 id) {
        if (auto cached = cache::UserRedisCache::instance().tryGetJson(id)) {
            try {
                Poco::JSON::Parser parser;
                Poco::Dynamic::Var parsed = parser.parse(*cached);
                auto obj = parsed.extract<Poco::JSON::Object::Ptr>();
                return userFromJson(obj);
            } catch (const Poco::Exception&) {
                // повреждённый кэш — читаем из MongoDB
            }
        }

        std::optional<User> fromMongo = loadUserFromMongo(id);
        if (fromMongo) {
            std::ostringstream oss;
            toJson(*fromMongo)->stringify(oss);
            cache::UserRedisCache::instance().putJson(id, oss.str());
        }
        return fromMongo;
    }

    std::optional<User> update(Poco::Int64 id, const Poco::JSON::Object::Ptr& payload) {
        User user = fromPayload(payload);
        if (!user.organizationId.isNull() && !organizationExists(user.organizationId.value())) {
            throw Poco::InvalidArgumentException("organization not found");
        }

        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();

        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_UPDATE);
        Document::Ptr spec = new Document();
        spec->addNewDocument("q").add("id", id);
        Document& setDoc = spec->addNewDocument("u").addNewDocument("$set");
        setDoc.add("first_name", user.firstName);
        setDoc.add("last_name", user.lastName);
        setDoc.add("title", user.title);
        setDoc.add("email", user.email);
        setDoc.add("login", user.login);
        setDoc.add("password", user.password);
        if (user.organizationId.isNull()) {
            setDoc.add("organization_id", NullValue());
        } else {
            setDoc.add("organization_id", user.organizationId.value());
        }
        spec->add("multi", false);
        req->documents().push_back(spec);

        OpMsgMessage resp;
        d.send(*req, resp);
        if (!resp.responseOk()) {
            Poco::Int64 code = 0;
            try {
                code = resp.body().getInteger("code");
            } catch (const Poco::Exception&) {
            }
            if (code == 11000) {
                throw Poco::InvalidArgumentException("duplicate key (email or login must be unique)");
            }
            throw Poco::RuntimeException("user update failed: " + resp.body().toString());
        }

        const Poco::Int64 matched = resp.body().getInteger("n");
        if (matched == 0) return std::nullopt;
        std::optional<User> fresh = loadUserFromMongo(id);
        if (fresh) {
            std::ostringstream oss;
            toJson(*fresh)->stringify(oss);
            cache::UserRedisCache::instance().putJson(id, oss.str());
        }
        return fresh;
    }

    bool remove(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_DELETE);
        Document::Ptr del = new Document();
        del->add("limit", 0).addNewDocument("q").add("id", id);
        req->documents().push_back(del);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "user delete");
        const bool removed = resp.body().getInteger("n") > 0;
        if (removed) cache::UserRedisCache::instance().remove(id);
        return removed;
    }

    static Poco::JSON::Object::Ptr toJson(const User& user) {
        Poco::JSON::Object::Ptr out = new Poco::JSON::Object();
        out->set("id", user.id);
        out->set("first_name", user.firstName);
        out->set("last_name", user.lastName);
        out->set("title", user.title);
        out->set("email", user.email);
        out->set("login", user.login);
        out->set("password", user.password);
        if (user.organizationId.isNull()) out->set("organization_id", Poco::Dynamic::Var());
        else out->set("organization_id", user.organizationId.value());
        return out;
    }

private:
    std::optional<User> loadUserFromMongo(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("id", id);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "user getById");
        if (resp.documents().empty()) return std::nullopt;
        return userFromDoc(resp.documents()[0]);
    }

    User fromPayload(const Poco::JSON::Object::Ptr& payload) {
        User user;
        user.firstName = payload->getValue<std::string>("first_name");
        user.lastName = payload->getValue<std::string>("last_name");
        user.title = payload->getValue<std::string>("title");
        user.email = payload->getValue<std::string>("email");
        user.login = payload->getValue<std::string>("login");
        user.password = payload->getValue<std::string>("password");
        if (payload->has("organization_id") && !payload->isNull("organization_id")) {
            user.organizationId = payload->getValue<Poco::Int64>("organization_id");
        }
        return user;
    }
};

} // namespace repository
