#include "FileManager.h"
#include "../Core/Utils.h"

using namespace std;
FileManager::FileManager() {
    dataDir = "Data/";
}

bool FileManager::splitTag(const string& line, string& tag, string& rest) {
    size_t pos = line.find('|');
    if (pos == string::npos) return false;
    tag = line.substr(0, pos);
    rest = line.substr(pos + 1);
    return true;
}

bool FileManager::openForWrite(ofstream& out, const string& fullPath) {
    out.open(fullPath.c_str());
    if (!out) {
        cout << "  [Warning] Could not write " << fullPath << "\n";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------- users.txt
void FileManager::saveUsers(LinkedList<User>& users, const string& adminId, const string& adminHash) {
    ofstream out;
    if (!openForWrite(out, path("users.txt"))) return;
    out << "ADMIN|" << adminId << "|" << adminHash << "\n";
    for (ListNode<User>* p = users.getHead(); p; p = p->next)
        out << "USER|" << p->data.toFileString() << "\n";
}

void FileManager::loadUsers(LinkedList<User>& users, string& adminId, string& adminHash) {
    ifstream in(path("users.txt"));
    if (!in) return;
    string line, tag, rest;
    while (getline(in, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#' || !splitTag(line, tag, rest)) continue;
        if (tag == "ADMIN") {
            string p[2];
            if (Utils::split(rest, '|', p, 2) == 2) { adminId = p[0]; adminHash = p[1]; }
        } else if (tag == "USER") {
            User u;
            if (User::fromFileString(rest, u)) users.insertBack(u);
        }
    }
}

// ---------------------------------------------------------------- donors.txt
void FileManager::saveDonors(LinkedList<Donor>& donors, LinkedList<Donation>& donations) {
    ofstream out;
    if (!openForWrite(out, path("donors.txt"))) return;
    for (ListNode<Donor>* p = donors.getHead(); p; p = p->next)
        out << "DONOR|" << p->data.toFileString() << "\n";
    for (ListNode<Donation>* p = donations.getHead(); p; p = p->next)
        out << "DONATION|" << p->data.toFileString() << "\n";
}

void FileManager::loadDonors(LinkedList<Donor>& donors, LinkedList<Donation>& donations) {
    ifstream in(path("donors.txt"));
    if (!in) return;
    string line, tag, rest;
    while (getline(in, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#' || !splitTag(line, tag, rest)) continue;
        if (tag == "DONOR") {
            Donor d;
            if (Donor::fromFileString(rest, d)) donors.insertBack(d);
        } else if (tag == "DONATION") {
            Donation d;
            if (Donation::fromFileString(rest, d)) donations.insertBack(d);
        }
    }
}

// ---------------------------------------------------------------- requests.txt
void FileManager::saveRequests(LinkedList<BloodRequest>& requests) {
    ofstream out;
    if (!openForWrite(out, path("requests.txt"))) return;
    for (ListNode<BloodRequest>* p = requests.getHead(); p; p = p->next)
        out << "REQ|" << p->data.toFileString() << "\n";
}

void FileManager::loadRequests(LinkedList<BloodRequest>& requests) {
    ifstream in(path("requests.txt"));
    if (!in) return;
    string line, tag, rest;
    while (getline(in, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#' || !splitTag(line, tag, rest)) continue;
        if (tag == "REQ") {
            BloodRequest r;
            if (BloodRequest::fromFileString(rest, r)) requests.insertBack(r);
        }
    }
}

// ---------------------------------------------------------------- inventory.txt
void FileManager::saveInventory(const map<string, int>& inventory) {
    ofstream out;
    if (!openForWrite(out, path("inventory.txt"))) return;
    for (map<string, int>::const_iterator it = inventory.begin(); it != inventory.end(); ++it)
        out << it->first << "|" << it->second << "\n";
}

void FileManager::loadInventory(map<string, int>& inventory) {
    ifstream in(path("inventory.txt"));
    if (!in) return;
    string line;
    while (getline(in, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#') continue;
        string p[2];
        if (Utils::split(line, '|', p, 2) == 2) inventory[p[0]] = max(0, Utils::toInt(p[1]));
    }
}

// ---------------------------------------------------------------- halls.txt
void FileManager::saveHalls(HallGraph& graph) {
    ofstream out;
    if (!openForWrite(out, path("halls.txt"))) return;
    for (int i = 0; i < graph.nodeCount(); i++) out << "NODE|" << graph.nameOf(i) << "\n";
    for (int i = 0; i < graph.nodeCount(); i++)
        for (ListNode<GraphEdge>* p = graph.neighbors(i).getHead(); p; p = p->next)
            if (i < p->data.to)   // undirected: write each edge once
                out << "EDGE|" << graph.nameOf(i) << "|" << graph.nameOf(p->data.to) << "|"
                    << p->data.meters << "|" << p->data.minutes << "\n";
}

bool FileManager::loadHalls(HallGraph& graph) {
    ifstream in(path("halls.txt"));
    if (!in) return false;
    string line, tag, rest;
    while (getline(in, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#' || !splitTag(line, tag, rest)) continue;
        if (tag == "NODE") {
            graph.addNode(rest);
        } else if (tag == "EDGE") {
            string p[4];
            if (Utils::split(rest, '|', p, 4) == 4)
                graph.addEdge(graph.findNode(p[0]), graph.findNode(p[1]),
                              Utils::toInt(p[2]), Utils::toInt(p[3]));
        }
    }
    return graph.nodeCount() > 0;
}
