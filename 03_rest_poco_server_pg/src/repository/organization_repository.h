#pragma once

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Nullable.h>

#include "../db/database.h"

#include <optional>
#include <stdexcept>
#include <string>

namespace repository {

struct Organization {
    Poco::Int64 id{0};
    std::string name;
    std::string address;
};

class OrganizationRepository {
public:
    Poco::JSON::Object::Ptr create(const Poco::JSON::Object::Ptr& payload) {
        Organization org;
        org.name = payload->getValue<std::string>("name");
        org.address = payload->getValue<std::string>("address");

        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement insert(session);
        insert << "INSERT INTO organizations(name, address) VALUES($1, $2) RETURNING id",
            Poco::Data::Keywords::use(org.name),
            Poco::Data::Keywords::use(org.address),
            Poco::Data::Keywords::into(org.id),
            Poco::Data::Keywords::now;

        return toJson(org);
    }

    Poco::JSON::Array::Ptr list() {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement select(session);
        select << "SELECT id, name, address FROM organizations ORDER BY id";
        select.execute();
        Poco::Data::RecordSet rs(select);
        while (rs.moveNext()) {
            Poco::JSON::Object::Ptr item = new Poco::JSON::Object();
            item->set("id", rs[0].convert<Poco::Int64>());
            item->set("name", rs[1].convert<std::string>());
            item->set("address", rs[2].convert<std::string>());
            result->add(item);
        }
        return result;
    }

    std::optional<Organization> getById(Poco::Int64 id) {
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement select(session);
        select << "SELECT id, name, address FROM organizations WHERE id = $1",
            Poco::Data::Keywords::use(id),
            Poco::Data::Keywords::now;

        select.execute();
        Poco::Data::RecordSet rs(select);
        if (!rs.moveFirst()) return std::nullopt;

        Organization org;
        org.id = rs["id"].convert<Poco::Int64>();
        org.name = rs["name"].convert<std::string>();
        org.address = rs["address"].convert<std::string>();
        return org;
    }

    std::optional<Organization> update(Poco::Int64 id, const Poco::JSON::Object::Ptr& payload) {
        Organization org;
        org.name = payload->getValue<std::string>("name");
        org.address = payload->getValue<std::string>("address");

        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement upd(session);
        upd << "UPDATE organizations SET name=$1, address=$2 WHERE id=$3 "
               "RETURNING id, name, address",
            Poco::Data::Keywords::use(org.name),
            Poco::Data::Keywords::use(org.address),
            Poco::Data::Keywords::use(id),
            Poco::Data::Keywords::now;

        upd.execute();
        Poco::Data::RecordSet rs(upd);
        if (!rs.moveFirst()) return std::nullopt;

        Organization out;
        out.id = rs["id"].convert<Poco::Int64>();
        out.name = rs["name"].convert<std::string>();
        out.address = rs["address"].convert<std::string>();
        return out;
    }

    bool remove(Poco::Int64 id) {
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement del(session);
        del << "DELETE FROM organizations WHERE id = $1 RETURNING id",
            Poco::Data::Keywords::use(id),
            Poco::Data::Keywords::now;

        del.execute();
        Poco::Data::RecordSet rs(del);
        return rs.moveFirst();
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
