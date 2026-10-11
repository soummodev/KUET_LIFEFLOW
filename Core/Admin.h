#ifndef ADMIN_H
#define ADMIN_H

#include <bits/stdc++.h>
using namespace std;

class BloodBankSystem;   // forward declaration (the system owns the Admin object)

// Admin: holds admin credentials and runs the Admin Dashboard.
class Admin {
private:
    string adminId;
    string passwordHash;

    // ---- dashboard options
    void viewUsers(BloodBankSystem& sys);
    void manageDonors(BloodBankSystem& sys);
    void updateInventory(BloodBankSystem& sys);
    void viewRequests(BloodBankSystem& sys);
    void reports(BloodBankSystem& sys);
    void viewActivityHistory(BloodBankSystem& sys);
    void changePassword(BloodBankSystem& sys);

    // ---- manage donors
    void viewDonorList(BloodBankSystem& sys);
    void searchDonor(BloodBankSystem& sys);
    void recordDonation(BloodBankSystem& sys);
    void changeAvailability(BloodBankSystem& sys);
    void removeDonor(BloodBankSystem& sys);

    // ---- requests
    void viewAllRequests(BloodBankSystem& sys);
    void searchRequests(BloodBankSystem& sys);

    // ---- reports
    void donationReport(BloodBankSystem& sys);
    void requestReport(BloodBankSystem& sys);
    void summaryReport(BloodBankSystem& sys);
    void mergeSortDemo(BloodBankSystem& sys);

public:
    Admin();

    void setCredentials(const string& id, const string& hash) { adminId = id; passwordHash = hash; }
    const string& getId() const { return adminId; }
    const string& getPasswordHash() const { return passwordHash; }

    bool login(const string& id, const string& plain) const;
    bool changePassword(const string& oldPlain, const string& newPlain);

    void dashboard(BloodBankSystem& sys);   // admin menu loop
};

#endif
