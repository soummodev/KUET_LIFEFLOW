#ifndef DONATION_H
#define DONATION_H

#include <bits/stdc++.h>
using namespace std;

// One completed blood donation (used for donation history and reports).
class Donation {
private:
    int id;
    string donorName, donorPhone, bloodGroup;
    int units;
    string date;   // DD-MM-YYYY

public:
    Donation();
    Donation(int id, const string& donorName, const string& donorPhone,
             const string& bloodGroup, int units, const string& date);

    int getId() const { return id; }
    const string& getDonorName() const { return donorName; }
    const string& getDonorPhone() const { return donorPhone; }
    const string& getBloodGroup() const { return bloodGroup; }
    int getUnits() const { return units; }
    const string& getDate() const { return date; }

    // file format: id|donorName|donorPhone|bloodGroup|units|date
    string toFileString() const;
    static bool fromFileString(const string& line, Donation& out);

    string rowText() const;
    static string tableHeader();
};

#endif
