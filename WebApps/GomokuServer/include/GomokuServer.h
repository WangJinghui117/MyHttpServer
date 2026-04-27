#pragma once

#include <atomic>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <mutex>


#include "AiGame.h"
#include "../../../HttpServer/include/http/HttpServer.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../../../HttpServer/include/utils/FileUtil.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"

class LoginHandler;
class EntryHandler;
class RegisterHandler;
class MenuHandler;
class AiGameStartHandler;
class LogoutHandler;
class AiGameMoveHandler;
class GameBackendHandler;

#define DURING_GAME 1
#define GAME_OVER 2

#define MAX_AIBOT_NUM 4096

class GomokuServer{

public:
    GomokuServer(int port, const string& name, muduo::net::TcpServer::Option options = muduo::net::TcpServer::kNoReusePort);

    void setThreadNum(int numThreads);
    void start();

private:
    void initialize();
    void initializeSession();
    void initializeRouter();
    void initializeMiddleware();

    void setSessionManager(unique_ptr<http::session::SessionManager> manager){
        httpServer_.setSessionManager(move(manager));
    }

    http::session::SessionManager* getSessionManager() const{
        return httpServer_.getSessionManager();
    }

    void restartChessGameVsAi(const http::HttpRequest& req, http::HttpResponse* resp);
    void getBackendData(const http::HttpRequest& req, http::HttpResponse* resp);
    void packageResp(const string& version, http::HttpResponse::HttpStatusCode statusCode,
                     const string& statusMsg, bool close, const string& contentType, int contentLen, const string& body, http::HttpResponse* resp);

    int getMaxOnline() const{
        return maxOnline_.load();
    }

    int getCurOnline() const{
        return onlineUsers_.size();
    }

    void updateMaxOnline(int online){
        maxOnline_ = max(maxOnline_.load(), online);
    }

    int getUserCount(){
        string sql = "SELECT COUNT(*) as count From users";
        sql::ResultSet* res = mysqlUtil_.executeQuery(sql);
        if(res->next()){
            return res->getInt("count");
        }
        return 0;
    }

private:
    friend class EntryHandler;
    friend class LoginHandler;
    friend class RegisterHandler;
    friend class MenuHandler;
    friend class AiGameStartHandler;
    friend class LogoutHandler;
    friend class AiGameMoveHandler;
    friend class GameBackendHandler;    



private:
    enum GameType{
        NO_GAME = 0,
        MAN_VS_AI = 1,
        MAN_VS_MAN = 2
    };

    http::HttpServer                        httpServer_;
    http::MysqlUtil                         mysqlUtil_;

    unordered_map<int, shared_ptr<AiGame>>  aiGames_;             // userId -> AiBot
    mutex                                   mutexForAiGames_;

    unordered_map<int, bool>                onlineUsers_;         // userId -> 是否在游戏中
    mutex                                   mutexForOnlineUsers_;

    atomic<int>                             maxOnline_;           // 最高在线人数

};