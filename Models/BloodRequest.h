#ifndef BLOODREQUEST_H
#define BLOODREQUEST_H

#include <bits/stdc++.h>
using namespace std;

class BloodRequest {
private:
    int id;
    string patientName, bloodGroup;
    int units;
    string location;
    bool emergency;
    string requestedBy;   // phone of the user who made the request
    string date;          // DD-MM-YYYY
    string status;        // Pending / Fulfilled / Rejected

public:
    BloodRequest();
    BloodRequest(int id, const string& patientName, const string& bloodGroup, int units,
                 const string& location, bool emergency, const string& requestedBy,
                 const string& date, const string& status = "Pending");

    int getId() const { return id; }
    const string& getPatientName() const { return patientName; }
    const string& getBloodGroup() const { return bloodGroup; }
    int getUnits() const { return units; }
    const string& getLocation() const { return location; }
    bool isEmergency() const { return emergency; }
    const string& getRequestedBy() const { return requestedBy; }
    const string& getDate() const { return date; }
    const string& getStatus() const { return status; }
    void setStatus(const string& s) { status = s; }
    bool isPending() const { return status == "Pending"; }

    // file format: id|patient|group|units|location|emergency(0/1)|requestedBy|date|status
    string toFileString() const;
    static bool fromFileString(const string& line, BloodRequest& out);

    void displayDetails() const;
    string rowText() const;
    static string tableHeader();
};

#endif
