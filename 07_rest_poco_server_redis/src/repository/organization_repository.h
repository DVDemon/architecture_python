#pragma once

#include <Poco/Exception.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/MongoDB/Document.h>
#include <Poco/MongoDB/Element.h>
#include <Poco/MongoDB/OpMsgCursor.h>
#include <Poco/MongoDB/OpMsgMessage.h>

#include "../db/database.h"
#include "../db/mongo_helpers.h"

#include <optional>
#include <string>

namespace repository {

struct Organization {
    Poco::Int64 id{0};
    std::string name;
    std::string address;
};

namespace {

using namespace Poco::MongoDB;

inline Organization orgFromDoc(const Document::Ptr& doc) {
    Organization org;
    org.id = doc->getInteger("id");
    org.name = doc->get<std::string>("name");
    org.address = doc->get<std::string>("address");
    return org;
}

} // namespace

class OrganizationRepository {
public:
    Poco::JSON::Object::Ptr create(const Poco::JSON::Object::Ptr& payload) {
        Organization org;
        org.name = payload->getValue<std::string>("name");
        org.address = payload->getValue<std::string>("address");

        org.id = db::Database::instance().nextSequence("organizations");

        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("organizations");
        req->setCommandName(OpMsgMessage::CMD_INSERT);

        Document::Ptr doc = new Document();
        doc->add("id", org.id);
        doc->add("name", org.name);
        doc->add("address", org.address);
        req->documents().push_back(doc);

        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "organization insert");

        return toJson(org);
    }

    Poco::JSON::Array::Ptr list() {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        const std::string dbName = mdb.name();

        d.withLockedConnection([&](Poco::MongoDB::Connection& conn) {
            Poco::MongoDB::OpMsgCursor cursor(dbName, "organizations");
            cursor.setBatchSize(500);
            cursor.query().setCommandName(OpMsgMessage::CMD_FIND);
            Poco::MongoDB::Document& body = cursor.query().body();
            body.addNewDocument("sort").add("id", 1);
            body.addNewDocument("filter");

            Poco::MongoDB::OpMsgMessage& cresp = cursor.next(conn);
            while (true) {
                db::mongoEnsureOk(cresp, "organization list");
                for (const auto& doc : cresp.documents()) {
                    result->add(toJson(orgFromDoc(doc)));
                }
                if (cursor.cursorID() == 0) break;
                cresp = cursor.next(conn);
            }
        });
        return result;
    }

    std::optional<Organization> getById(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("organizations");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("id", id);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "organization getById");
        if (resp.documents().empty()) return std::nullopt;
        return orgFromDoc(resp.documents()[0]);
    }

    std::optional<Organization> update(Poco::Int64 id, const Poco::JSON::Object::Ptr& payload) {
        Organization org;
        org.name = payload->getValue<std::string>("name");
        org.address = payload->getValue<std::string>("address");

        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();

        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("organizations");
        req->setCommandName(OpMsgMessage::CMD_UPDATE);
        Document::Ptr spec = new Document();
        spec->addNewDocument("q").add("id", id);
        Document& setDoc = spec->addNewDocument("u").addNewDocument("$set");
        setDoc.add("name", org.name);
        setDoc.add("address", org.address);
        spec->add("multi", false);
        req->documents().push_back(spec);

        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "organization update");

        if (resp.body().getInteger("n") == 0) return std::nullopt;

        Organization out;
        out.id = id;
        out.name = org.name;
        out.address = org.address;
        return out;
    }

    bool remove(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();

        {
            Poco::SharedPtr<OpMsgMessage> ureq = mdb.createOpMsgMessage("users");
            ureq->setCommandName(OpMsgMessage::CMD_UPDATE);
            Document::Ptr spec = new Document();
            spec->addNewDocument("q").add("organization_id", id);
            Document& setDoc = spec->addNewDocument("u").addNewDocument("$set");
            setDoc.add("organization_id", NullValue());
            spec->add("multi", true);
            ureq->documents().push_back(spec);
            OpMsgMessage uresp;
            d.send(*ureq, uresp);
            db::mongoEnsureOk(uresp, "clear users organization_id");
        }

        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("organizations");
        req->setCommandName(OpMsgMessage::CMD_DELETE);
        Document::Ptr del = new Document();
        del->add("limit", 0).addNewDocument("q").add("id", id);
        req->documents().push_back(del);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "organization delete");
        return resp.body().getInteger("n") > 0;
    }

    static Poco::JSON::Object::Ptr toJson(const Organization& org) {
        Poco::JSON::Object::Ptr out = new Poco::JSON::Object();
        out->set("id", org.id);
        out->set("name", org.name);
        out->set("address", org.address);
        return out;
    }
};

} // namespace repository
