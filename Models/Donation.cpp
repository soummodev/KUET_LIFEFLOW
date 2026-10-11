#include "Donation.h"
#include "../Core/Utils.h"

Donation::Donation() : id(0), units(0) {}

Donation::Donation(int id, const string& donorName, const string& donorPhone,
                   const string& bloodGroup, int units, const string& date)
    : id(id), donorName(donorName), donorPhone(donorPhone), bloodGroup(bloodGroup),
      units(units), date(date) {}

string Donation::toFileString() const {
    return to_string(id) + "|" + donorName + "|" + donorPhone + "|" + bloodGroup + "|" +
           to_string(units) + "|" + date;
}

bool Donation::fromFileString(const string& line, Donation& out) {
    string p[6];
    if (Utils::split(line, '|', p, 6) < 6) return false;
    out = Donation(Utils::toInt(p[0]), p[1], p[2], p[3], Utils::toInt(p[4]), p[5]);
    return true;
}

string Donation::tableHeader() {
    return Utils::fit("ID", 5) + Utils::fit("Date", 13) + Utils::fit("Donor", 20) +
           Utils::fit("Phone", 13) + Utils::fit("Group", 7) + "Units";
}

string Donation::rowText() const {
    return Utils::fit(to_string(id), 5) + Utils::fit(date, 13) + Utils::fit(donorName, 20) +
           Utils::fit(donorPhone, 13) + Utils::fit(bloodGroup, 7) + to_string(units);
}
