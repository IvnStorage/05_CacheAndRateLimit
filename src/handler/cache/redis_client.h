#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <hiredis/hiredis.h>

namespace handlers
{

    class RedisClient
    {
    public:
        static RedisClient &instance();

        void configure(const std::string &host, int port);

        bool get(const std::string &key, std::string &value);
        bool setEx(const std::string &key, const std::string &value, int ttlSeconds);
        bool del(const std::string &key);
        bool delByPrefix(const std::string &prefix);

        long long incr(const std::string &key);
        bool expire(const std::string &key, int ttlSeconds);
        int ttl(const std::string &key);

    private:
        RedisClient() = default;
        ~RedisClient();

        RedisClient(const RedisClient &) = delete;
        RedisClient &operator=(const RedisClient &) = delete;

        bool ensureConnected();

        std::mutex _mutex;
        redisContext *_context = nullptr;
        std::string _host = "redis";
        int _port = 6379;
    };

}