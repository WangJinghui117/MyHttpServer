#pragma once
#include <muduo/net/TcpServer.h>
using namespace std;

namespace http{

class HttpResponse{

public:
    enum HttpStatusCode{
        kUnkown,
        k200Ok = 200,
        k204NoContent = 204,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k409Conflict = 409,
        k500InternalServerError = 500,
    };

    HttpResponse(bool close = true): statusCode_(kUnkown), closeConnection_(close){}

    void setVersion(string version){
        httpVersion_ = version;
    }

    void setStatusCode(HttpStatusCode code){
        statusCode_ = code;
    }

    HttpStatusCode getStatusCode() const{
        return statusCode_;
    }

    void setStatusMessage(const string& messages){
        statusMessage_ = messages;
    }

    void setCloseConnection(bool on){
        closeConnection_ = on;
    }

    bool closeConnection() const{
        return closeConnection_;
    }

    void setContentType(const string& contentType){
        addHeader("Content-Type", contentType);
    }

    void setContentLength(uint64_t length){
        addHeader("Content-Length", to_string(length));
    }

    void addHeader(const string& key, const string& value){
        headers_[key] = value;
    }

    void setBody(const string& body){
        body_ = body;
    }

    void setStatusLine(const string& version, HttpStatusCode statusCode, const string& statusMessage);

    void setErrorHeader(){}

    void appendToBuffer(muduo::net::Buffer* outputBuf) const;


private:
    string               httpVersion_;
    HttpStatusCode       statusCode_;
    string               statusMessage_;
    bool                 closeConnection_;
    map<string, string>  headers_;
    string               body_;
    bool                 isFile_;
};

}