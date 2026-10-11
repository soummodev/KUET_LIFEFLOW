#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>

class BloodBankSystem;

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;

    std::string getParam(const std::string& key, const std::string& def = "") const {
        auto it = params.find(key);
        return it != params.end() ? it->second : def;
    }
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json";
    std::string body;
    std::map<std::string, std::string> headers;

    static HttpResponse json(const std::string& jsonBody, int statusCode = 200) {
        HttpResponse res;
        res.status = statusCode;
        res.contentType = "application/json; charset=utf-8";
        res.body = jsonBody;
        return res;
    }

    static HttpResponse html(const std::string& htmlBody, int statusCode = 200) {
        HttpResponse res;
        res.status = statusCode;
        res.contentType = "text/html; charset=utf-8";
        res.body = htmlBody;
        return res;
    }

    static HttpResponse file(const std::string& content, const std::string& mimeType) {
        HttpResponse res;
        res.status = 200;
        res.contentType = mimeType;
        res.body = content;
        return res;
    }

    static HttpResponse error(const std::string& message, int statusCode = 400) {
        HttpResponse res;
        res.status = statusCode;
        res.contentType = "application/json; charset=utf-8";
        res.body = "{\"success\":false,\"error\":\"" + message + "\"}";
        return res;
    }
};

class HttpServer {
private:
    int port;
    std::string webRoot;
    BloodBankSystem& system;
    bool running;
    uintptr_t serverSocket;

    void handleClient(uintptr_t clientSocket);
    HttpResponse routeRequest(const HttpRequest& req);
    bool serveStaticFile(const std::string& path, HttpResponse& res);
    static std::string getMimeType(const std::string& path);
    static HttpRequest parseRequest(const std::string& raw);
    static std::string urlDecode(const std::string& str);
    static void parseQueryParams(const std::string& query, std::map<std::string, std::string>& params);

public:
    HttpServer(int port, const std::string& webRoot, BloodBankSystem& sys);
    ~HttpServer();

    void start();
    void stop();
    bool isRunning() const { return running; }
};

#endif // HTTPSERVER_H
