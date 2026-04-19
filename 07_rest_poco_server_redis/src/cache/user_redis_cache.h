#pragma once

#include <Poco/Environment.h>
#include <Poco/Types.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/Redis/Client.h>
#include <Poco/Redis/Command.h>
#include <Poco/Redis/Exception.h>
#include <Poco/Redis/Type.h>
#include <Poco/Timespan.h>

#include <atomic>
#include <mutex>
#include <optional>
#include <string>

namespace cache {

/// Кэш JSON-представления пользователя по `user_id` в Redis (`Poco::Redis`).
/// Один `Client` + `std::mutex` — корректно при параллельных HTTP-воркерах POCO.
class UserRedisCache {
public:
    static constexpr int kTtlSeconds = 60;

    static UserRedisCache& instance() {
        static UserRedisCache inst;
        return inst;
    }

    /// Включён ли кэш (по умолчанию да; можно выключить через `--user-cache=off` до старта сервера).
    static bool userCacheEnabled() {
        return _userCacheEnabled.load(std::memory_order_relaxed);
    }

    static void setUserCacheEnabled(bool enabled) {
        _userCacheEnabled.store(enabled, std::memory_order_relaxed);
    }

    /// Read-through: значение из Redis или nullopt при промахе / ошибке Redis.
    std::optional<std::string> tryGetJson(Poco::Int64 userId) {
        if (!userCacheEnabled()) return std::nullopt;
        if (!ensureConnected()) return std::nullopt;

        std::lock_guard<std::mutex> lock(_mutex);
        try {
            Poco::Redis::BulkString raw =
                _client.execute<Poco::Redis::BulkString>(Poco::Redis::Command::get(cacheKey(userId)));
            if (raw.isNull()) return std::nullopt;
            return raw.value();
        } catch (const Poco::Redis::RedisException& e) {
            logRedisWarning("tryGetJson", e);
            return std::nullopt;
        }
    }

    /// Write-through: сохранить актуальный JSON в Redis с TTL.
    void putJson(Poco::Int64 userId, const std::string& json) {
        if (!userCacheEnabled()) return;
        if (!ensureConnected()) return;

        const Poco::Timespan ttl(kTtlSeconds, 0);

        std::lock_guard<std::mutex> lock(_mutex);
        try {
            _client.execute<std::string>(
                Poco::Redis::Command::set(cacheKey(userId), json, true, ttl, true));
        } catch (const Poco::Redis::RedisException& e) {
            logRedisWarning("putJson", e);
        }
    }

    void remove(Poco::Int64 userId) {
        if (!userCacheEnabled()) return;
        if (!ensureConnected()) return;

        std::lock_guard<std::mutex> lock(_mutex);
        try {
            _client.execute<Poco::Int64>(Poco::Redis::Command::del(cacheKey(userId)));
        } catch (const Poco::Redis::RedisException& e) {
            logRedisWarning("remove", e);
        }
    }

private:
    UserRedisCache() = default;

    static std::string cacheKey(Poco::Int64 userId) {
        return std::string("user:") + Poco::NumberFormatter::format(userId);
    }

    bool ensureConnected() {
        if (_disabled) return false;
        if (_connected && _client.isConnected()) return true;

        std::lock_guard<std::mutex> lock(_mutex);
        if (_disabled) return false;
        if (_connected && _client.isConnected()) return true;

        const std::string host = Poco::Environment::get("REDIS_HOST", "localhost");
        const std::string portStr = Poco::Environment::get("REDIS_PORT", "6379");
        int port = 6379;
        try {
            port = Poco::NumberParser::parse(portStr);
        } catch (const Poco::Exception&) {
            port = 6379;
        }

        try {
            if (_client.isConnected()) _client.disconnect();
            _client.connect(host, port);
            _client.execute<std::string>(Poco::Redis::Command::ping());
            _connected = true;
            _disabled = false;
            return true;
        } catch (const Poco::Exception& e) {
            auto& logger = Poco::Logger::get("Server");
            logger.warning("Redis unavailable at %s:%d (%s). User cache disabled for this process.",
                           host,
                           port,
                           e.displayText());
            _connected = false;
            _disabled = true;
            return false;
        }
    }

    static void logRedisWarning(const char* op, const Poco::Exception& e) {
        Poco::Logger::get("Server").warning("Redis %s: %s", std::string(op), e.displayText());
    }

    std::mutex _mutex;
    Poco::Redis::Client _client;
    bool _connected{false};
    bool _disabled{false};

    inline static std::atomic<bool> _userCacheEnabled{true};
};

} // namespace cache
