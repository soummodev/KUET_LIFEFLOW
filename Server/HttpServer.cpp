#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSE_SOCKET(s) closesocket(s)
#define IS_INVALID_SOCKET(s) ((s) == INVALID_SOCKET)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int socket_t;
#define CLOSE_SOCKET(s) close(s)
#define IS_INVALID_SOCKET(s) ((s) < 0)
#endif

#include "HttpServer.h"
#include "../Core/BloodBankSystem.h"
#include "../Core/Utils.h"
#include "../Algorithms/Search.h"
#include "../Algorithms/MergeSort.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <cstring>
#include <cctype>
#include <algorithm>

using namespace std;

// =====================================================================================
//  JSON UTILITIES
// =====================================================================================
static string jsonEscape(const string& s) {
    ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u" << hex << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

static string jsonExtractString(const string& json, const string& key, const string& def = "") {
    string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == string::npos) return def;

    pos = json.find(':', pos + searchKey.length());
    if (pos == string::npos) return def;

    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == string::npos) return def;

    if (json[pos] == '"') {
        size_t start = pos + 1;
        size_t end = start;
        while (end < json.length()) {
            if (json[end] == '"' && json[end - 1] != '\\') break;
            end++;
        }
        if (end <= json.length()) {
            string raw = json.substr(start, end - start);
            string res;
            for (size_t i = 0; i < raw.length(); i++) {
                if (raw[i] == '\\' && i + 1 < raw.length()) {
                    if (raw[i + 1] == '"') { res += '"'; i++; continue; }
                    if (raw[i + 1] == '\\') { res += '\\'; i++; continue; }
                    if (raw[i + 1] == 'n') { res += '\n'; i++; continue; }
                }
                res += raw[i];
            }
            return res;
        }
    }
    return def;
}

static int jsonExtractInt(const string& json, const string& key, int def = 0) {
    string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == string::npos) return def;

    pos = json.find(':', pos + searchKey.length());
    if (pos == string::npos) return def;

    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == string::npos) return def;

    if (json[pos] == '"') {
        string s = jsonExtractString(json, key, to_string(def));
        return Utils::toInt(s, def);
    }

    size_t end = json.find_first_of(",}\t\r\n", pos);
    string numStr = json.substr(pos, end - pos);
    return Utils::toInt(Utils::trim(numStr), def);
}

static bool jsonExtractBool(const string& json, const string& key, bool def = false) {
    string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == string::npos) return def;

    pos = json.find(':', pos + searchKey.length());
    if (pos == string::npos) return def;

    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == string::npos) return def;

    if (json.substr(pos, 4) == "true" || json.substr(pos, 1) == "1" || json.substr(pos, 6) == "\"true\"") return true;
    if (json.substr(pos, 5) == "false" || json.substr(pos, 1) == "0" || json.substr(pos, 7) == "\"false\"") return false;
    return def;
}

// =====================================================================================
//  HTTP SERVER CORE
// =====================================================================================
HttpServer::HttpServer(int p, const string& root, BloodBankSystem& sys)
    : port(p), webRoot(root), system(sys), running(false), serverSocket(0) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

HttpServer::~HttpServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

string HttpServer::urlDecode(const string& str) {
    string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%') {
            if (i + 2 < str.length()) {
                int value = 0;
                istringstream is(str.substr(i + 1, 2));
                if (is >> hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i];
                }
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

void HttpServer::parseQueryParams(const string& query, map<string, string>& params) {
    if (query.empty()) return;
    stringstream ss(query);
    string pair;
    while (getline(ss, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq != string::npos) {
            string key = urlDecode(pair.substr(0, eq));
            string val = urlDecode(pair.substr(eq + 1));
            params[key] = val;
        } else if (!pair.empty()) {
            params[urlDecode(pair)] = "";
        }
    }
}

HttpRequest HttpServer::parseRequest(const string& raw) {
    HttpRequest req;
    size_t headerEnd = raw.find("\r\n\r\n");
    string headerPart = (headerEnd != string::npos) ? raw.substr(0, headerEnd) : raw;
    req.body = (headerEnd != string::npos && headerEnd + 4 < raw.length()) ? raw.substr(headerEnd + 4) : "";

    stringstream ss(headerPart);
    string requestLine;
    if (getline(ss, requestLine)) {
        if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();
        stringstream reqLineSs(requestLine);
        reqLineSs >> req.method;
        string fullPath;
        reqLineSs >> fullPath;

        size_t qPos = fullPath.find('?');
        if (qPos != string::npos) {
            req.path = fullPath.substr(0, qPos);
            req.query = fullPath.substr(qPos + 1);
            parseQueryParams(req.query, req.params);
        } else {
            req.path = fullPath;
        }
    }

    string line;
    while (getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t colon = line.find(':');
        if (colon != string::npos) {
            string key = Utils::toLower(Utils::trim(line.substr(0, colon)));
            string val = Utils::trim(line.substr(colon + 1));
            req.headers[key] = val;
        }
    }

    return req;
}

string HttpServer::getMimeType(const string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == string::npos) return "application/octet-stream";
    string ext = Utils::toLower(path.substr(dot));
    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".js") return "application/javascript; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".woff2") return "font/woff2";
    return "application/octet-stream";
}

bool HttpServer::serveStaticFile(const string& path, HttpResponse& res) {
    string filePath = webRoot;
    if (filePath.back() != '/' && filePath.back() != '\\') filePath += "/";
    if (path == "/" || path.empty()) {
        filePath += "index.html";
    } else {
        string clean = path;
        if (clean.front() == '/') clean = clean.substr(1);
        filePath += clean;
    }

    ifstream file(filePath, ios::binary);
    if (!file) return false;

    ostringstream ss;
    ss << file.rdbuf();
    res.body = ss.str();
    res.contentType = getMimeType(filePath);
    res.status = 200;
    return true;
}

void HttpServer::handleClient(uintptr_t clientSocket) {
    socket_t client = (socket_t)clientSocket;
    char buffer[16384];
    int bytesRead = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        HttpRequest req = parseRequest(string(buffer, bytesRead));

        HttpResponse res;
        if (req.method == "OPTIONS") {
            res.status = 204;
            res.contentType = "text/plain";
            res.body = "";
        } else if (req.path.rfind("/api/", 0) == 0) {
            res = routeRequest(req);
        } else {
            if (!serveStaticFile(req.path, res)) {
                // Fallback to index.html for SPA routing
                if (!serveStaticFile("/index.html", res)) {
                    res = HttpResponse::error("File not found", 404);
                }
            }
        }

        ostringstream responseStream;
        responseStream << "HTTP/1.1 " << res.status << " OK\r\n";
        responseStream << "Content-Type: " << res.contentType << "\r\n";
        responseStream << "Content-Length: " << res.body.size() << "\r\n";
        responseStream << "Access-Control-Allow-Origin: *\r\n";
        responseStream << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
        responseStream << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
        responseStream << "Connection: close\r\n\r\n";
        responseStream << res.body;

        string fullResponse = responseStream.str();
        send(client, fullResponse.c_str(), (int)fullResponse.length(), 0);
    }
    CLOSE_SOCKET(client);
}

void HttpServer::start() {
    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (IS_INVALID_SOCKET(server)) {
        cerr << "Failed to create socket\n";
        return;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (::bind(server, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Failed to bind to port " << port << "\n";
        CLOSE_SOCKET(server);
        return;
    }

    if (listen(server, 10) < 0) {
        cerr << "Failed to listen on socket\n";
        CLOSE_SOCKET(server);
        return;
    }

    serverSocket = (uintptr_t)server;
    running = true;

    cout << "\n=======================================================\n";
    cout << "  KUET LifeFlow Web Server is RUNNING!\n";
    cout << "  Local URL: http://localhost:" << port << "\n";
    cout << "  Web Root : " << webRoot << "\n";
    cout << "=======================================================\n\n";

    while (running) {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        socket_t client = accept(server, (sockaddr*)&clientAddr,
#ifdef _WIN32
                                 &clientLen
#else
                                 (socklen_t*)&clientLen
#endif
        );

        if (!running) break;
        if (IS_INVALID_SOCKET(client)) continue;

        // Handle connection in a detached thread for concurrency
        thread([this, client]() {
            this->handleClient((uintptr_t)client);
        }).detach();
    }
}

void HttpServer::stop() {
    running = false;
    if (serverSocket) {
        CLOSE_SOCKET((socket_t)serverSocket);
        serverSocket = 0;
    }
}

// =====================================================================================
//  REST API ROUTER & SERIALIZERS
// =====================================================================================
HttpResponse HttpServer::routeRequest(const HttpRequest& req) {
    const string& path = req.path;
    const string& method = req.method;

    // ---------------------------------------------------------------- AUTH: LOGIN
    if (path == "/api/login" && method == "POST") {
        string role = jsonExtractString(req.body, "role", "user");
        string username = jsonExtractString(req.body, "username");
        string password = jsonExtractString(req.body, "password");

        if (role == "admin") {
            if (system.getAdmin().login(username, password)) {
                system.logActivity("Admin logged in via Web");
                ostringstream res;
                res << "{\"success\":true,\"role\":\"admin\",\"adminId\":\"" << jsonEscape(username) << "\"}";
                return HttpResponse::json(res.str());
            } else {
                return HttpResponse::error("Invalid admin credentials", 401);
            }
        } else {
            User* u = system.authenticateUser(username, password);
            if (u) {
                system.logActivity("User logged in via Web: " + u->getName());
                Donor* d = Search::findDonorByPhone(system.getDonors(), u->getPhone());
                ostringstream res;
                res << "{\"success\":true,\"role\":\"user\",\"user\":{"
                    << "\"name\":\"" << jsonEscape(u->getName()) << "\","
                    << "\"phone\":\"" << jsonEscape(u->getPhone()) << "\","
                    << "\"age\":" << u->getAge() << ","
                    << "\"gender\":\"" << jsonEscape(u->getGender()) << "\","
                    << "\"address\":\"" << jsonEscape(u->getAddress()) << "\","
                    << "\"hall\":\"" << jsonEscape(u->getHall()) << "\","
                    << "\"bloodGroup\":\"" << jsonEscape(u->getBloodGroup()) << "\","
                    << "\"isDonor\":" << (d ? "true" : "false") << ","
                    << "\"donorStatus\":\"" << jsonEscape(d ? d->statusText() : "Not Registered") << "\","
                    << "\"lastDonationDate\":\"" << jsonEscape(d ? d->getLastDonationDate() : "N/A") << "\""
                    << "}}";
                return HttpResponse::json(res.str());
            } else {
                return HttpResponse::error("Invalid phone or password", 401);
            }
        }
    }

    // ---------------------------------------------------------------- AUTH: REGISTER
    if (path == "/api/register" && method == "POST") {
        string name = jsonExtractString(req.body, "name");
        string phone = jsonExtractString(req.body, "phone");
        int age = jsonExtractInt(req.body, "age", 0);
        string gender = jsonExtractString(req.body, "gender", "Male");
        string address = jsonExtractString(req.body, "address");
        string hall = jsonExtractString(req.body, "hall");
        string bloodGroup = jsonExtractString(req.body, "bloodGroup");
        string password = jsonExtractString(req.body, "password");

        string err;
        if (system.registerUserApi(name, phone, age, gender, address, hall, bloodGroup, password, err)) {
            return HttpResponse::json("{\"success\":true,\"message\":\"User registered successfully\"}");
        } else {
            return HttpResponse::error(err, 400);
        }
    }

    // ---------------------------------------------------------------- USER PROFILE
    if (path == "/api/user/profile" && method == "GET") {
        string phone = req.getParam("phone");
        User* u = Search::findUserByPhone(system.getUsers(), phone);
        if (!u) return HttpResponse::error("User not found", 404);

        Donor* d = Search::findDonorByPhone(system.getDonors(), u->getPhone());
        ostringstream res;
        res << "{\"success\":true,\"user\":{"
            << "\"name\":\"" << jsonEscape(u->getName()) << "\","
            << "\"phone\":\"" << jsonEscape(u->getPhone()) << "\","
            << "\"age\":" << u->getAge() << ","
            << "\"gender\":\"" << jsonEscape(u->getGender()) << "\","
            << "\"address\":\"" << jsonEscape(u->getAddress()) << "\","
            << "\"hall\":\"" << jsonEscape(u->getHall()) << "\","
            << "\"bloodGroup\":\"" << jsonEscape(u->getBloodGroup()) << "\","
            << "\"isDonor\":" << (d ? "true" : "false") << ","
            << "\"donorStatus\":\"" << jsonEscape(d ? d->statusText() : "Not Registered") << "\","
            << "\"lastDonationDate\":\"" << jsonEscape(d ? d->getLastDonationDate() : "N/A") << "\","
            << "\"available\":" << (d && d->isAvailable() ? "true" : "false")
            << "}}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- BECOME DONOR
    if (path == "/api/user/become-donor" && method == "POST") {
        string phone = jsonExtractString(req.body, "phone");
        string lastDate = jsonExtractString(req.body, "lastDonationDate", "N/A");
        string err;
        if (system.becomeDonorApi(phone, lastDate, err)) {
            return HttpResponse::json("{\"success\":true,\"message\":\"Successfully registered as a donor\"}");
        } else {
            return HttpResponse::error(err, 400);
        }
    }

    // ---------------------------------------------------------------- INVENTORY
    if (path == "/api/inventory" && method == "GET") {
        ostringstream res;
        res << "{\"success\":true,\"totalStock\":" << system.totalStock() << ",\"inventory\":[";
        for (int i = 0; i < BloodTree::TOTAL; i++) {
            const string& g = BloodTree::groupAt(i);
            int units = system.stockOf(g);
            string status = units == 0 ? "Out of stock" : (units <= 3 ? "Low" : "OK");
            if (i > 0) res << ",";
            res << "{\"group\":\"" << g << "\",\"units\":" << units << ",\"status\":\"" << status << "\"}";
        }
        res << "]}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- INVENTORY UPDATE
    if (path == "/api/inventory/update" && method == "POST") {
        string group = jsonExtractString(req.body, "group");
        string action = jsonExtractString(req.body, "action", "set");
        int units = jsonExtractInt(req.body, "units", 0);

        if (!BloodTree::isValidGroup(group)) return HttpResponse::error("Invalid blood group", 400);

        if (action == "add") {
            system.adjustStock(group, units);
            system.logActivity("Inventory updated via Web: " + group + " +" + to_string(units));
        } else if (action == "remove") {
            if (!system.adjustStock(group, -units)) return HttpResponse::error("Not enough stock to remove", 400);
            system.logActivity("Inventory updated via Web: " + group + " -" + to_string(units));
        } else {
            system.setStock(group, units);
            system.logActivity("Inventory updated via Web: " + group + " set to " + to_string(units));
        }
        system.saveAll();
        return HttpResponse::json("{\"success\":true,\"message\":\"Inventory updated successfully\"}");
    }

    // ---------------------------------------------------------------- DONORS LIST / SEARCH
    if (path == "/api/donors" && method == "GET") {
        string group = req.getParam("group");
        string hall = req.getParam("hall");
        string search = req.getParam("search");
        bool onlyAvailable = (req.getParam("available") == "true" || req.getParam("available") == "1");

        ostringstream res;
        res << "{\"success\":true,\"donors\":[";
        bool first = true;
        int count = 0;

        for (ListNode<Donor>* p = system.getDonors().getHead(); p; p = p->next) {
            Donor& d = p->data;
            if (!group.empty() && !Utils::iequals(d.getBloodGroup(), group)) continue;
            if (!hall.empty() && !Utils::iequals(d.getHall(), hall)) continue;
            if (onlyAvailable && !d.isAvailable()) continue;
            if (!search.empty() && !Search::containsIgnoreCase(d.getName(), search) &&
                !Search::containsIgnoreCase(d.getPhone(), search)) continue;

            if (!first) res << ",";
            first = false;
            count++;

            res << "{"
                << "\"name\":\"" << jsonEscape(d.getName()) << "\","
                << "\"bloodGroup\":\"" << jsonEscape(d.getBloodGroup()) << "\","
                << "\"hall\":\"" << jsonEscape(d.getHall()) << "\","
                << "\"phone\":\"" << jsonEscape(d.getPhone()) << "\","
                << "\"lastDonationDate\":\"" << jsonEscape(d.getLastDonationDate()) << "\","
                << "\"availableFlag\":" << (d.getAvailableFlag() ? "true" : "false") << ","
                << "\"isEligible\":" << (d.isEligible() ? "true" : "false") << ","
                << "\"isAvailable\":" << (d.isAvailable() ? "true" : "false") << ","
                << "\"waitDays\":" << d.waitDays() << ","
                << "\"status\":\"" << jsonEscape(d.statusText()) << "\""
                << "}";
        }
        res << "],\"count\":" << count << "}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- DONOR TOGGLE AVAILABILITY
    if (path == "/api/donors/toggle" && method == "POST") {
        string phone = jsonExtractString(req.body, "phone");
        Donor* d = Search::findDonorByPhone(system.getDonors(), phone);
        if (!d) return HttpResponse::error("Donor not found", 404);

        d->setAvailable(!d->getAvailableFlag());
        system.logActivity("Donor " + d->getName() + " availability toggled via Web to " + (d->getAvailableFlag() ? "willing" : "unavailable"));
        system.saveAll();

        ostringstream res;
        res << "{\"success\":true,\"availableFlag\":" << (d->getAvailableFlag() ? "true" : "false")
            << ",\"status\":\"" << jsonEscape(d->statusText()) << "\"}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- DONOR RECORD DONATION
    if (path == "/api/donors/record-donation" && method == "POST") {
        string phone = jsonExtractString(req.body, "phone");
        int units = jsonExtractInt(req.body, "units", 1);
        string date = jsonExtractString(req.body, "date", Utils::today());

        Donor* d = Search::findDonorByPhone(system.getDonors(), phone);
        if (!d) return HttpResponse::error("Donor not found", 404);

        string msg;
        if (system.recordDonation(*d, units, date, msg)) {
            return HttpResponse::json("{\"success\":true,\"message\":\"" + jsonEscape(msg) + "\"}");
        } else {
            return HttpResponse::error(msg, 400);
        }
    }

    // ---------------------------------------------------------------- DONOR DELETE
    if (path == "/api/donors/delete" && method == "POST") {
        string phone = jsonExtractString(req.body, "phone");
        Donor* d = Search::findDonorByPhone(system.getDonors(), phone);
        if (!d) return HttpResponse::error("Donor not found", 404);

        string name = d->getName();
        system.getDonors().removeIf([&](Donor& x) { return x.getPhone() == phone; });
        system.logActivity("Donor removed via Web: " + name);
        system.saveAll();
        return HttpResponse::json("{\"success\":true,\"message\":\"Donor removed successfully\"}");
    }

    // ---------------------------------------------------------------- REQUESTS LIST
    if (path == "/api/requests" && method == "GET") {
        string phone = req.getParam("phone");
        string statusFilter = req.getParam("status");

        ostringstream res;
        res << "{\"success\":true,\"requests\":[";
        bool first = true;
        int count = 0;

        for (ListNode<BloodRequest>* p = system.getRequests().getHead(); p; p = p->next) {
            BloodRequest& r = p->data;
            if (!phone.empty() && r.getRequestedBy() != phone) continue;
            if (!statusFilter.empty() && !Utils::iequals(r.getStatus(), statusFilter)) continue;

            if (!first) res << ",";
            first = false;
            count++;

            res << "{"
                << "\"id\":" << r.getId() << ","
                << "\"patientName\":\"" << jsonEscape(r.getPatientName()) << "\","
                << "\"bloodGroup\":\"" << jsonEscape(r.getBloodGroup()) << "\","
                << "\"units\":" << r.getUnits() << ","
                << "\"location\":\"" << jsonEscape(r.getLocation()) << "\","
                << "\"emergency\":" << (r.isEmergency() ? "true" : "false") << ","
                << "\"requestedBy\":\"" << jsonEscape(r.getRequestedBy()) << "\","
                << "\"date\":\"" << jsonEscape(r.getDate()) << "\","
                << "\"status\":\"" << jsonEscape(r.getStatus()) << "\""
                << "}";
        }
        res << "],\"count\":" << count
            << ",\"emergencyQueueSize\":" << system.getEmergencyQueue().size()
            << ",\"normalQueueSize\":" << system.getNormalQueue().size() << "}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- REQUEST CREATE
    if (path == "/api/requests/create" && method == "POST") {
        string patientName = jsonExtractString(req.body, "patientName");
        string bloodGroup = jsonExtractString(req.body, "bloodGroup");
        int units = jsonExtractInt(req.body, "units", 1);
        string location = jsonExtractString(req.body, "location");
        bool emergency = jsonExtractBool(req.body, "emergency", false);
        string requestedBy = jsonExtractString(req.body, "requestedBy");

        int queuePos = 0;
        BloodRequest* r = system.createRequestApi(patientName, bloodGroup, units, location, emergency, requestedBy, queuePos);
        if (r) {
            ostringstream res;
            res << "{\"success\":true,\"requestId\":" << r->getId()
                << ",\"queuePosition\":" << queuePos
                << ",\"emergency\":" << (emergency ? "true" : "false")
                << ",\"stock\":" << system.stockOf(bloodGroup)
                << ",\"message\":\"Request submitted successfully\"}";
            return HttpResponse::json(res.str());
        } else {
            return HttpResponse::error("Failed to submit blood request", 400);
        }
    }

    // ---------------------------------------------------------------- REQUEST FULFILL / REJECT
    if (path == "/api/requests/fulfill" && method == "POST") {
        int id = jsonExtractInt(req.body, "id", 0);
        string err;
        if (system.fulfillRequestApi(id, err)) {
            return HttpResponse::json("{\"success\":true,\"message\":\"Request fulfilled and inventory updated\"}");
        } else {
            return HttpResponse::error(err, 400);
        }
    }

    if (path == "/api/requests/reject" && method == "POST") {
        int id = jsonExtractInt(req.body, "id", 0);
        string err;
        if (system.rejectRequestApi(id, err)) {
            return HttpResponse::json("{\"success\":true,\"message\":\"Request rejected\"}");
        } else {
            return HttpResponse::error(err, 400);
        }
    }

    // ---------------------------------------------------------------- REQUEST PROCESS NEXT IN QUEUE
    if (path == "/api/requests/process-queue" && method == "POST") {
        string msg;
        bool fulfilled = false;
        system.processNextQueueRequestApi(msg, fulfilled);
        ostringstream res;
        res << "{\"success\":" << (fulfilled ? "true" : "false") << ",\"message\":\"" << jsonEscape(msg) << "\"}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- HALLS / GRAPH
    if (path == "/api/halls" && method == "GET") {
        ostringstream res;
        res << "{\"success\":true,\"halls\":[";
        for (int i = 0; i < system.getGraph().nodeCount(); i++) {
            if (i > 0) res << ",";
            res << "{\"id\":" << i << ",\"name\":\"" << jsonEscape(system.getGraph().nameOf(i)) << "\"}";
        }
        res << "]}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- BFS NEAREST DONOR FINDER
    if (path == "/api/nearest-donor" && method == "GET") {
        string startName = req.getParam("start", HallGraph::medicalCenterName());
        string bloodGroup = req.getParam("group");

        if (!BloodTree::isValidGroup(bloodGroup)) return HttpResponse::error("Invalid blood group", 400);

        int startNode = system.getGraph().findNode(startName);
        if (startNode == -1) startNode = system.getGraph().findNode(HallGraph::medicalCenterName());

        auto hasDonor = [&](int node) {
            return system.countMatchingDonors(system.getGraph().nameOf(node), bloodGroup) > 0;
        };

        NearestResult r = system.getGraph().bfsNearest(startNode, hasDonor);

        ostringstream res;
        res << "{\"success\":true,\"found\":" << (r.found ? "true" : "false")
            << ",\"start\":\"" << jsonEscape(system.getGraph().nameOf(startNode)) << "\""
            << ",\"group\":\"" << jsonEscape(bloodGroup) << "\"";

        if (r.found) {
            string foundHall = system.getGraph().nameOf(r.node);
            res << ",\"destinationHall\":\"" << jsonEscape(foundHall) << "\""
                << ",\"hops\":" << r.hops
                << ",\"minutes\":" << r.minutes
                << ",\"meters\":" << r.meters
                << ",\"path\":\"" << jsonEscape(r.path) << "\""
                << ",\"trace\":\"" << jsonEscape(r.trace) << "\""
                << ",\"donors\":[";

            bool firstDonor = true;
            for (ListNode<Donor>* p = system.getDonors().getHead(); p; p = p->next) {
                Donor& d = p->data;
                if (Utils::iequals(d.getHall(), foundHall) && d.isAvailable() &&
                    system.getTree().canDonate(d.getBloodGroup(), bloodGroup)) {
                    if (!firstDonor) res << ",";
                    firstDonor = false;
                    res << "{"
                        << "\"name\":\"" << jsonEscape(d.getName()) << "\","
                        << "\"bloodGroup\":\"" << jsonEscape(d.getBloodGroup()) << "\","
                        << "\"phone\":\"" << jsonEscape(d.getPhone()) << "\","
                        << "\"exactMatch\":" << (d.getBloodGroup() == bloodGroup ? "true" : "false")
                        << "}";
                }
            }
            res << "]";
        } else {
            res << ",\"trace\":\"" << jsonEscape(r.trace) << "\"";
        }
        res << "}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- BLOOD COMPATIBILITY MATRIX
    if (path == "/api/compatibility" && method == "GET") {
        ostringstream res;
        res << "{\"success\":true,\"matrix\":[";
        for (int i = 0; i < BloodTree::TOTAL; i++) {
            const string& g = BloodTree::groupAt(i);
            if (i > 0) res << ",";
            res << "{\"group\":\"" << g << "\",\"canDonateTo\":[";
            bool flags[BloodTree::TOTAL];
            system.getTree().getRecipients(g, flags);
            bool f1 = true;
            for (int k = 0; k < BloodTree::TOTAL; k++) {
                if (flags[k]) {
                    if (!f1) res << ",";
                    f1 = false;
                    res << "\"" << BloodTree::groupAt(k) << "\"";
                }
            }
            res << "],\"canReceiveFrom\":[";
            system.getTree().getDonors(g, flags);
            bool f2 = true;
            for (int k = 0; k < BloodTree::TOTAL; k++) {
                if (flags[k]) {
                    if (!f2) res << ",";
                    f2 = false;
                    res << "\"" << BloodTree::groupAt(k) << "\"";
                }
            }
            res << "]}";
        }
        res << "]}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- MERGESORT REPORTS
    if (path == "/api/reports/donations" && method == "GET") {
        string phone = req.getParam("phone");
        string order = req.getParam("order", "desc");
        int count = 0;
        Donation** sorted = system.getSortedDonations(phone, count);

        ostringstream res;
        res << "{\"success\":true,\"total\":" << count << ",\"donations\":[";
        int totalUnits = 0;
        for (int k = 0; k < count; k++) {
            Donation* d = (order == "asc") ? sorted[k] : sorted[count - 1 - k];
            if (k > 0) res << ",";
            res << "{"
                << "\"id\":" << d->getId() << ","
                << "\"donorName\":\"" << jsonEscape(d->getDonorName()) << "\","
                << "\"donorPhone\":\"" << jsonEscape(d->getDonorPhone()) << "\","
                << "\"bloodGroup\":\"" << jsonEscape(d->getBloodGroup()) << "\","
                << "\"units\":" << d->getUnits() << ","
                << "\"date\":\"" << jsonEscape(d->getDate()) << "\""
                << "}";
            totalUnits += d->getUnits();
        }
        res << "],\"totalUnits\":" << totalUnits << "}";
        if (sorted) delete[] sorted;
        return HttpResponse::json(res.str());
    }

    if (path == "/api/reports/requests" && method == "GET") {
        string order = req.getParam("order", "desc");
        int count = 0;
        BloodRequest** sorted = system.getSortedRequests(count);

        ostringstream res;
        res << "{\"success\":true,\"total\":" << count << ",\"requests\":[";
        int pending = 0, fulfilled = 0, rejected = 0;
        for (int k = 0; k < count; k++) {
            BloodRequest* r = (order == "asc") ? sorted[k] : sorted[count - 1 - k];
            if (k > 0) res << ",";
            res << "{"
                << "\"id\":" << r->getId() << ","
                << "\"patientName\":\"" << jsonEscape(r->getPatientName()) << "\","
                << "\"bloodGroup\":\"" << jsonEscape(r->getBloodGroup()) << "\","
                << "\"units\":" << r->getUnits() << ","
                << "\"location\":\"" << jsonEscape(r->getLocation()) << "\","
                << "\"emergency\":" << (r->isEmergency() ? "true" : "false") << ","
                << "\"requestedBy\":\"" << jsonEscape(r->getRequestedBy()) << "\","
                << "\"date\":\"" << jsonEscape(r->getDate()) << "\","
                << "\"status\":\"" << jsonEscape(r->getStatus()) << "\""
                << "}";
            if (r->getStatus() == "Fulfilled") fulfilled++;
            else if (r->getStatus() == "Rejected") rejected++;
            else pending++;
        }
        res << "],\"pending\":" << pending << ",\"fulfilled\":" << fulfilled << ",\"rejected\":" << rejected << "}";
        if (sorted) delete[] sorted;
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- SUMMARY STATS
    if (path == "/api/reports/summary" && method == "GET") {
        int available = 0, unitsDonated = 0, pending = 0, fulfilled = 0, rejected = 0, emergency = 0;
        system.getDonors().forEach([&](Donor& d) { if (d.isAvailable()) available++; });
        system.getDonations().forEach([&](Donation& d) { unitsDonated += d.getUnits(); });
        system.getRequests().forEach([&](BloodRequest& r) {
            if (r.getStatus() == "Fulfilled") fulfilled++;
            else if (r.getStatus() == "Rejected") rejected++;
            else pending++;
            if (r.isEmergency()) emergency++;
        });

        ostringstream res;
        res << "{\"success\":true,\"stats\":{"
            << "\"totalUsers\":" << system.getUsers().size() << ","
            << "\"totalDonors\":" << system.getDonors().size() << ","
            << "\"availableDonors\":" << available << ","
            << "\"totalDonations\":" << system.getDonations().size() << ","
            << "\"totalUnitsDonated\":" << unitsDonated << ","
            << "\"totalStock\":" << system.totalStock() << ","
            << "\"totalRequests\":" << system.getRequests().size() << ","
            << "\"pendingRequests\":" << pending << ","
            << "\"fulfilledRequests\":" << fulfilled << ","
            << "\"rejectedRequests\":" << rejected << ","
            << "\"emergencyRequests\":" << emergency
            << "}}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- ACTIVITY HISTORY (STACK)
    if (path == "/api/activity" && method == "GET") {
        ostringstream res;
        res << "{\"success\":true,\"activities\":[";
        vector<string> items;
        int limit = 50;
        for (ListNode<string>* p = system.getActivityHistory().getTop(); p && limit-- > 0; p = p->next) {
            items.push_back(p->data);
        }
        for (size_t i = 0; i < items.size(); i++) {
            if (i > 0) res << ",";
            res << "\"" << jsonEscape(items[i]) << "\"";
        }
        res << "]}";
        return HttpResponse::json(res.str());
    }

    // ---------------------------------------------------------------- ADMIN CHANGE PASSWORD
    if (path == "/api/admin/change-password" && method == "POST") {
        string oldPw = jsonExtractString(req.body, "oldPassword");
        string newPw = jsonExtractString(req.body, "newPassword");

        if (system.getAdmin().changePassword(oldPw, newPw)) {
            system.logActivity("Admin password changed via Web");
            system.saveAll();
            return HttpResponse::json("{\"success\":true,\"message\":\"Password updated successfully\"}");
        } else {
            return HttpResponse::error("Invalid current password or new password too short", 400);
        }
    }

    return HttpResponse::error("Endpoint not found", 404);
}
