#include "../../include/handlers/MenuHandler.h"

void MenuHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp){
    try{
        auto session = server_->getSessionManager()->getSession(req, resp);
        LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
        if(session->getValue("isLoggedIn") != "true"){
            // 用户未登录，返回未授权错误
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Unauthorized";
            std::string errorBody = errorResp.dump(4);

            server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                                "Unauthorized", true, "application/json", errorBody.size(),
                                 errorBody, resp);
            return;
        }

        int userId = stoi(session->getValue("userId"));
        string username = session->getValue("username");

        string reqFile = ("../../resource/menu.html");
        FileUtil fileOperater(reqFile);
        if(!fileOperater.isValid()){
            LOG_WARN << reqFile << "not exist.";
            fileOperater.resetDefaultFile();
        }

        vector<char> buffer(fileOperater.size());
        fileOperater.readFile(buffer);
        string htmlContent(buffer.data(), buffer.size());

        // 在html中插入userId
        size_t headEnd = htmlContent.find("</head>");
        if(headEnd != string::npos){
            std::string script = "<script>const userId = '" + std::to_string(userId) + "';</script>";
            htmlContent.insert(headEnd, script);
        }

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("text/html");
        resp->setContentLength(htmlContent.size());
        resp->setBody(htmlContent);
    }

    catch(const exception& e){
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