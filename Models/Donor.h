#ifndef DONOR_H
#define DONOR_H

#include <bits/stdc++.h>
using namespace std;

class Donor {
private:
    string name, bloodGroup, hall, phone, lastDonationDate;   // date = DD-MM-YYYY or "N/A"
    bool available;                                            // donor's own willingness switch

public:
    static constexpr int GAP_DAYS = 90;   // minimum days between two donations

    Donor();
    Donor(const string& name, const string& bloodGroup, const string& hall,
          const string& phone, const string& lastDonationDate, bool available);

    const string& getName() const { return name; }
    const string& getBloodGroup() const { return bloodGroup; }
    const string& getHall() const { return hall; }
    const string& getPhone() const { return phone; }
    const string& getLastDonationDate() const { return lastDonationDate; }
    bool getAvailableFlag() const { return available; }

    void setAvailable(bool a) { available = a; }
    void setLastDonationDate(const string& d) { lastDonationDate = d; }

    bool isEligibleOn(const string& date) const;   // 90-day rule
    bool isEligible() const;                        // eligible today
    int waitDays() const;                           // days left until eligible (0 if eligible)
    bool isAvailable() const;                       // willing AND eligible
    string statusText() const;

    // file format: name|bloodGroup|hall|phone|lastDonationDate|available(0/1)
    string toFileString() const;
    static bool fromFileString(const string& line, Donor& out);

    string rowText() const;
    static string tableHeader();
};

#endif
