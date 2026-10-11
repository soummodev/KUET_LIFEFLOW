#include "BloodRequest.h"
#include "../Core/Utils.h"

BloodRequest::BloodRequest() : id(0), units(0), emergency(false), status("Pending") {}

BloodRequest::BloodRequest(int id, const string& patientName, const string& bloodGroup,
                           int units, const string& location, bool emergency,
                           const string& requestedBy, const string& date, const string& status)
    : id(id), patientName(patientName), bloodGroup(bloodGroup), units(units),
      location(location), emergency(emergency), requestedBy(requestedBy), date(date),
      status(status) {}

string BloodRequest::toFileString() const {
    return to_string(id) + "|" + patientName + "|" + bloodGroup + "|" + to_string(units) + "|" +
           location + "|" + (emergency ? "1" : "0") + "|" + requestedBy + "|" + date + "|" + status;
}

bool BloodRequest::fromFileString(const string& line, BloodRequest& out) {
    string p[9];
    if (Utils::split(line, '|', p, 9) < 9) return false;
    out = BloodRequest(Utils::toInt(p[0]), p[1], p[2], Utils::toInt(p[3]), p[4], p[5] == "1",
                       p[6], p[7], p[8]);
    return true;
}

void BloodRequest::displayDetails() const {
    Utils::print("Request ID    : " + to_string(id));
    Utils::print("Patient       : " + patientName);
    Utils::print("Blood Group   : " + bloodGroup);
    Utils::print("Units Needed  : " + to_string(units));
    Utils::print("Location      : " + location);
    Utils::print("Emergency     : " + string(emergency ? "YES" : "No"));
    Utils::print("Requested By  : " + requestedBy);
    Utils::print("Date          : " + date);
    Utils::print("Status        : " + status);
}

string BloodRequest::tableHeader() {
    return Utils::fit("ID", 5) + Utils::fit("Patient", 17) + Utils::fit("Group", 7) +
           Utils::fit("Units", 6) + Utils::fit("Location", 23) + Utils::fit("Emerg.", 8) +
           Utils::fit("Date", 12) + "Status";
}

string BloodRequest::rowText() const {
    return Utils::fit(to_string(id), 5) + Utils::fit(patientName, 17) + Utils::fit(bloodGroup, 7) +
           Utils::fit(to_string(units), 6) + Utils::fit(location, 23) +
           Utils::fit(emergency ? "YES" : "No", 8) + Utils::fit(date, 12) + status;
}
