#include "Admin.h"
#include "BloodBankSystem.h"

Admin::Admin() : adminId("admin"), passwordHash(Utils::hashPassword("admin123")) {}

bool Admin::login(const string& id, const string& plain) const {
    return id == adminId && Utils::hashPassword(plain) == passwordHash;
}

bool Admin::changePassword(const string& oldPlain, const string& newPlain) {
    if (Utils::hashPassword(oldPlain) != passwordHash || newPlain.size() < 4) return false;
    passwordHash = Utils::hashPassword(newPlain);
    return true;
}

// =====================================================================================
void Admin::dashboard(BloodBankSystem& sys) {
    while (true) {
        Utils::heading("ADMIN DASHBOARD");
        Utils::print("1. View Users");
        Utils::print("2. Manage Donors");
        Utils::print("3. Update Blood Inventory");
        Utils::print("4. View Blood Requests");
        Utils::print("5. Generate Reports");
        Utils::print("6. View Activity History");
        Utils::print("7. Change Admin Password");
        Utils::print("8. Logout");
        Utils::blank();
        int c = Utils::askInt("Enter choice", 1, 8);
        switch (c) {
            case 1: viewUsers(sys); break;
            case 2: manageDonors(sys); break;
            case 3: updateInventory(sys); break;
            case 4: viewRequests(sys); break;
            case 5: reports(sys); break;
            case 6: viewActivityHistory(sys); break;
            case 7: changePassword(sys); break;
            case 8: sys.logActivity("Admin logged out"); return;
        }
    }
}

// ------------------------------------------------------------------ 1. users
void Admin::viewUsers(BloodBankSystem& sys) {
    Utils::heading("REGISTERED USERS");
    if (sys.getUsers().isEmpty()) Utils::print("No users registered yet.");
    else {
        Utils::row(User::tableHeader());
        sys.getUsers().forEach([](User& u) { Utils::row(u.rowText()); });
        Utils::blank();
        Utils::print("Total users: " + to_string(sys.getUsers().size()));
    }
    Utils::pause();
}

// ------------------------------------------------------------------ 2. donors
void Admin::manageDonors(BloodBankSystem& sys) {
    while (true) {
        Utils::heading("MANAGE DONORS");
        Utils::print("1. View Donor List");
        Utils::print("2. Search Donor");
        Utils::print("3. Record Donation");
        Utils::print("4. Change Donor Availability");
        Utils::print("5. Remove Donor");
        Utils::print("6. Back");
        Utils::blank();
        int c = Utils::askInt("Enter choice", 1, 6);
        switch (c) {
            case 1: viewDonorList(sys); break;
            case 2: searchDonor(sys); break;
            case 3: recordDonation(sys); break;
            case 4: changeAvailability(sys); break;
            case 5: removeDonor(sys); break;
            case 6: return;
        }
    }
}

void Admin::viewDonorList(BloodBankSystem& sys) {
    Utils::heading("DONOR LIST");
    if (sys.getDonors().isEmpty()) Utils::print("No donors registered yet.");
    else {
        Utils::row(Donor::tableHeader());
        int available = 0;
        sys.getDonors().forEach([&](Donor& d) { Utils::row(d.rowText()); if (d.isAvailable()) available++; });
        Utils::blank();
        Utils::print("Total donors: " + to_string(sys.getDonors().size()) + "   Available now: " + to_string(available));
    }
    Utils::pause();
}

void Admin::searchDonor(BloodBankSystem& sys) {
    Utils::heading("SEARCH DONOR");
    Utils::print("1. By blood group");
    Utils::print("2. By hall");
    Utils::print("3. By name");
    Utils::print("4. By phone");
    Utils::blank();
    int c = Utils::askInt("Enter choice", 1, 4);
    LinkedList<Donor*> found;
    if (c == 1) {
        Search::findDonorsByBloodGroup(sys.getDonors(), BloodBankSystem::askBloodGroup(), found, false);
    } else if (c == 2) {
        Utils::print("Select hall:");
        Search::findDonorsByHall(sys.getDonors(), sys.chooseLocation(false), found);
    } else if (c == 3) {
        Search::findDonorsByName(sys.getDonors(), Utils::askNonEmpty("Name (or part of it)"), found);
    } else {
        Donor* d = Search::findDonorByPhone(sys.getDonors(), Utils::askNonEmpty("Phone"));
        if (d) found.insertBack(d);
    }
    Utils::blank();
    if (found.isEmpty()) Utils::print("No matching donor found.");
    else {
        Utils::row(Donor::tableHeader());
        for (ListNode<Donor*>* p = found.getHead(); p; p = p->next) Utils::row(p->data->rowText());
        Utils::blank();
        Utils::print("Found: " + to_string(found.size()));
    }
    Utils::pause();
}

void Admin::recordDonation(BloodBankSystem& sys) {
    Utils::heading("RECORD DONATION");
    Donor* d = Search::findDonorByPhone(sys.getDonors(), Utils::askNonEmpty("Donor phone"));
    if (!d) { Utils::print("Donor not found."); Utils::pause(); return; }
    Utils::print("Donor: " + d->getName() + "  (" + d->getBloodGroup() + ")   Status: " + d->statusText());
    Utils::blank();

    int units = Utils::askInt("Units donated (1-2)", 1, 2);
    string date;
    while (true) {
        date = Utils::ask("Donation date (DD-MM-YYYY, Enter = today)");
        if (date.empty()) date = Utils::today();
        if (!Utils::isValidDate(date)) { Utils::print("Invalid date format."); continue; }
        if (Utils::dateKey(date) > Utils::dateKey(Utils::today())) { Utils::print("Date cannot be in the future."); continue; }
        break;
    }
    string msg;
    sys.recordDonation(*d, units, date, msg);
    Utils::blank();
    Utils::print(msg);
    Utils::pause();
}

void Admin::changeAvailability(BloodBankSystem& sys) {
    Utils::heading("CHANGE DONOR AVAILABILITY");
    Donor* d = Search::findDonorByPhone(sys.getDonors(), Utils::askNonEmpty("Donor phone"));
    if (!d) { Utils::print("Donor not found."); Utils::pause(); return; }
    d->setAvailable(!d->getAvailableFlag());
    sys.logActivity("Donor " + d->getName() + " set to " + (d->getAvailableFlag() ? "willing" : "unavailable"));
    sys.saveAll();
    Utils::print(d->getName() + " is now: " + d->statusText());
    Utils::pause();
}

void Admin::removeDonor(BloodBankSystem& sys) {
    Utils::heading("REMOVE DONOR");
    string phone = Utils::askNonEmpty("Donor phone");
    Donor* d = Search::findDonorByPhone(sys.getDonors(), phone);
    if (!d) { Utils::print("Donor not found."); Utils::pause(); return; }
    string name = d->getName();
    if (Utils::askYesNo("Remove donor " + name + "?")) {
        sys.getDonors().removeIf([&](Donor& x) { return x.getPhone() == phone; });
        sys.logActivity("Donor removed: " + name);
        sys.saveAll();
        Utils::print("Donor removed.");
    }
    Utils::pause();
}

// ------------------------------------------------------------------ 3. inventory
void Admin::updateInventory(BloodBankSystem& sys) {
    while (true) {
        Utils::heading("UPDATE BLOOD INVENTORY");
        sys.displayInventory();
        Utils::blank();
        Utils::print("1. Add units");
        Utils::print("2. Remove units");
        Utils::print("3. Set exact quantity");
        Utils::print("4. Back");
        Utils::blank();
        int c = Utils::askInt("Enter choice", 1, 4);
        if (c == 4) return;
        string g = BloodBankSystem::askBloodGroup();
        if (c == 1) {
            int u = Utils::askInt("Units to add (1-100)", 1, 100);
            sys.adjustStock(g, u);
            sys.logActivity("Inventory updated: " + g + " +" + to_string(u));
        } else if (c == 2) {
            int u = Utils::askInt("Units to remove (1-100)", 1, 100);
            if (!sys.adjustStock(g, -u)) { Utils::print("Not enough stock to remove that many units."); Utils::pause(); continue; }
            sys.logActivity("Inventory updated: " + g + " -" + to_string(u));
        } else {
            int u = Utils::askInt("New quantity (0-500)", 0, 500);
            sys.setStock(g, u);
            sys.logActivity("Inventory updated: " + g + " set to " + to_string(u));
        }
        sys.saveAll();
        Utils::print("Inventory updated.");
        Utils::pause();
    }
}

// ------------------------------------------------------------------ 4. requests
void Admin::viewRequests(BloodBankSystem& sys) {
    while (true) {
        Utils::heading("BLOOD REQUESTS");
        Utils::print("1. View All Requests");
        Utils::print("2. View Pending Queue");
        Utils::print("3. Process Next Request");
        Utils::print("4. Search Requests");
        Utils::print("5. Back");
        Utils::blank();
        int c = Utils::askInt("Enter choice", 1, 5);
        if (c == 5) return;
        Utils::blank();
        if (c == 1) viewAllRequests(sys);
        else if (c == 2) { sys.displayPendingQueues(); Utils::pause(); }
        else if (c == 3) sys.processNextRequest();
        else searchRequests(sys);
    }
}

void Admin::viewAllRequests(BloodBankSystem& sys) {
    if (sys.getRequests().isEmpty()) Utils::print("No blood requests yet.");
    else {
        Utils::row(BloodRequest::tableHeader());
        sys.getRequests().forEach([](BloodRequest& r) { Utils::row(r.rowText()); });
        Utils::blank();
        Utils::print("Total requests: " + to_string(sys.getRequests().size()));
    }
    Utils::pause();
}

void Admin::searchRequests(BloodBankSystem& sys) {
    Utils::print("1. By request ID");
    Utils::print("2. By blood group");
    Utils::print("3. By patient name");
    Utils::blank();
    int c = Utils::askInt("Enter choice", 1, 3);
    LinkedList<BloodRequest*> found;
    if (c == 1) {
        BloodRequest* r = Search::findRequestById(sys.getRequests(), Utils::askInt("Request ID", 1, 999999));
        if (r) found.insertBack(r);
    } else if (c == 2) {
        Search::findRequestsByBloodGroup(sys.getRequests(), BloodBankSystem::askBloodGroup(), found);
    } else {
        Search::findRequestsByPatient(sys.getRequests(), Utils::askNonEmpty("Patient name (or part of it)"), found);
    }
    Utils::blank();
    if (found.isEmpty()) Utils::print("No matching request found.");
    else {
        Utils::row(BloodRequest::tableHeader());
        for (ListNode<BloodRequest*>* p = found.getHead(); p; p = p->next) Utils::row(p->data->rowText());
    }
    Utils::pause();
}

// ------------------------------------------------------------------ 5. reports
void Admin::reports(BloodBankSystem& sys) {
    while (true) {
        Utils::heading("REPORTS");
        Utils::print("1. Donation Report (sorted by date)");
        Utils::print("2. Request Report (sorted by date)");
        Utils::print("3. Inventory Report");
        Utils::print("4. Summary Statistics");
        Utils::print("5. Hall Network Map");
        Utils::print("6. Merge Sort Demo");
        Utils::print("7. Back");
        Utils::blank();
        int c = Utils::askInt("Enter choice", 1, 7);
        switch (c) {
            case 1: donationReport(sys); break;
            case 2: requestReport(sys); break;
            case 3:
                Utils::heading("INVENTORY REPORT");
                sys.displayInventory();
                Utils::pause();
                break;
            case 4: summaryReport(sys); break;
            case 5:
                Utils::heading("KUET HALL NETWORK");
                sys.getGraph().display(Utils::pad());
                Utils::pause();
                break;
            case 6: mergeSortDemo(sys); break;
            case 7: return;
        }
    }
}

void Admin::donationReport(BloodBankSystem& sys) {
    Utils::heading("DONATION REPORT");
    int n = 0;
    Donation** sorted = sys.getSortedDonations("", n);   // Merge Sort by date
    if (n == 0) { Utils::print("No donations recorded yet."); Utils::pause(); return; }

    Utils::print("1. Oldest first   2. Latest first");
    int order = Utils::askInt("Select", 1, 2);
    Utils::blank();
    Utils::row(Donation::tableHeader());
    int total = 0;
    for (int k = 0; k < n; k++) {
        Donation* d = (order == 1) ? sorted[k] : sorted[n - 1 - k];
        Utils::row(d->rowText());
        total += d->getUnits();
    }
    Utils::blank();
    Utils::print("Total donations: " + to_string(n) + "   Total units: " + to_string(total));
    delete[] sorted;
    Utils::pause();
}

void Admin::requestReport(BloodBankSystem& sys) {
    Utils::heading("REQUEST REPORT");
    int n = 0;
    BloodRequest** sorted = sys.getSortedRequests(n);   // Merge Sort by date
    if (n == 0) { Utils::print("No requests recorded yet."); Utils::pause(); return; }

    Utils::print("1. Oldest first   2. Latest first");
    int order = Utils::askInt("Select", 1, 2);
    Utils::blank();
    Utils::row(BloodRequest::tableHeader());
    int pending = 0, fulfilled = 0, rejected = 0;
    for (int k = 0; k < n; k++) {
        BloodRequest* r = (order == 1) ? sorted[k] : sorted[n - 1 - k];
        Utils::row(r->rowText());
        if (r->getStatus() == "Fulfilled") fulfilled++;
        else if (r->getStatus() == "Rejected") rejected++;
        else pending++;
    }
    Utils::blank();
    Utils::print("Total: " + to_string(n) + "   Pending: " + to_string(pending) +
                 "   Fulfilled: " + to_string(fulfilled) + "   Rejected: " + to_string(rejected));
    delete[] sorted;
    Utils::pause();
}

void Admin::summaryReport(BloodBankSystem& sys) {
    Utils::heading("SUMMARY STATISTICS");
    int available = 0, unitsDonated = 0, pending = 0, fulfilled = 0, rejected = 0, emergency = 0;
    sys.getDonors().forEach([&](Donor& d) { if (d.isAvailable()) available++; });
    sys.getDonations().forEach([&](Donation& d) { unitsDonated += d.getUnits(); });
    sys.getRequests().forEach([&](BloodRequest& r) {
        if (r.getStatus() == "Fulfilled") fulfilled++;
        else if (r.getStatus() == "Rejected") rejected++;
        else pending++;
        if (r.isEmergency()) emergency++;
    });
    Utils::print("Registered users     : " + to_string(sys.getUsers().size()));
    Utils::print("Registered donors    : " + to_string(sys.getDonors().size()));
    Utils::print("Donors available now : " + to_string(available));
    Utils::print("Donations recorded   : " + to_string(sys.getDonations().size()));
    Utils::print("Units ever donated   : " + to_string(unitsDonated));
    Utils::print("Units in stock       : " + to_string(sys.totalStock()));
    Utils::blank();
    Utils::print("Total requests       : " + to_string(sys.getRequests().size()));
    Utils::print("   Pending           : " + to_string(pending));
    Utils::print("   Fulfilled         : " + to_string(fulfilled));
    Utils::print("   Rejected          : " + to_string(rejected));
    Utils::print("   Emergency         : " + to_string(emergency));
    Utils::pause();
}

void Admin::mergeSortDemo(BloodBankSystem& sys) {
    (void)sys;
    Utils::heading("MERGE SORT DEMO");
    string dates[3] = {"25-08-2026", "01-08-2026", "10-08-2026"};
    Utils::print("Before:");
    for (int i = 0; i < 3; i++) Utils::print("   " + dates[i]);
    MergeSort::sortDates(dates, 3);
    Utils::blank();
    Utils::print("After:");
    for (int i = 0; i < 3; i++) Utils::print("   " + dates[i]);
    Utils::pause();
}

// ------------------------------------------------------------------ 6. activity
void Admin::viewActivityHistory(BloodBankSystem& sys) {
    Utils::heading("ACTIVITY HISTORY");
    Utils::print("(Stack: latest activity first)");
    Utils::blank();
    sys.getActivityHistory().display(Utils::pad(), 25);
    if (sys.getActivityHistory().size() > 25)
        Utils::print("... showing latest 25 of " + to_string(sys.getActivityHistory().size()));
    Utils::pause();
}

// ------------------------------------------------------------------ 7. password
void Admin::changePassword(BloodBankSystem& sys) {
    Utils::heading("CHANGE ADMIN PASSWORD");
    string oldPw = Utils::ask("Current password");
    if (Utils::hashPassword(oldPw) != passwordHash) {
        Utils::print("Current password is incorrect.");
        Utils::pause();
        return;
    }
    string newPw = Utils::ask("New password (min 4 characters)");
    if (newPw.size() < 4) { Utils::print("Password is too short."); Utils::pause(); return; }
    if (Utils::ask("Confirm new password") != newPw) { Utils::print("Passwords do not match."); Utils::pause(); return; }
    changePassword(oldPw, newPw);
    sys.logActivity("Admin password changed");
    sys.saveAll();
    Utils::print("Password changed successfully.");
    Utils::pause();
}
