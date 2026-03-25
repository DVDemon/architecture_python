#pragma once

#include <Poco/Data/PostgreSQL/Connector.h>
#include <Poco/Data/Session.h>
#include <Poco/Environment.h>

#include <mutex>
#include <string>

namespace db {

class Database {
public:
    static Database& instance() {
        static Database instance;
        return instance;
    }

    void initialize() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_initialized) return;
        Poco::Data::PostgreSQL::Connector::registerConnector();
        _connectionString = buildConnectionString();
        _initialized = true;
    }

    Poco::Data::Session createSession() {
        initialize();
        return Poco::Data::Session(Poco::Data::PostgreSQL::Connector::KEY, _connectionString);
    }

    void initializeSchema() {
        Poco::Data::Session session = createSession();
        session << "CREATE TABLE IF NOT EXISTS organizations (id BIGSERIAL PRIMARY KEY,name TEXT NOT NULL,address TEXT NOT NULL)", Poco::Data::Keywords::now;
        session << "CREATE TABLE IF NOT EXISTS users (id BIGSERIAL PRIMARY KEY,first_name TEXT NOT NULL,last_name TEXT NOT NULL,title TEXT NOT NULL,email TEXT NOT NULL UNIQUE,login TEXT NOT NULL UNIQUE,password TEXT NOT NULL,organization_id BIGINT NULL REFERENCES organizations(id) ON DELETE SET NULL)", Poco::Data::Keywords::now;
        session << "CREATE INDEX IF NOT EXISTS idx_users_id_btree ON users USING btree (id)", Poco::Data::Keywords::now;
        session << "CREATE INDEX IF NOT EXISTS idx_organizations_id_btree ON organizations USING btree (id)", Poco::Data::Keywords::now;
        session << "CREATE INDEX IF NOT EXISTS idx_users_first_name_fts ON users USING gin (to_tsvector('simple', first_name))", Poco::Data::Keywords::now;
        session << "CREATE INDEX IF NOT EXISTS idx_users_last_name_fts ON users USING gin (to_tsvector('simple', last_name))", Poco::Data::Keywords::now;
        session << "CREATE INDEX IF NOT EXISTS idx_organizations_name_fts ON organizations USING gin (to_tsvector('simple', name))", Poco::Data::Keywords::now;
    }

private:
    Database() = default;

    std::string buildConnectionString() const {
        const std::string host = Poco::Environment::get("PGHOST", "localhost");
        const std::string port = Poco::Environment::get("PGPORT", "5432");
        const std::string dbname = Poco::Environment::get("PGDATABASE", "poco_template");
        const std::string user = Poco::Environment::get("PGUSER", "postgres");
        const std::string password = Poco::Environment::get("PGPASSWORD", "postgres");
        return "host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password;
    }

    std::mutex _mutex;
    bool _initialized{false};
    std::string _connectionString;
};

} // namespace db
