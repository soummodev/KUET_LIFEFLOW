#ifndef USER_H
#define USER_H

#include <bits/stdc++.h>
using namespace std;

class User {
private:
    string name, phone, gender, address, hall, bloodGroup, passwordHash;
    int age;

public:
    User();
    User(const string& name, const string& phone, int age, const string& gender,
         const string& address, const string& hall, const string& bloodGroup,
         const string& passwordHash);

    const string& getName() const { return name; }
    const string& getPhone() const { return phone; }
    int getAge() const { return age; }
    const string& getGender() const { return gender; }
    const string& getAddress() const { return address; }
    const string& getHall() const { return hall; }
    const string& getBloodGroup() const { return bloodGroup; }

    bool checkPassword(const string& plain) const;

    // file format: name|phone|age|gender|address|hall|bloodGroup|passwordHash
    string toFileString() const;
    static bool fromFileString(const string& line, User& out);

    void displayProfile() const;
    string rowText() const;
    static string tableHeader();
};

#endif
