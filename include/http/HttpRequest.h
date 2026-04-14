#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <muduo/base/Timestamp.h>
using namespace std;

namespace http{

class HttpRequest{
public:
    enum Method{
        kInvalid, kGet, kPost, kHead, kDelete, kOptions
    };

    HttpRequest():
        method_(kInvalid),
        version_("Unkown") {}

    void setReceiveTime(muduo::Timestamp t);
    muduo::Timestamp receiveTime() const {return receiveTime_;}

    bool setMethod(const char* start, const char* end);
    Method method() const {return method_;}

    void setPath(const char* start, const char* end);
    string path() const {return path_;}

    void setPathParameter(const string& key, const string& value);
    string getPathParameter(const string& key) const;

    void setQueryParameter(const char* start, const char* end);
    string getQueryParameter(const string& key) const;

    void setVersion(string v){
        version_ = v;
    }

    string getVersion() const{
        return version_;
    }

    void addHeader(const char* start, const char* colon, const char* end);
    string getHeader(const string& field) const;

    const map<string, string>& headers() const{
        return headers_;
    }

    void setBody(const string& body) {content_ = body;}
    void setBody(const char* start, const char* end){
        if(end >= start){
            content_.assign(start, end - start);
        }
    }

    string getBody() const{
        return content_;
    }
    void setContentLength(uint64_t length){
        contentLength_ = length;
    }

    uint64_t contentLength() const{
        return contentLength_;
    }

    void swap(HttpRequest& that);


private:
    Method        method_;   // 请求方法
    string        version_;  // HTTP版本
    string        path_;     // 请求路径
    unordered_map<string, string>   pathParameters_;  // 请求路径参数
    unordered_map<string, string>   queryParameters_; // 请求查询参数
    muduo::Timestamp                receiveTime_;     // 请求接收时间
    
    map<string, string>             headers_;         // 请求头部字段
    string                          content_;         // 请求体
    uint64_t                        contentLength_{ 0 };   // 请求体长度

};


}