#pragma once

#include <algorithm>
#include <string>

#include "redis_client.h"

namespace handlers
{

    struct RateLimitResult
    {
        bool allowed = true;
        int limit = 0;
        int remaining = 0;
        int resetSeconds = 0;
    };

    class RateLimiter
    {
    public:
        static RateLimitResult fixedWindow(
            const std::string &key,
            int limit,
            int windowSeconds)
        {
            RateLimitResult result;
            result.limit = limit;

            const long long current = RedisClient::instance().incr(key);

            if (current == 0)
            {
                result.allowed = true;
                result.remaining = limit;
                result.resetSeconds = windowSeconds;
                return result;
            }

            if (current == 1)
            {
                RedisClient::instance().expire(key, windowSeconds);
            }

            int ttl = RedisClient::instance().ttl(key);
            if (ttl < 0)
            {
                ttl = windowSeconds;
            }

            result.allowed = current <= limit;
            result.remaining = std::max(0, limit - static_cast<int>(current));
            result.resetSeconds = ttl;

            return result;
        }
    };

}