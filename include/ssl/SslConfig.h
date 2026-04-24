#pragma once
#include "SslTypes.h"
#include <string>
#include <vector>
using namespace std;

namespace ssl{

class SslConfig{

public:
    SslConfig();
    ~SslConfig() = default;

    // 证书配置     : 证书文件    私钥文件      证书链文件
    void setCertificateFile(const string& certFile) { certFile_ = certFile; }
    void setPrivateKeyFile(const string& keyFile) { keyFile_ = keyFile; }
    void setCertificateChainFile(const string& chainFile) { chainFile_ = chainFile; }

    // 协议版本 & 加密套件    TLS/SSL version    加密套件
    void setProtocolVersion(SSLVersion version) { version_ = version; }
    void setCipherList(const string& cipherList) { cipherList_ = cipherList; }

    // 客户端验证配置      是否验证客户端    验证深度
    void setVerifyClient(bool verify) { verifyClient_ = verify; }
    void setVerifyDepth(int depth) { verifyDepth_ = depth; }

    // 会话设置：         超时时间   缓存大小
    void setSessionTimeout(int seconds) { sessionTimeout_ = seconds; }
    void setSessionCacheSizes(long size) { sessionCacheSize_ = size; }

    // 获取配置 getters
    const string& getCertificateFile() const { return certFile_; }
    const string& getPrivateKeyFile() const { return keyFile_; }
    const string& getCertificateChainFile() const { return chainFile_; }
    SSLVersion getProtocolVersion() const {return version_; }
    const string& getCipherList() const { return cipherList_; }
    bool getVerifyClient() const { return verifyClient_; }
    int getVerifyDepth() const { return verifyDepth_; }
    int getSessionTimeout() const { return sessionTimeout_; }
    long getSessionCacheSize() const { return sessionCacheSize_; }

private:
    string            certFile_;   // 证书文件
    string            keyFile_;    // 私钥文件
    string            chainFile_;  // 证书链文件
    SSLVersion        version_;    // TLS/SSL版本
    string            cipherList_; // 加密套件
    bool              verifyClient_; // 是否验证客户端
    int               verifyDepth_;  // 验证深度
    int               sessionTimeout_;// 会话超时时间
    long              sessionCacheSize_; // 缓存大小

};


}