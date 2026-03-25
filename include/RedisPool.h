#pragma once

#include <memory>
#include <string>
#include <vector>
#include <hiredis/hiredis.h>

namespace auction {

class RedisConnection {
public:
    RedisConnection(redisContext* ctx) : ctx_(ctx) {}
    ~RedisConnection() { if (ctx_) redisFree(ctx_); }

    redisContext* get() { return ctx_; }
    explicit operator bool() const { return ctx_ != nullptr; }

private:
    redisContext* ctx_;
};

class RedisPool {
public:
    static RedisPool& instance();

    bool init(const std::string& host, int port, int pool_size);
    void close();

    std::unique_ptr<RedisConnection> getConnection();
    std::string get(const std::string& key);
    bool set(const std::string& key, const std::string& value);
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    std::string hget(const std::string& key, const std::string& field);
    std::vector<std::string> hgetall(const std::string& key);
    bool zadd(const std::string& key, double score, const std::string& member);
    std::vector<std::string> zrange(const std::string& key, int start, int stop);
    long long publish(const std::string& channel, const std::string& message);

    std::string eval(const std::string& script, const std::vector<std::string>& keys,
                    const std::vector<std::string>& args);

private:
    RedisPool() = default;

    struct PoolEntry {
        redisContext* context;
        bool in_use;
    };

    std::vector<PoolEntry> pool_;
    std::mutex mutex_;
    int pool_size_{0};
};

} // namespace auction
