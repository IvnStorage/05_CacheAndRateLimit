#include "redis_client.h"

#include <Poco/Logger.h>

namespace handlers
{

    RedisClient &RedisClient::instance()
    {
        static RedisClient client;
        return client;
    }

    RedisClient::~RedisClient()
    {
        if (_context)
        {
            redisFree(_context);
            _context = nullptr;
        }
    }

    void RedisClient::configure(const std::string &host, int port)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _host = host;
        _port = port;

        if (_context)
        {
            redisFree(_context);
            _context = nullptr;
        }
    }

    bool RedisClient::ensureConnected()
    {
        if (_context && !_context->err)
        {
            return true;
        }

        if (_context)
        {
            redisFree(_context);
            _context = nullptr;
        }

        _context = redisConnect(_host.c_str(), _port);

        if (!_context || _context->err)
        {
            Poco::Logger::get("Server").warning("Redis connection failed");
            return false;
        }

        return true;
    }

    bool RedisClient::get(const std::string &key, std::string &value)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!ensureConnected())
        {
            return false;
        }

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(_context, "GET %s", key.c_str()));

        if (!reply)
        {
            return false;
        }

        bool found = false;

        if (reply->type == REDIS_REPLY_STRING)
        {
            value.assign(reply->str, reply->len);
            found = true;
        }

        freeReplyObject(reply);
        return found;
    }

    bool RedisClient::setEx(const std::string &key, const std::string &value, int ttlSeconds)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!ensureConnected())
        {
            return false;
        }

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(_context, "SETEX %s %d %b",
                         key.c_str(),
                         ttlSeconds,
                         value.data(),
                         value.size()));

        if (!reply)
        {
            return false;
        }

        const bool ok = reply->type == REDIS_REPLY_STATUS;
        freeReplyObject(reply);
        return ok;
    }

    bool RedisClient::del(const std::string &key)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!ensureConnected())
        {
            return false;
        }

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(_context, "DEL %s", key.c_str()));

        if (!reply)
        {
            return false;
        }

        freeReplyObject(reply);
        return true;
    }

    bool RedisClient::delByPrefix(const std::string &prefix)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!ensureConnected())
        {
            return false;
        }

        const std::string pattern = prefix + "*";

        redisReply *keysReply = static_cast<redisReply *>(
            redisCommand(_context, "KEYS %s", pattern.c_str()));

        if (!keysReply)
        {
            return false;
        }

        if (keysReply->type == REDIS_REPLY_ARRAY)
        {
            for (size_t i = 0; i < keysReply->elements; ++i)
            {
                redisReply *keyReply = keysReply->element[i];

                if (keyReply && keyReply->type == REDIS_REPLY_STRING)
                {
                    redisReply *delReply = static_cast<redisReply *>(
                        redisCommand(_context, "DEL %s", keyReply->str));

                    if (delReply)
                    {
                        freeReplyObject(delReply);
                    }
                }
            }
        }

        freeReplyObject(keysReply);
        return true;
    }

    long long RedisClient::incr(const std::string &key)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!ensureConnected())
        {
            return 0;
        }

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(_context, "INCR %s", key.c_str()));

        if (!reply)
        {
            return 0;
        }

        long long value = 0;

        if (reply->type == REDIS_REPLY_INTEGER)
        {
            value = reply->integer;
        }

        freeReplyObject(reply);
        return value;
    }

    bool RedisClient::expire(const std::string &key, int ttlSeconds)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!ensureConnected())
        {
            return false;
        }

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(_context, "EXPIRE %s %d", key.c_str(), ttlSeconds));

        if (!reply)
        {
            return false;
        }

        freeReplyObject(reply);
        return true;
    }

    int RedisClient::ttl(const std::string &key)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!ensureConnected())
        {
            return -1;
        }

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(_context, "TTL %s", key.c_str()));

        if (!reply)
        {
            return -1;
        }

        int value = -1;

        if (reply->type == REDIS_REPLY_INTEGER)
        {
            value = static_cast<int>(reply->integer);
        }

        freeReplyObject(reply);
        return value;
    }

}