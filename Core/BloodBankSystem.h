#ifndef BLOODBANKSYSTEM_H
#define BLOODBANKSYSTEM_H

#include <bits/stdc++.h>
using namespace std;

#include "Utils.h"
#include "Admin.h"
#include "../DataStructures/LinkedList.h"
#include "../DataStructures/Stack.h"
#include "../DataStructures/Queue.h"
#include "../DataStructures/BloodTree.h"
#include "../DataStructures/HallGraph.h"
#include "../Models/User.h"
#include "../Models/Donor.h"
#include "../Models/Donation.h"
#include "../Models/BloodRequest.h"
#include "../Algorithms/MergeSort.h"
#include "../Algorithms/Search.h"
#include "../Database/FileManager.h"

// Central controller: owns every data structure and runs the User panel.
// The Admin panel lives in Admin.cpp and works through the public API below.
class BloodBankSystem {
private:
    // ---- data structures
    LinkedList<User> users;                 // registered users
    LinkedList<Donor> donors;               // donors
    LinkedList<Donation> donations;         // donation records
    LinkedList<BloodRequest> requests;      // all blood requests
    Queue<BloodRequest*> emergencyQueue;    // pending emergency requests (FIFO)
    Queue<BloodRequest*> normalQueue;       // pending normal requests (FIFO)
    Stack<string> activityHistory;          // latest activity on top
    map<string, int> inventory;             // blood group -> units
    BloodTree bloodTree;                    // compatibility tree
    HallGraph hallGraph;                    // KUET hall network

    Admin admin;
    FileManager fileManager;
    User* currentUser;
    int nextRequestId;
    int nextDonationId;

    void seedDefaultHalls();

    // ---- login menu
    void registerUser();
    void loginUser();
    void adminLogin();

    // ---- user dashboard
    void userDashboard();
    void viewProfile();
    void becomeDonor();
    void searchBlood();
    void requestBlood();
    void findNearestDonor();
    void compatibilityCheck();
    void donationHistory();

    // ---- helpers
    void showCompatibility(const string& group);

public:
    BloodBankSystem();

    void loadData();                  // loads all files and initializes data structures
    void run();                       // whole application loop (after "Start System")
    static void showAbout();

    // ---- accessors used by Admin & Server
    LinkedList<User>& getUsers() { return users; }
    LinkedList<Donor>& getDonors() { return donors; }
    LinkedList<Donation>& getDonations() { return donations; }
    LinkedList<BloodRequest>& getRequests() { return requests; }
    Queue<BloodRequest*>& getEmergencyQueue() { return emergencyQueue; }
    Queue<BloodRequest*>& getNormalQueue() { return normalQueue; }
    map<string, int>& getInventory() { return inventory; }
    BloodTree& getTree() { return bloodTree; }
    HallGraph& getGraph() { return hallGraph; }
    Stack<string>& getActivityHistory() { return activityHistory; }
    Admin& getAdmin() { return admin; }

    // ---- shared operations
    void logActivity(const string& message);
    void saveAll();
    static string askBloodGroup(const string& label = "Blood Group");
    string chooseLocation(bool includeMedicalCenter);
    void displayInventory();
    int totalStock();
    bool adjustStock(const string& group, int delta);   // false if it would go below zero
    void setStock(const string& group, int units);
    int countMatchingDonors(const string& hall, const string& group);
    int stockOf(const string& group);
    static bool isValidPhone(const string& phone);

    bool recordDonation(Donor& donor, int units, const string& date, string& message);
    Donation** getSortedDonations(const string& phoneFilter, int& count);   // caller delete[]
    BloodRequest** getSortedRequests(int& count);                            // caller delete[]

    void displayPendingQueues();
    void processNextRequest();
    bool planIssue(const string& group, int units, int* plan);

    // ---- programmatic / API operations
    bool registerUserApi(const string& name, const string& phone, int age, const string& gender,
                         const string& address, const string& hall, const string& bloodGroup,
                         const string& password, string& errorMsg);
    User* authenticateUser(const string& phone, const string& password);
    bool becomeDonorApi(const string& phone, const string& lastDonationDate, string& errorMsg);
    BloodRequest* createRequestApi(const string& patientName, const string& bloodGroup, int units,
                                  const string& location, bool emergency, const string& requestedBy, int& queuePos);
    bool fulfillRequestApi(int requestId, string& errorMsg);
    bool rejectRequestApi(int requestId, string& errorMsg);
    bool processNextQueueRequestApi(string& outMessage, bool& outFulfilled);
};

#endif
