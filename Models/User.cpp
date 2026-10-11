#include "User.h"
#include "../Core/Utils.h"

User::User() : age(0) {}

User::User(const string& name, const string& phone, int age, const string& gender,
           const string& address, const string& hall, const string& bloodGroup,
           const string& passwordHash)
    : name(name), phone(phone), gender(gender), address(address), hall(hall),
      bloodGroup(bloodGroup), passwordHash(passwordHash), age(age) {}

bool User::checkPassword(const string& plain) const {
    return Utils::hashPassword(plain) == passwordHash;
}

string User::toFileString() const {
    return name + "|" + phone + "|" + to_string(age) + "|" + gender + "|" + address + "|" +
           hall + "|" + bloodGroup + "|" + passwordHash;
}

bool User::fromFileString(const string& line, User& out) {
    string p[8];
    if (Utils::split(line, '|', p, 8) < 8) return false;
    out = User(p[0], p[1], Utils::toInt(p[2]), p[3], p[4], p[5], p[6], p[7]);
    return true;
}

void User::displayProfile() const {
    Utils::print("Name          : " + name);
    Utils::print("Blood Group   : " + bloodGroup);
    Utils::print("Hall          : " + hall);
    Utils::print("Phone         : " + phone);
    Utils::print("Age           : " + to_string(age));
    Utils::print("Gender        : " + gender);
    Utils::print("Address       : " + address);
}

string User::tableHeader() {
    return Utils::fit("Name", 20) + Utils::fit("Phone", 13) + Utils::fit("Age", 5) +
           Utils::fit("Group", 7) + Utils::fit("Hall", 24);
}

string User::rowText() const {
    return Utils::fit(name, 20) + Utils::fit(phone, 13) + Utils::fit(to_string(age), 5) +
           Utils::fit(bloodGroup, 7) + Utils::fit(hall, 24);
}
