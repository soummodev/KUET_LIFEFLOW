#include <bits/stdc++.h>
using namespace std;

#include "Core/Utils.h"
#include "Core/BloodBankSystem.h"
#include "Server/HttpServer.h"

void openBrowser(const string& url) {
#ifdef _WIN32
    system(("start " + url).c_str());
#elif __APPLE__
    system(("open " + url).c_str());
#else
    system(("xdg-open " + url).c_str());
#endif
}

void startWebServer(BloodBankSystem& app, int port = 8080) {
    app.loadData();
    HttpServer server(port, "web", app);

    Utils::heading("KUET LIFEFLOW - WEB SERVER");
    Utils::print("Web Server is starting on port " + to_string(port) + "...");
    Utils::print("Frontend UI: http://localhost:" + to_string(port));
    Utils::blank();
    Utils::print("Launching browser at http://localhost:" + to_string(port) + "...");
    Utils::blank();

    openBrowser("http://localhost:" + to_string(port));

    Utils::print("Server active! Press Ctrl+C in this terminal to stop.");
    Utils::blank();

    server.start();
}

int main(int argc, char* argv[]) {
    BloodBankSystem app;

    // Direct web mode flag
    if (argc > 1) {
        string arg = argv[1];
        if (arg == "--web" || arg == "-w" || arg == "--server" || arg == "-s") {
            int port = 8080;
            if (argc > 2) port = Utils::toInt(argv[2], 8080);
            startWebServer(app, port);
            return 0;
        }
    }

    while (true) {
        Utils::heading("KUET LIFEFLOW - SMART BLOOD BANK SYSTEM");
        Utils::print("Welcome! Save a life - donate blood.");
        Utils::blank();
        Utils::print("1. Launch Web Frontend (Browser UI) [Recommended]");
        Utils::print("2. Start Terminal Console Mode");
        Utils::print("3. About KUET LifeFlow");
        Utils::print("4. Exit");
        Utils::blank();

        int choice = Utils::askInt("Enter choice", 1, 4);
        if (choice == 1) {
            startWebServer(app, 8080);
            break;
        } else if (choice == 2) {
            app.run();
        } else if (choice == 3) {
            BloodBankSystem::showAbout();
        } else {
            Utils::blank();
            Utils::print("Thank you for using KUET LifeFlow.");
            Utils::blank();
            break;
        }
    }
    return 0;
}
