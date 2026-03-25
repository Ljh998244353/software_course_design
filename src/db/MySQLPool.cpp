#include "MySQLPool.h"
#include <iostream>

namespace auction {

MySQLPool& MySQLPool::instance() {
    static MySQLPool instance;
    return instance;
}

bool MySQLPool::init(const DatabaseConfig& config) {
    config_ = config;
    return true;
}

void MySQLPool::close() {
}

class MySQLResultSet : public MySQLPool::ResultSet {
public:
    MySQLResultSet() {}
    bool next() override { return false; }
    int64_t getInt64(const std::string& col) override { (void)col; return 0; }
    int getInt(const std::string& col) override { (void)col; return 0; }
    double getDouble(const std::string& col) override { (void)col; return 0.0; }
    std::string getString(const std::string& col) override { (void)col; return ""; }
};

std::shared_ptr<MySQLPool::ResultSet> MySQLPool::executeQuery(const std::string& sql,
                                                              const std::vector<std::string>& params) {
    (void)sql;
    (void)params;
    return std::make_shared<MySQLResultSet>();
}

int MySQLPool::executeUpdate(const std::string& sql,
                            const std::vector<std::string>& params) {
    (void)sql;
    (void)params;
    return 0;
}

} // namespace auction
