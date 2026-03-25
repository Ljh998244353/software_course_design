#include "RedisPool.h"
#include <hiredis/hiredis.h>
#include <iostream>

namespace auction {

RedisPool& RedisPool::instance() {
    static RedisPool instance;
    return instance;
}

bool RedisPool::init(const std::string& host, int port, int pool_size) {
    pool_size_ = pool_size;
    pool_.resize(pool_size_);

    for (int i = 0; i < pool_size_; ++i) {
        redisContext* ctx = redisConnect(host.c_str(), port);
        if (ctx == nullptr || ctx->err) {
            if (ctx) {
                std::cerr << "Redis connection error: " << ctx->errstr << std::endl;
                redisFree(ctx);
            }
            return false;
        }
        pool_[i].context = ctx;
        pool_[i].in_use = false;
    }
    return true;
}

void RedisPool::close() {
    for (auto& entry : pool_) {
        if (entry.context) {
            redisFree(entry.context);
            entry.context = nullptr;
        }
    }
    pool_.clear();
}

std::unique_ptr<RedisConnection> RedisPool::getConnection() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& entry : pool_) {
        if (!entry.in_use && entry.context) {
            entry.in_use = true;
            return std::make_unique<RedisConnection>(entry.context);
        }
    }
    return nullptr;
}

std::string RedisPool::get(const std::string& key) {
    auto conn = getConnection();
    if (!conn) return "";

    auto* reply = (redisReply*)redisCommand(conn->get(), "GET %s", key.c_str());
    if (reply == nullptr) return "";

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    freeReplyObject(reply);
    return result;
}

bool RedisPool::set(const std::string& key, const std::string& value) {
    auto conn = getConnection();
    if (!conn) return false;

    auto* reply = (redisReply*)redisCommand(conn->get(), "SET %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) return false;

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return success;
}

bool RedisPool::hset(const std::string& key, const std::string& field, const std::string& value) {
    auto conn = getConnection();
    if (!conn) return false;

    auto* reply = (redisReply*)redisCommand(conn->get(), "HSET %s %s %s",
                                            key.c_str(), field.c_str(), value.c_str());
    if (reply == nullptr) return false;

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return success;
}

std::string RedisPool::hget(const std::string& key, const std::string& field) {
    auto conn = getConnection();
    if (!conn) return "";

    auto* reply = (redisReply*)redisCommand(conn->get(), "HGET %s %s", key.c_str(), field.c_str());
    if (reply == nullptr) return "";

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    freeReplyObject(reply);
    return result;
}

std::vector<std::string> RedisPool::hgetall(const std::string& key) {
    std::vector<std::string> result;
    auto conn = getConnection();
    if (!conn) return result;

    auto* reply = (redisReply*)redisCommand(conn->get(), "HGETALL %s", key.c_str());
    if (reply == nullptr) return result;

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            auto* elem = reply->element[i];
            if (elem->type == REDIS_REPLY_STRING) {
                result.push_back(std::string(elem->str, elem->len));
            }
        }
    }
    freeReplyObject(reply);
    return result;
}

bool RedisPool::zadd(const std::string& key, double score, const std::string& member) {
    auto conn = getConnection();
    if (!conn) return false;

    auto* reply = (redisReply*)redisCommand(conn->get(), "ZADD %s %f %s",
                                            key.c_str(), score, member.c_str());
    if (reply == nullptr) return false;

    bool success = reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return success;
}

std::vector<std::string> RedisPool::zrange(const std::string& key, int start, int stop) {
    std::vector<std::string> result;
    auto conn = getConnection();
    if (!conn) return result;

    auto* reply = (redisReply*)redisCommand(conn->get(), "ZRANGE %s %d %d",
                                            key.c_str(), start, stop);
    if (reply == nullptr) return result;

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            auto* elem = reply->element[i];
            if (elem->type == REDIS_REPLY_STRING) {
                result.push_back(std::string(elem->str, elem->len));
            }
        }
    }
    freeReplyObject(reply);
    return result;
}

long long RedisPool::publish(const std::string& channel, const std::string& message) {
    auto conn = getConnection();
    if (!conn) return 0;

    auto* reply = (redisReply*)redisCommand(conn->get(), "PUBLISH %s %s",
                                            channel.c_str(), message.c_str());
    if (reply == nullptr) return 0;

    long long result = reply->integer;
    freeReplyObject(reply);
    return result;
}

std::string RedisPool::eval(const std::string& script, const std::vector<std::string>& keys,
                            const std::vector<std::string>& args) {
    auto conn = getConnection();
    if (!conn) return "";

    std::vector<const char*> argv;
    argv.push_back(script.c_str());
    argv.push_back(std::to_string(keys.size()).c_str());
    for (const auto& k : keys) argv.push_back(k.c_str());
    for (const auto& a : args) argv.push_back(a.c_str());

    auto* reply = (redisReply*)redisCommand(conn->get(), argv[0],
                                            std::to_string(keys.size()).c_str(),
                                            keys[0].c_str(),
                                            args[0].c_str());
    if (reply == nullptr) return "";

    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    } else if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            auto* elem = reply->element[i];
            if (elem->type == REDIS_REPLY_STRING) {
                result += std::string(elem->str, elem->len) + ",";
            }
        }
    }
    freeReplyObject(reply);
    return result;
}

} // namespace auction
