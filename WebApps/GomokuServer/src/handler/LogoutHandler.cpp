#include "../../include/handlers/LogoutHandler.h"

void LogoutHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp){
    auto contentType = req.getHeader("Content-Type");
    if(contentType.empty() || contentType != "application/json" || req.getBody().empty()){
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(0);
        resp->setBody("");
        return;
    }

    try{
        auto session = server_->getSessionManager()->getSession(req, resp);
        int userId = stoi(session->getValue("userId"));
        session->clear();
        server_->getSessionManager()->destroySession(session->getId());

        json parsed = json::parse(req.getBody());
        int gameType = parsed["gameType"];

        {   // 释放资源
            lock_guard<mutex> lock(server_->mutexForAiGames_);
            server_->onlineUsers_.erase(userId);
        }

        if(gameType == GomokuServer::MAN_VS_AI){
            lock_guard<mutex> lock(server_->mutexForAiGames_);
            server_->aiGames_.erase(userId);
        }
        else if(gameType == GomokuServer::MAN_VS_MAN){
            // 释放相应创造资源，并且通知另一个用户对方已经主动退出游戏
        }

        // 返回响应报文
        json response;
        response["message"] = "logout successful";
        std::string responseBody = response.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(responseBody.size());
        resp->setBody(responseBody);
    }
    catch (const exception& e){
        // 捕获异常，返回错误信息
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}