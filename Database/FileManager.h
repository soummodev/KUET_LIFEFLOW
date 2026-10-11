#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "../DataStructures/LinkedList.h"
#include "../DataStructures/HallGraph.h"
#include "../Models/User.h"
#include "../Models/Donor.h"
#include "../Models/Donation.h"
#include "../Models/BloodRequest.h"

// File handling: save / load everything so data stays after the program exits.
//
//   users.txt      ADMIN|id|passwordHash          USER|name|phone|age|gender|address|hall|group|hash
//   donors.txt     DONOR|name|group|hall|phone|lastDate|available     DONATION|id|name|phone|group|units|date
//   requests.txt   REQ|id|patient|group|units|location|emergency|requestedBy|date|status
//   inventory.txt  A+|10
//   halls.txt      NODE|name          EDGE|nameA|nameB|meters|minutes
//
// Lines that are empty or start with '#' are ignored when loading.
class FileManager {
private:
    string dataDir;

    string path(const string& file) const { return dataDir + file; }
    static bool splitTag(const string& line, string& tag, string& rest);
    static bool openForWrite(ofstream& out, const string& fullPath);

public:
    FileManager();

    void saveUsers(LinkedList<User>& users, const string& adminId, const string& adminHash);
    void loadUsers(LinkedList<User>& users, string& adminId, string& adminHash);

    void saveDonors(LinkedList<Donor>& donors, LinkedList<Donation>& donations);
    void loadDonors(LinkedList<Donor>& donors, LinkedList<Donation>& donations);

    void saveRequests(LinkedList<BloodRequest>& requests);
    void loadRequests(LinkedList<BloodRequest>& requests);

    void saveInventory(const map<string, int>& inventory);
    void loadInventory(map<string, int>& inventory);

    void saveHalls(HallGraph& graph);
    bool loadHalls(HallGraph& graph);   // false if no hall data was found
};

#endif
