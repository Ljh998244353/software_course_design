#pragma once

#include "ConfigManager.h"
#include <string>
#include <vector>
#include <vector>
#include <memory>
#include <mutex>

namespace auction {

class MySQLPool {
public:
    static MySQLPool& instance();

    bool init(const DatabaseConfig& config);
    void close();

    struct ResultSet {
        virtual ~ResultSet() = default;
        virtual bool next() = 0;
        virtual int64_t getInt64(const std::string& col) = 0;
        virtual int getInt(const std::string& col) = 0;
        virtual double getDouble(const std::string& col) = 0;
        virtual std::string getString(const std::string& col) = 0;
    };

    std::shared_ptr<ResultSet> executeQuery(const std::string& sql,
                                           const std::vector<std::string>& params = {});
    int executeUpdate(const std::string& sql,
                      const std::vector<std::string>& params = {});

private:
    MySQLPool() = default;
    DatabaseConfig config_;
};

} // namespace auction
