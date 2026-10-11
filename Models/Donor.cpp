#include "Donor.h"
#include "../Core/Utils.h"

Donor::Donor() : lastDonationDate("N/A"), available(true) {}

Donor::Donor(const string& name, const string& bloodGroup, const string& hall,
             const string& phone, const string& lastDonationDate, bool available)
    : name(name), bloodGroup(bloodGroup), hall(hall), phone(phone),
      lastDonationDate(lastDonationDate), available(available) {}

bool Donor::isEligibleOn(const string& date) const {
    if (lastDonationDate == "N/A") return true;
    int days;
    if (!Utils::daysBetween(lastDonationDate, date, days)) return true;
    return days >= GAP_DAYS;
}

bool Donor::isEligible() const { return isEligibleOn(Utils::today()); }

int Donor::waitDays() const {
    if (lastDonationDate == "N/A") return 0;
    int days;
    if (!Utils::daysBetween(lastDonationDate, Utils::today(), days)) return 0;
    return days >= GAP_DAYS ? 0 : GAP_DAYS - days;
}

bool Donor::isAvailable() const { return available && isEligible(); }

string Donor::statusText() const {
    if (!available) return "Unavailable";
    if (!isEligible()) return "Wait " + to_string(waitDays()) + " days";
    return "Available";
}

string Donor::toFileString() const {
    return name + "|" + bloodGroup + "|" + hall + "|" + phone + "|" + lastDonationDate + "|" +
           (available ? "1" : "0");
}

bool Donor::fromFileString(const string& line, Donor& out) {
    string p[6];
    if (Utils::split(line, '|', p, 6) < 6) return false;
    out = Donor(p[0], p[1], p[2], p[3], p[4], p[5] == "1");
    return true;
}

string Donor::tableHeader() {
    return Utils::fit("Name", 20) + Utils::fit("Group", 7) + Utils::fit("Hall", 24) +
           Utils::fit("Phone", 13) + Utils::fit("Last Donation", 15) + "Status";
}

string Donor::rowText() const {
    return Utils::fit(name, 20) + Utils::fit(bloodGroup, 7) + Utils::fit(hall, 24) +
           Utils::fit(phone, 13) + Utils::fit(lastDonationDate, 15) + statusText();
}
