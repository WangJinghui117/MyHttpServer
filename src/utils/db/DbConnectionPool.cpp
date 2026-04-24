#include "../../../include/utils/db/DbConnectionPool.h"
#include "../../../include/utils/db/DbException.h"
#include <muduo/base/Logging.h>

namespace http{
namespace db{

void DbConnectionPool::init(const string& host, const string& user, 
                const string& password, const string & database, size_t poolSize){
    
    // 连接池会被多个线程访问，加锁
    lock_guard<mutex> lock(mutex_);
    if(initialized){
        return;
    }
    host_ = host;
    user_ = user;
    password_ = password;
    database_ = database;

    // 创建链接
    for(size_t i = 0; i < poolSize; i++){
        connections_.push(createConnection());
    }
    initialized = true;
    LOG_INFO << "Database connection pool initialized with " << poolSize << " connections";
}

DbConnectionPool::DbConnectionPool(){
    checkThread_ = std::thread(&DbConnectionPool::checkConnections, this);
    checkThread_.detach();
}

DbConnectionPool::~DbConnectionPool(){
    lock_guard<mutex> lock(mutex_);
    while(!connections_.empty()){
        connections_.pop();
    }
    LOG_INFO << "Database connection pool destroyed";
}

shared_ptr<DbConnection> DbConnectionPool::getConnection(){
    shared_ptr<DbConnection> conn;
    {
        unique_lock<mutex> lock(mutex_);
        while(connections_.empty()){
            if(!initialized){
                throw DbException("Connection pool not initialized");
            }
            LOG_INFO << "waiting for available connection...";
            cv_.wait(lock);
        }
        conn = connections_.front();
        connections_.pop();
    } // 释放锁

    try{
        // 在锁外检查连接
        if(!conn->ping()){
            LOG_WARN << "Connection lost, attempting to reconnect...";
            conn->reconnect();
        }

        return shared_ptr<DbConnection>(conn.get(), [this, conn](DbConnection*){
            lock_guard<mutex> lock(mutex_);
            connections_.push(conn);
            cv_.notify_one();
        });
    }
    catch(const exception& e){
        LOG_ERROR << "Failed to get connection: " << e.what();
        {
            lock_guard<mutex> lock(mutex_);
            connections_.push(conn);
            cv_.notify_one();
        }
        throw;
    }
}


shared_ptr<DbConnection> DbConnectionPool::createConnection(){
    return make_shared<DbConnection>(host_, user_, password_, database_);
}

void DbConnectionPool::checkConnections(){
    while(true){
        try{
            vector<shared_ptr<DbConnection>> connsToCheck;
            {
                unique_lock<mutex> lock(mutex_);
                if(connections_.empty()){
                    this_thread::sleep_for(chrono::seconds(1));
                    continue;
                }
                auto temp = connections_;
                while(!temp.empty()){
                    connsToCheck.push_back(temp.front());
                    temp.pop();
                }
            }

            //在锁外检查连接
            for(auto& conn : connsToCheck){
                if(!conn->ping()){
                    try{
                        conn->reconnect();
                    }
                    catch(const exception& e){
                        LOG_ERROR << "Failed to reconnect: " << e.what();
                    }
                }
            }
            this_thread::sleep_for(chrono::seconds(60));
        }
        catch (const exception& e){
            LOG_ERROR << "Error in check thread: " << e.what();
            this_thread::sleep_for(chrono::seconds(5));
        }
    }
}

}
}