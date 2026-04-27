#include "../../include/handlers/RegisterHandler.h"

void RegisterHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp){
    json parsed = json::parse(req.getBody());
    string username = parsed["username"];
    string password = parsed["password"];

    int userId = insertUser(username, password);
    if(userId != -1){
        // 插入成功
        // 封装成功响应
        json successResp;   
        successResp["status"] = "success";
        successResp["message"] = "Register successful";
        successResp["userId"] = userId;
        std::string successBody = successResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
    }
    else {
        // 插入失败
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = "username already exists";
        std::string failureBody = failureResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k409Conflict, "Conflict");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}

int RegisterHandler::insertUser(const string& username, const string& password){
    // 如果不存在
    if(!isUserExist(username)){
        // 插入用户
        string sql = "INSERT INTO users (username, password) VALUES ('" + username + "', '" + password + "')";
        mysqlUtil_.executeUpdate(sql);
        string sql2 = "SELECT id FROM users WHERE username = '" + username + "'";
        sql::ResultSet*  res = mysqlUtil_.executeQuery(sql2);
        if(res->next()){
            return res->getInt("id");
        }
    }
    return -1;
}

bool RegisterHandler::isUserExist(const string& username){
    string sql = "SELECT id FROM users WHERE username = '" + username + "'";
    sql::ResultSet* res = mysqlUtil_.executeQuery(sql);
    if(res->next()){
        delete res;
        return true;
    }
    if(res) delete res;
    return false;
}