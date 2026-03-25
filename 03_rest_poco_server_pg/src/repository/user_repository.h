#pragma once

#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Nullable.h>

#include "../db/database.h"

#include <optional>
#include <string>

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

class UserRepository {
public:
    Poco::JSON::Object::Ptr create(const Poco::JSON::Object::Ptr& payload) {
        User user = fromPayload(payload);
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement insert(session);
        insert << "INSERT INTO users(first_name,last_name,title,email,login,password,organization_id) "
                  "VALUES($1,$2,$3,$4,$5,$6,$7) RETURNING id",
            Poco::Data::Keywords::use(user.firstName),
            Poco::Data::Keywords::use(user.lastName),
            Poco::Data::Keywords::use(user.title),
            Poco::Data::Keywords::use(user.email),
            Poco::Data::Keywords::use(user.login),
            Poco::Data::Keywords::use(user.password),
            Poco::Data::Keywords::use(user.organizationId),
            Poco::Data::Keywords::into(user.id),
            Poco::Data::Keywords::now;
        return toJson(user);
    }

    Poco::JSON::Array::Ptr list() {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement select(session);
        select << "SELECT id, first_name, last_name, title, email, login, password, organization_id FROM users ORDER BY id";
        select.execute();
        Poco::Data::RecordSet rs(select);
        if (!rs.moveFirst()) {
            return result; // empty result set
        }
        do {
            Poco::JSON::Object::Ptr item = new Poco::JSON::Object();
            item->set("id", rs[0].convert<Poco::Int64>());
            item->set("first_name", rs[1].convert<std::string>());
            item->set("last_name", rs[2].convert<std::string>());
            item->set("title", rs[3].convert<std::string>());
            item->set("email", rs[4].convert<std::string>());
            item->set("login", rs[5].convert<std::string>());
            item->set("password", rs[6].convert<std::string>());
            if (rs.isNull("organization_id")) item->set("organization_id", Poco::Dynamic::Var());
            else item->set("organization_id", rs["organization_id"].convert<Poco::Int64>());
            result->add(item);
        } while (rs.moveNext());
        return result;
    }

    Poco::JSON::Array::Ptr searchBySubstrings(std::string firstNameSubstr,
                                                std::string lastNameSubstr,
                                                std::string organizationNameSubstr) {
        // "Подстрока" реализована через PostgreSQL full-text search
        // (используем те самые текстовые индексы на to_tsvector()).
        //
        // Генерируем SQL под комбинацию непустых параметров (а не CASE WHEN),
        // т.к. POCO::Data::RecordSet в некоторых случаях ломается на выражениях
        // с несколькими CASE WHEN для разных плейсхолдеров.
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement select(session);

        const bool hasFirst = !firstNameSubstr.empty();
        const bool hasLast = !lastNameSubstr.empty();
        const bool hasOrg = !organizationNameSubstr.empty();

        const std::string base =
            "SELECT u.id, u.first_name, u.last_name, u.title, u.email, u.login, u.password, u.organization_id "
            "FROM users u "
            "LEFT JOIN organizations o ON u.organization_id = o.id ";

        if (hasFirst && !hasLast && !hasOrg) {
            select << (base +
                        "WHERE to_tsvector('simple', u.first_name) @@ plainto_tsquery('simple', $1) "
                        "ORDER BY u.id"),
                Poco::Data::Keywords::use(firstNameSubstr);
        } else if (!hasFirst && hasLast && !hasOrg) {
            select << (base +
                        "WHERE to_tsvector('simple', u.last_name) @@ plainto_tsquery('simple', $1) "
                        "ORDER BY u.id"),
                Poco::Data::Keywords::use(lastNameSubstr);
        } else if (!hasFirst && !hasLast && hasOrg) {
            select << (base +
                        "WHERE to_tsvector('simple', o.name) @@ plainto_tsquery('simple', $1) "
                        "ORDER BY u.id"),
                Poco::Data::Keywords::use(organizationNameSubstr);
        } else if (hasFirst && hasLast && !hasOrg) {
            select << (base +
                        "WHERE to_tsvector('simple', u.first_name) @@ plainto_tsquery('simple', $1) "
                        "AND to_tsvector('simple', u.last_name) @@ plainto_tsquery('simple', $2) "
                        "ORDER BY u.id"),
                Poco::Data::Keywords::use(firstNameSubstr),
                Poco::Data::Keywords::use(lastNameSubstr);
        } else if (hasFirst && !hasLast && hasOrg) {
            select << (base +
                        "WHERE to_tsvector('simple', u.first_name) @@ plainto_tsquery('simple', $1) "
                        "AND to_tsvector('simple', o.name) @@ plainto_tsquery('simple', $2) "
                        "ORDER BY u.id"),
                Poco::Data::Keywords::use(firstNameSubstr),
                Poco::Data::Keywords::use(organizationNameSubstr);
        } else if (!hasFirst && hasLast && hasOrg) {
            select << (base +
                        "WHERE to_tsvector('simple', u.last_name) @@ plainto_tsquery('simple', $1) "
                        "AND to_tsvector('simple', o.name) @@ plainto_tsquery('simple', $2) "
                        "ORDER BY u.id"),
                Poco::Data::Keywords::use(lastNameSubstr),
                Poco::Data::Keywords::use(organizationNameSubstr);
        } else { // hasFirst && hasLast && hasOrg
            select << (base +
                        "WHERE to_tsvector('simple', u.first_name) @@ plainto_tsquery('simple', $1) "
                        "AND to_tsvector('simple', u.last_name) @@ plainto_tsquery('simple', $2) "
                        "AND to_tsvector('simple', o.name) @@ plainto_tsquery('simple', $3) "
                        "ORDER BY u.id"),
                Poco::Data::Keywords::use(firstNameSubstr),
                Poco::Data::Keywords::use(lastNameSubstr),
                Poco::Data::Keywords::use(organizationNameSubstr);
        }

        select.execute();
        Poco::Data::RecordSet rs(select);
        if (!rs.moveFirst()) return result;

        do {
            Poco::JSON::Object::Ptr item = new Poco::JSON::Object();
            item->set("id", rs[0].convert<Poco::Int64>());
            item->set("first_name", rs[1].convert<std::string>());
            item->set("last_name", rs[2].convert<std::string>());
            item->set("title", rs[3].convert<std::string>());
            item->set("email", rs[4].convert<std::string>());
            item->set("login", rs[5].convert<std::string>());
            item->set("password", rs[6].convert<std::string>());
            if (rs.isNull("organization_id")) item->set("organization_id", Poco::Dynamic::Var());
            else item->set("organization_id", rs["organization_id"].convert<Poco::Int64>());
            result->add(item);
        } while (rs.moveNext());

        return result;
    }

    std::optional<User> getById(Poco::Int64 id) {
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement select(session);
        select << "SELECT id, first_name, last_name, title, email, login, password, organization_id "
                  "FROM users WHERE id = $1",
            Poco::Data::Keywords::use(id);

        select.execute();
        Poco::Data::RecordSet rs(select);
        if (!rs.moveFirst()) return std::nullopt;

        User user;
        user.id = rs["id"].convert<Poco::Int64>();
        user.firstName = rs["first_name"].convert<std::string>();
        user.lastName = rs["last_name"].convert<std::string>();
        user.title = rs["title"].convert<std::string>();
        user.email = rs["email"].convert<std::string>();
        user.login = rs["login"].convert<std::string>();
        user.password = rs["password"].convert<std::string>();
        if (!rs.isNull("organization_id")) user.organizationId = rs["organization_id"].convert<Poco::Int64>();
        return user;
    }

    std::optional<User> update(Poco::Int64 id, const Poco::JSON::Object::Ptr& payload) {
        User user = fromPayload(payload);
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement upd(session);
        upd << "UPDATE users SET "
               "first_name=$1,last_name=$2,title=$3,email=$4,login=$5,password=$6,organization_id=$7 "
               "WHERE id=$8 "
               "RETURNING id, first_name, last_name, title, email, login, password, organization_id",
            Poco::Data::Keywords::use(user.firstName),
            Poco::Data::Keywords::use(user.lastName),
            Poco::Data::Keywords::use(user.title),
            Poco::Data::Keywords::use(user.email),
            Poco::Data::Keywords::use(user.login),
            Poco::Data::Keywords::use(user.password),
            Poco::Data::Keywords::use(user.organizationId),
            Poco::Data::Keywords::use(id);

        upd.execute();
        Poco::Data::RecordSet rs(upd);
        if (!rs.moveFirst()) return std::nullopt;

        User out;
        out.id = rs["id"].convert<Poco::Int64>();
        out.firstName = rs["first_name"].convert<std::string>();
        out.lastName = rs["last_name"].convert<std::string>();
        out.title = rs["title"].convert<std::string>();
        out.email = rs["email"].convert<std::string>();
        out.login = rs["login"].convert<std::string>();
        out.password = rs["password"].convert<std::string>();
        if (!rs.isNull("organization_id")) out.organizationId = rs["organization_id"].convert<Poco::Int64>();
        return out;
    }

    bool remove(Poco::Int64 id) {
        Poco::Data::Session session = db::Database::instance().createSession();
        Poco::Data::Statement del(session);
        del << "DELETE FROM users WHERE id = $1 RETURNING id",
            Poco::Data::Keywords::use(id);

        del.execute();
        Poco::Data::RecordSet rs(del);
        return rs.moveFirst();
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
