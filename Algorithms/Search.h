#ifndef SEARCH_H
#define SEARCH_H

#include "../DataStructures/LinkedList.h"
#include "../Models/User.h"
#include "../Models/Donor.h"
#include "../Models/BloodRequest.h"

// Manual searching (linear search written by hand, no std::find).
// Used for: donor search, blood (inventory) search, request search, login lookup.
class Search {
public:
    // ---- single record lookups (return nullptr when not found)
    static User* findUserByPhone(LinkedList<User>& users, const string& phone);
    static Donor* findDonorByPhone(LinkedList<Donor>& donors, const string& phone);
    static BloodRequest* findRequestById(LinkedList<BloodRequest>& requests, int id);

    // ---- multi-result searches (results are appended to 'out')
    static void findDonorsByBloodGroup(LinkedList<Donor>& donors, const string& group,
                                       LinkedList<Donor*>& out, bool onlyAvailable);
    static void findDonorsByHall(LinkedList<Donor>& donors, const string& hall, LinkedList<Donor*>& out);
    static void findDonorsByName(LinkedList<Donor>& donors, const string& keyword, LinkedList<Donor*>& out);
    static void findRequestsByBloodGroup(LinkedList<BloodRequest>& requests, const string& group,
                                         LinkedList<BloodRequest*>& out);
    static void findRequestsByPatient(LinkedList<BloodRequest>& requests, const string& keyword,
                                      LinkedList<BloodRequest*>& out);

    // ---- blood search in the inventory map (-1 if the group is not present)
    static int searchInventory(const map<string, int>& inventory, const string& group);

    // ---- manual substring test, case-insensitive
    static bool containsIgnoreCase(const string& text, const string& pattern);
};

#endif
