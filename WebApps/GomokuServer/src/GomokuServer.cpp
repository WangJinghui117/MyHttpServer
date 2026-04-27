#include "../include/handlers/EntryHandler.h"
#include "../include/handlers/LoginHandler.h"
#include "../include/handlers/RegisterHandler.h"
#include "../include/handlers/MenuHandler.h"
#include "../include/handlers/AiGameStartHandler.h"
#include "../include/handlers/LogoutHandler.h"
#include "../include/handlers/AiGameMoveHandler.h"
#include "../include/handlers/GameBackendHandler.h"
#include "../include/GomokuServer.h"
#include "../../../HttpServer/include/http/HttpRequest.h"
#include "../../../HttpServer/include/http/HttpResponse.h"
#include "../../../HttpServer/include/http/HttpServer.h"

using namespace http;

GomokuServer::GomokuServer(int port, const string& name, muduo::net::TcpServer::Option option)
    : httpServer_(port, name, option), maxOnline_(0)
{
    initialize();
}

void GomokuServer::setThreadNum(int numThreads){
    httpServer_.setThreadNum(numThreads);
}

void GomokuServer::start(){
    httpServer_.start();
}

void GomokuServer::initialize(){
    // 初始化数据库连接池
    http::MysqlUtil::init("tcp://127.0.0.1:3306", "root", "2467", "Gomoku", 10);
    // 初始化会话
    initializeSession();
    // 初始化中间件
    initializeMiddleware();
    // 初始化路由
    initializeRouter();
}

void GomokuServer::initializeSession(){
    // 创建会话存储
    auto sessionStorage = make_unique<session::MemorySessionStorage>();
    // 创建会话管理器
    auto sessionManager = make_unique<session::SessionManager>(move(sessionStorage));
    // 设置会话管理器
    setSessionManager(move(sessionManager));
}

void GomokuServer::initializeMiddleware(){
    // 创建中间件
    auto corsMiddleware = make_shared<middleware::CorsMiddleware>();
    // 添加中间件
    httpServer_.addMiddleware(corsMiddleware);
}

void GomokuServer::initializeRouter(){
    // 注册url回调处理器
    // 登录注册入口页面
    httpServer_.Get("/", make_shared<EntryHandler>(this));
    httpServer_.Get("/entry", make_shared<EntryHandler>(this));
    // 登录
    httpServer_.Post("/login", make_shared<LoginHandler>(this));
    // 注册
    httpServer_.Post("/register", make_shared<RegisterHandler>(this));
    // 登出
    httpServer_.Post("/user/logout", make_shared<LogoutHandler>(this));
    // 菜单页面
    httpServer_.Get("/menu", make_shared<MenuHandler>(this));
    // 开始对战
    httpServer_.Get("/aiBot/start", make_shared<AiGameStartHandler>(this));
    // 下棋
    httpServer_.Post("/aiBot/move", make_shared<AiGameMoveHandler>(this));
    // 重新开始
    httpServer_.Get("/aiBot/restart", [this](const http::HttpRequest& req, http::HttpResponse* resp){
        restartChessGameVsAi(req, resp);
    });

    // 后台界面
    httpServer_.Get("/backend", make_shared<GameBackendHandler>(this));
    // 后台数据获取
    httpServer_.Get("/backend_data", [this](const http::HttpRequest& req, http::HttpResponse* resp){
        getBackendData(req, resp);
    });
}

void GomokuServer::restartChessGameVsAi(const http::HttpRequest& req, http::HttpResponse* resp){
    // 解析请求体
    auto session = getSessionManager()->getSession(req, resp);

    // 没有登录， 返回未授权错误
    if(session->getValue("isLoggedIn") != "true"){
        json errorResp;
        errorResp["status"] = "error";
        errorResp["message"] = "Unauthorized";
        string errorBody = errorResp.dump(4);

        packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                     "Unauthorized", true, "application/json", errorBody.size(), errorBody, resp);
        return;
    }

    int userId = stoi(session->getValue("userId"));
    {
        // restart
        lock_guard<mutex> lock(mutexForAiGames_);
        if(aiGames_.find(userId) != aiGames_.end()){
            aiGames_.erase(userId);
        }
        aiGames_[userId] = make_shared<AiGame>(userId);
    }

    json successResp;
    successResp["status"] = "ok";
    successResp["message"] = "restart successful";
    successResp["userId"] = userId;
    string successBody = successResp.dump(4);
    packageResp(req.getVersion(), http::HttpResponse::k200Ok, 
                "OK", false, "application/json", successBody.size(), successBody, resp);
}

void GomokuServer::getBackendData(const http::HttpRequest& req, http::HttpResponse* resp){
    try{
        // 获取数据
        int curOnline = getCurOnline();
        LOG_INFO << "当前在线人数：" << curOnline;

        int maxOnline = getMaxOnline();
        LOG_INFO << "历史最高在线人数：" << maxOnline;

        int totalUser = getUserCount();
        LOG_INFO << "注册用户人数：" << totalUser;

        // 构造JSON 响应
        nlohmann::json respBody;
        respBody = {
            {"curOnline", curOnline},
            {"maxOnline", maxOnline},
            {"totalUser", totalUser}
        };

        // 转换为字符串
        string responseStr = respBody.dump(4);

        // 设置响应
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setContentType("application/json");
        resp->setBody(responseStr);
        resp->setContentLength(responseStr.size());
        resp->setCloseConnection(false);

        LOG_INFO << "Backend data response prepared successfully";
    }
    catch(const exception& e){
        LOG_ERROR << "Error in getBackendData: " << e.what();
        // 错误响应
        nlohmann::json errorBody = {
            {"error", "Internal Server Error"},
            {"message", e.what()}
        };

        string errorStr = errorBody.dump();
        resp->setStatusCode(http::HttpResponse::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setContentType("application/json");
        resp->setBody(errorStr);
        resp->setContentLength(errorStr.size());
        resp->setCloseConnection(true);
    }
}

void GomokuServer::packageResp(const string& version, http::HttpResponse::HttpStatusCode statusCode,
                               const string& statusMsg, bool close, const string& contentType, int contentLen, 
                               const string& body, http::HttpResponse* resp)
{
    if(resp == nullptr){
        LOG_ERROR << "Response pointer is null";
        return;
    }
    try{
        resp->setVersion(version);
        resp->setStatusCode(statusCode);
        resp->setStatusMessage(statusMsg);
        resp->setCloseConnection(close);
        resp->setContentType(contentType);
        resp->setContentLength(contentLen);
        resp->setBody(body);
        
        LOG_INFO << "Response packaged successfully";
    }
    catch (const exception& e){
        LOG_ERROR << "Error in packageResp: " << e.what();
        // 设置一个基本的错误响应
        resp->setStatusCode(http::HttpResponse::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setCloseConnection(true);
    }
}