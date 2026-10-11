#include "Search.h"
#include "../Core/Utils.h"

User* Search::findUserByPhone(LinkedList<User>& users, const string& phone) {
    for (ListNode<User>* p = users.getHead(); p; p = p->next)
        if (p->data.getPhone() == phone) return &p->data;
    return nullptr;
}

Donor* Search::findDonorByPhone(LinkedList<Donor>& donors, const string& phone) {
    for (ListNode<Donor>* p = donors.getHead(); p; p = p->next)
        if (p->data.getPhone() == phone) return &p->data;
    return nullptr;
}

BloodRequest* Search::findRequestById(LinkedList<BloodRequest>& requests, int id) {
    for (ListNode<BloodRequest>* p = requests.getHead(); p; p = p->next)
        if (p->data.getId() == id) return &p->data;
    return nullptr;
}

void Search::findDonorsByBloodGroup(LinkedList<Donor>& donors, const string& group,
                                    LinkedList<Donor*>& out, bool onlyAvailable) {
    for (ListNode<Donor>* p = donors.getHead(); p; p = p->next)
        if (p->data.getBloodGroup() == group && (!onlyAvailable || p->data.isAvailable()))
            out.insertBack(&p->data);
}

void Search::findDonorsByHall(LinkedList<Donor>& donors, const string& hall, LinkedList<Donor*>& out) {
    for (ListNode<Donor>* p = donors.getHead(); p; p = p->next)
        if (Utils::iequals(p->data.getHall(), hall)) out.insertBack(&p->data);
}

void Search::findDonorsByName(LinkedList<Donor>& donors, const string& keyword, LinkedList<Donor*>& out) {
    for (ListNode<Donor>* p = donors.getHead(); p; p = p->next)
        if (containsIgnoreCase(p->data.getName(), keyword)) out.insertBack(&p->data);
}

void Search::findRequestsByBloodGroup(LinkedList<BloodRequest>& requests, const string& group,
                                      LinkedList<BloodRequest*>& out) {
    for (ListNode<BloodRequest>* p = requests.getHead(); p; p = p->next)
        if (p->data.getBloodGroup() == group) out.insertBack(&p->data);
}

void Search::findRequestsByPatient(LinkedList<BloodRequest>& requests, const string& keyword,
                                   LinkedList<BloodRequest*>& out) {
    for (ListNode<BloodRequest>* p = requests.getHead(); p; p = p->next)
        if (containsIgnoreCase(p->data.getPatientName(), keyword)) out.insertBack(&p->data);
}

int Search::searchInventory(const map<string, int>& inventory, const string& group) {
    for (map<string, int>::const_iterator it = inventory.begin(); it != inventory.end(); ++it)
        if (it->first == group) return it->second;
    return -1;
}

bool Search::containsIgnoreCase(const string& text, const string& pattern) {
    if (pattern.empty()) return true;
    string t = Utils::toLower(text), s = Utils::toLower(pattern);
    if (s.size() > t.size()) return false;
    for (size_t i = 0; i + s.size() <= t.size(); i++) {
        size_t j = 0;
        while (j < s.size() && t[i + j] == s[j]) j++;
        if (j == s.size()) return true;
    }
    return false;
}
