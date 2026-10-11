#include "BloodBankSystem.h"

BloodBankSystem::BloodBankSystem() : currentUser(nullptr), nextRequestId(1), nextDonationId(1) {}

// =====================================================================================
//  START-UP / SHUTDOWN
// =====================================================================================
void BloodBankSystem::seedDefaultHalls() {
    const char* names[8] = {"KUET Medical Center", "Rokeya Hall", "Amar Ekushey Hall",
                            "Fazlul Haque Hall", "Khan Jahan Ali Hall", "Lalan Shah Hall",
                            "Dr. M. A. Rashid Hall", "Shaheed Smrity Hall"};
    for (int i = 0; i < 8; i++) hallGraph.addNode(names[i]);
    // {nodeA, nodeB, meters, minutes}  (sample values - edit Data/halls.txt to change)
    int edges[10][4] = {{0, 1, 300, 3}, {0, 2, 600, 6}, {0, 3, 900, 9}, {1, 2, 400, 4},
                        {1, 3, 700, 7}, {2, 5, 500, 5}, {3, 4, 700, 7}, {4, 6, 400, 4},
                        {5, 7, 600, 6}, {6, 7, 800, 8}};
    for (int i = 0; i < 10; i++) hallGraph.addEdge(edges[i][0], edges[i][1], edges[i][2], edges[i][3]);
}

void BloodBankSystem::loadData() {
    string adminId, adminHash;
    fileManager.loadUsers(users, adminId, adminHash);
    if (adminId.empty()) { adminId = "admin"; adminHash = Utils::hashPassword("admin123"); }
    admin.setCredentials(adminId, adminHash);

    fileManager.loadDonors(donors, donations);
    fileManager.loadRequests(requests);
    fileManager.loadInventory(inventory);
    for (int i = 0; i < BloodTree::TOTAL; i++)
        if (inventory.find(BloodTree::groupAt(i)) == inventory.end()) inventory[BloodTree::groupAt(i)] = 0;

    if (!fileManager.loadHalls(hallGraph)) {
        seedDefaultHalls();
        fileManager.saveHalls(hallGraph);
    }
    if (hallGraph.findNode(HallGraph::medicalCenterName()) == -1)
        hallGraph.addNode(HallGraph::medicalCenterName());

    donations.forEach([&](Donation& d) { nextDonationId = max(nextDonationId, d.getId() + 1); });

    // rebuild the processing queues from pending requests (oldest first)
    requests.forEach([&](BloodRequest& r) {
        nextRequestId = max(nextRequestId, r.getId() + 1);
        if (r.isPending()) {
            if (r.isEmergency()) emergencyQueue.enqueue(&r);
            else normalQueue.enqueue(&r);
        }
    });

    logActivity("System started");
}

void BloodBankSystem::saveAll() {
    fileManager.saveUsers(users, admin.getId(), admin.getPasswordHash());
    fileManager.saveDonors(donors, donations);
    fileManager.saveRequests(requests);
    fileManager.saveInventory(inventory);
    fileManager.saveHalls(hallGraph);
}

void BloodBankSystem::logActivity(const string& message) {
    activityHistory.push("[" + Utils::timestamp() + "] " + message);
}

void BloodBankSystem::run() {
    loadData();
    while (true) {
        Utils::heading("KUET LIFEFLOW - SMART BLOOD BANK SYSTEM");
        Utils::print("1. User Login");
        Utils::print("2. Register New User");
        Utils::print("3. Admin Login");
        Utils::print("4. Back to Main Menu");
        Utils::blank();
        int c = Utils::askInt("Enter choice", 1, 4);
        if (c == 1) loginUser();
        else if (c == 2) registerUser();
        else if (c == 3) adminLogin();
        else break;
    }
    saveAll();
}

void BloodBankSystem::showAbout() {
    Utils::heading("ABOUT KUET LIFEFLOW");
    Utils::print("KUET LifeFlow - Smart Blood Bank Management System");
    Utils::print("Using Data Structures and Algorithms (C++)");
    Utils::blank();
    Utils::print("Data Structure / Algorithm      Used for");
    Utils::print("------------------------------  ------------------------------");
    Utils::print("Class (OOP)                     Whole project");
    Utils::print("Linked List                     Users, donors, requests");
    Utils::print("Stack                           Activity history");
    Utils::print("Queue                           Blood request processing");
    Utils::print("Map                             Blood inventory");
    Utils::print("Tree                            Blood compatibility");
    Utils::print("Graph                           KUET hall network");
    Utils::print("BFS                             Nearest donor search");
    Utils::print("Merge Sort                      Donation / request reports");
    Utils::print("Searching                       Donor, blood, request search");
    Utils::print("File Handling                   Data persistence");
    Utils::pause();
}

// =====================================================================================
//  SMALL SHARED HELPERS
// =====================================================================================
bool BloodBankSystem::isValidPhone(const string& phone) {
    if (phone.size() != 11) return false;
    for (char c : phone) if (!isdigit((unsigned char)c)) return false;
    return phone[0] == '0' && phone[1] == '1';
}

string BloodBankSystem::askBloodGroup(const string& label) {
    while (true) {
        string g = Utils::toUpper(Utils::ask(label + " (A+ A- B+ B- AB+ AB- O+ O-)"));
        if (BloodTree::isValidGroup(g)) return g;
        Utils::print("Invalid blood group. Try again.");
    }
}

string BloodBankSystem::chooseLocation(bool includeMedicalCenter) {
    int shown = 0;
    int* idx = new int[hallGraph.nodeCount()];
    for (int i = 0; i < hallGraph.nodeCount(); i++) {
        if (!includeMedicalCenter && hallGraph.nameOf(i) == HallGraph::medicalCenterName()) continue;
        idx[shown++] = i;
        Utils::print(to_string(shown) + ". " + hallGraph.nameOf(i));
    }
    int c = Utils::askInt("Select (1-" + to_string(shown) + ")", 1, shown);
    string name = hallGraph.nameOf(idx[c - 1]);
    delete[] idx;
    return name;
}

int BloodBankSystem::stockOf(const string& group) {
    return max(0, Search::searchInventory(inventory, group));
}

int BloodBankSystem::totalStock() {
    int t = 0;
    for (int i = 0; i < BloodTree::TOTAL; i++) t += stockOf(BloodTree::groupAt(i));
    return t;
}

bool BloodBankSystem::adjustStock(const string& group, int delta) {
    if (!BloodTree::isValidGroup(group)) return false;
    int now = stockOf(group);
    if (now + delta < 0) return false;
    inventory[group] = now + delta;
    return true;
}

void BloodBankSystem::setStock(const string& group, int units) {
    if (BloodTree::isValidGroup(group) && units >= 0) inventory[group] = units;
}

void BloodBankSystem::displayInventory() {
    Utils::print(Utils::fit("Blood Group", 14) + Utils::fit("Units", 8) + "Status");
    Utils::print(Utils::fit("-----------", 14) + Utils::fit("-----", 8) + "------");
    for (int i = 0; i < BloodTree::TOTAL; i++) {
        const string& g = BloodTree::groupAt(i);
        int u = stockOf(g);
        string status = u == 0 ? "Out of stock" : (u <= 3 ? "Low" : "OK");
        Utils::print(Utils::fit(g, 14) + Utils::fit(to_string(u), 8) + status);
    }
    Utils::print("");
    Utils::print("Total units in stock: " + to_string(totalStock()));
}

int BloodBankSystem::countMatchingDonors(const string& hall, const string& group) {
    int c = 0;
    donors.forEach([&](Donor& d) {
        if (Utils::iequals(d.getHall(), hall) && d.isAvailable() &&
            bloodTree.canDonate(d.getBloodGroup(), group)) c++;
    });
    return c;
}

// =====================================================================================
//  REGISTRATION / LOGIN
// =====================================================================================
void BloodBankSystem::registerUser() {
    Utils::heading("USER REGISTRATION");
    string name = Utils::askNonEmpty("Full Name");

    string phone;
    while (true) {
        phone = Utils::ask("Phone (11 digits, e.g. 01712345678)");
        if (!isValidPhone(phone)) { Utils::print("Invalid phone number."); continue; }
        if (Search::findUserByPhone(users, phone)) { Utils::print("This phone is already registered."); continue; }
        break;
    }
    int age = Utils::askInt("Age", 15, 80);

    Utils::print("Gender:  1. Male   2. Female   3. Other");
    int g = Utils::askInt("Select", 1, 3);
    string gender = g == 1 ? "Male" : (g == 2 ? "Female" : "Other");

    string address = Utils::askNonEmpty("Address");
    Utils::blank();
    Utils::print("Select your hall:");
    string hall = chooseLocation(false);
    Utils::blank();
    string group = askBloodGroup();

    string pw, pw2;
    while (true) {
        pw = Utils::ask("Password (min 4 characters)");
        if (pw.size() < 4) { Utils::print("Password is too short."); continue; }
        pw2 = Utils::ask("Confirm Password");
        if (pw != pw2) { Utils::print("Passwords do not match."); continue; }
        break;
    }

    users.insertBack(User(name, phone, age, gender, address, hall, group, Utils::hashPassword(pw)));
    logActivity("User registered: " + name + " (" + phone + ")");
    saveAll();
    Utils::blank();
    Utils::print("Registration successful! You can log in now.");
    Utils::pause();
}

void BloodBankSystem::loginUser() {
    Utils::heading("USER LOGIN");
    for (int attempt = 1; attempt <= 3; attempt++) {
        string phone = Utils::ask("Phone");
        string pw = Utils::ask("Password");
        User* u = Search::findUserByPhone(users, phone);
        if (u && u->checkPassword(pw)) {
            currentUser = u;
            logActivity("User logged in: " + u->getName());
            userDashboard();
            currentUser = nullptr;
            return;
        }
        Utils::print("Invalid phone or password. Attempts left: " + to_string(3 - attempt));
    }
    Utils::pause();
}

void BloodBankSystem::adminLogin() {
    Utils::heading("ADMIN LOGIN");
    for (int attempt = 1; attempt <= 3; attempt++) {
        string id = Utils::ask("Admin ID");
        string pw = Utils::ask("Password");
        if (admin.login(id, pw)) {
            logActivity("Admin logged in");
            admin.dashboard(*this);
            return;
        }
        Utils::print("Invalid admin ID or password. Attempts left: " + to_string(3 - attempt));
    }
    Utils::pause();
}

// =====================================================================================
//  USER DASHBOARD
// =====================================================================================
void BloodBankSystem::userDashboard() {
    while (true) {
        Utils::heading("USER DASHBOARD");
        Utils::print("Welcome, " + currentUser->getName());
        Utils::blank();
        Utils::print("1. View Profile");
        Utils::print("2. Become Donor");
        Utils::print("3. Search Blood");
        Utils::print("4. Request Blood");
        Utils::print("5. Find Nearest Donor");
        Utils::print("6. Blood Compatibility Check");
        Utils::print("7. Donation History");
        Utils::print("8. Logout");
        Utils::blank();
        int c = Utils::askInt("Enter choice", 1, 8);
        switch (c) {
            case 1: viewProfile(); break;
            case 2: becomeDonor(); break;
            case 3: searchBlood(); break;
            case 4: requestBlood(); break;
            case 5: findNearestDonor(); break;
            case 6: compatibilityCheck(); break;
            case 7: donationHistory(); break;
            case 8: logActivity("User logged out: " + currentUser->getName()); return;
        }
    }
}

void BloodBankSystem::viewProfile() {
    Utils::heading("MY PROFILE");
    currentUser->displayProfile();
    Donor* d = Search::findDonorByPhone(donors, currentUser->getPhone());
    Utils::print("Donor Status  : " + (d ? d->statusText() : string("Not a donor")));
    if (d) Utils::print("Last Donation : " + d->getLastDonationDate());
    Utils::pause();
}

void BloodBankSystem::becomeDonor() {
    Utils::heading("BECOME A DONOR");
    if (Search::findDonorByPhone(donors, currentUser->getPhone())) {
        Utils::print("You are already registered as a donor.");
        Utils::pause();
        return;
    }
    if (currentUser->getAge() < 18) {
        Utils::print("Donors must be at least 18 years old.");
        Utils::pause();
        return;
    }
    Utils::print("Name        : " + currentUser->getName());
    Utils::print("Blood Group : " + currentUser->getBloodGroup());
    Utils::print("Hall        : " + currentUser->getHall());
    Utils::print("Phone       : " + currentUser->getPhone());
    Utils::blank();

    string last;
    while (true) {
        last = Utils::ask("Last donation date (DD-MM-YYYY, or N if never)");
        if (Utils::iequals(last, "n") || Utils::iequals(last, "no")) { last = "N/A"; break; }
        if (!Utils::isValidDate(last)) { Utils::print("Invalid date format."); continue; }
        if (Utils::dateKey(last) > Utils::dateKey(Utils::today())) { Utils::print("Date cannot be in the future."); continue; }
        break;
    }

    Donor* d = donors.insertBack(Donor(currentUser->getName(), currentUser->getBloodGroup(),
                                       currentUser->getHall(), currentUser->getPhone(), last, true));
    logActivity("New donor registered: " + currentUser->getName() + " (" + currentUser->getBloodGroup() + ")");
    saveAll();
    Utils::blank();
    Utils::print("Thank you! You are now a registered donor.");
    Utils::print("Current status: " + d->statusText());
    Utils::pause();
}

void BloodBankSystem::searchBlood() {
    Utils::heading("SEARCH BLOOD");
    string g = askBloodGroup("Blood group needed");
    Utils::blank();
    Utils::print("Stock of " + g + " : " + to_string(stockOf(g)) + " unit(s)");
    Utils::blank();
    Utils::print("A patient with " + g + " can receive from (Blood Tree):");
    for (int i = 0; i < BloodTree::TOTAL; i++) {
        const string& d = BloodTree::groupAt(i);
        if (bloodTree.canDonate(d, g))
            Utils::print("   " + Utils::fit(d, 5) + " : " + to_string(stockOf(d)) + " unit(s)");
    }
    Utils::blank();
    LinkedList<Donor*> found;
    Search::findDonorsByBloodGroup(donors, g, found, true);
    Utils::print("Available " + g + " donors: " + to_string(found.size()));
    for (ListNode<Donor*>* p = found.getHead(); p; p = p->next)
        Utils::print("   " + Utils::fit(p->data->getName(), 20) + Utils::fit(p->data->getHall(), 24) + p->data->getPhone());
    Utils::pause();
}

void BloodBankSystem::requestBlood() {
    Utils::heading("REQUEST BLOOD");
    string patient = Utils::askNonEmpty("Patient name");
    string group = askBloodGroup("Patient blood group");
    int units = Utils::askInt("Required units (1-10)", 1, 10);
    Utils::blank();
    Utils::print("Select hall / location:");
    string location = chooseLocation(true);
    bool emergency = Utils::askYesNo("Is this an emergency?");

    BloodRequest* r = requests.insertBack(BloodRequest(nextRequestId++, patient, group, units, location,
                                                       emergency, currentUser->getPhone(), Utils::today()));
    int position;
    if (emergency) { emergencyQueue.enqueue(r); position = emergencyQueue.size(); }
    else { normalQueue.enqueue(r); position = emergencyQueue.size() + normalQueue.size(); }

    logActivity("Blood request #" + to_string(r->getId()) + " (" + group + " x" + to_string(units) +
                (emergency ? ", EMERGENCY" : "") + ") for " + patient);
    saveAll();

    Utils::blank();
    Utils::print("Request submitted successfully!");
    Utils::print("Request ID     : " + to_string(r->getId()));
    Utils::print("Queue position : " + to_string(position));
    Utils::print("Current stock of " + group + " : " + to_string(stockOf(group)) + " unit(s)");
    Utils::pause();
}

void BloodBankSystem::findNearestDonor() {
    Utils::heading("FIND NEAREST DONOR");
    int start = hallGraph.findNode(HallGraph::medicalCenterName());
    Utils::print("Search starts from: " + HallGraph::medicalCenterName());
    string group = askBloodGroup("Blood group needed");
    Utils::blank();

    auto hasDonor = [&](int node) { return countMatchingDonors(hallGraph.nameOf(node), group) > 0; };
    NearestResult r = hallGraph.bfsNearest(start, hasDonor);

    Utils::print("BFS search (level by level):");
    istringstream trace(r.trace);
    string line;
    while (getline(trace, line)) Utils::print("   " + line);
    Utils::blank();

    if (!r.found) {
        Utils::print("No available donor found in any hall for " + group + ".");
        Utils::pause();
        return;
    }
    Utils::print("Nearest available donor:");
    Utils::blank();
    Utils::print("Hall     : " + hallGraph.nameOf(r.node));
    Utils::print("Time     : " + to_string(r.minutes) + " minutes");
    Utils::print("Distance : " + to_string(r.meters) + " meters");
    Utils::print("Route    : " + r.path);
    Utils::blank();
    Utils::print("Donors in this hall:");
    donors.forEach([&](Donor& d) {
        if (Utils::iequals(d.getHall(), hallGraph.nameOf(r.node)) && d.isAvailable() &&
            bloodTree.canDonate(d.getBloodGroup(), group))
            Utils::print("   " + Utils::fit(d.getName(), 20) + Utils::fit(d.getBloodGroup(), 5) +
                         d.getPhone() + (d.getBloodGroup() == group ? "  (exact match)" : "  (compatible)"));
    });
    Utils::pause();
}

void BloodBankSystem::showCompatibility(const string& group) {
    bool flags[BloodTree::TOTAL];
    Utils::print("Blood group: " + group);
    Utils::blank();
    Utils::print("Can donate to:");
    bloodTree.getRecipients(group, flags);
    for (int i = 0; i < BloodTree::TOTAL; i++)
        if (flags[i]) Utils::print(BloodTree::groupAt(i));
    Utils::blank();
    Utils::print("Can receive from:");
    bloodTree.getDonors(group, flags);
    for (int i = 0; i < BloodTree::TOTAL; i++)
        if (flags[i]) Utils::print(BloodTree::groupAt(i));
}

void BloodBankSystem::compatibilityCheck() {
    while (true) {
        Utils::heading("BLOOD COMPATIBILITY CHECK");
        Utils::print("1. Check a blood group");
        Utils::print("2. Check donor -> patient pair");
        Utils::print("3. View compatibility tree");
        Utils::print("4. Back");
        Utils::blank();
        int c = Utils::askInt("Enter choice", 1, 4);
        if (c == 4) return;
        Utils::blank();
        if (c == 1) {
            string g = askBloodGroup();
            Utils::blank();
            showCompatibility(g);
        } else if (c == 2) {
            string d = askBloodGroup("Donor blood group");
            string p = askBloodGroup("Patient blood group");
            Utils::blank();
            Utils::print(d + (bloodTree.canDonate(d, p) ? "  CAN  donate to  " : "  CANNOT donate to  ") + p);
        } else {
            Utils::print("Preorder view (a child = 'can also donate to'):");
            Utils::blank();
            bloodTree.preorder(Utils::pad());
            Utils::blank();
            Utils::print("Level order view:");
            bloodTree.levelOrder(Utils::pad());
        }
        Utils::pause();
    }
}

void BloodBankSystem::donationHistory() {
    Utils::heading("DONATION HISTORY");
    int n = 0;
    Donation** sorted = getSortedDonations(currentUser->getPhone(), n);   // Merge Sort by date
    if (n == 0) {
        Utils::print("No donations recorded for you yet.");
    } else {
        Utils::row(Donation::tableHeader());
        int total = 0;
        for (int i = 0; i < n; i++) { Utils::row(sorted[i]->rowText()); total += sorted[i]->getUnits(); }
        Utils::blank();
        Utils::print("Total donated: " + to_string(total) + " unit(s) in " + to_string(n) + " donation(s)");
    }
    delete[] sorted;

    Utils::blank(2);
    Utils::print("YOUR BLOOD REQUESTS");
    Utils::blank();
    int count = 0;
    for (ListNode<BloodRequest>* p = requests.getHead(); p; p = p->next) {
        if (p->data.getRequestedBy() != currentUser->getPhone()) continue;
        if (count++ == 0) Utils::row(BloodRequest::tableHeader());
        Utils::row(p->data.rowText());
    }
    if (count == 0) Utils::print("You have not made any blood request.");
    Utils::pause();
}

// =====================================================================================
//  SHARED BUSINESS LOGIC (used by user + admin panels)
// =====================================================================================
bool BloodBankSystem::recordDonation(Donor& donor, int units, const string& date, string& message) {
    if (!donor.isEligibleOn(date)) {
        message = "Donor must wait " + to_string(Donor::GAP_DAYS) + " days after the last donation (" +
                  donor.getLastDonationDate() + ").";
        return false;
    }
    donations.insertBack(Donation(nextDonationId++, donor.getName(), donor.getPhone(),
                                  donor.getBloodGroup(), units, date));
    donor.setLastDonationDate(date);
    adjustStock(donor.getBloodGroup(), units);
    logActivity("Donation: " + donor.getName() + " gave " + to_string(units) + " unit(s) of " + donor.getBloodGroup());
    logActivity("Inventory updated: " + donor.getBloodGroup() + " +" + to_string(units));
    saveAll();
    message = "Donation recorded and inventory updated.";
    return true;
}

// Builds a date-sorted array of donation pointers using Merge Sort.
Donation** BloodBankSystem::getSortedDonations(const string& phoneFilter, int& count) {
    count = 0;
    donations.forEach([&](Donation& d) { if (phoneFilter.empty() || d.getDonorPhone() == phoneFilter) count++; });
    if (count == 0) return nullptr;

    Donation** items = new Donation*[count];
    long long* keys = new long long[count];
    int* ids = new int[count];
    int i = 0;
    donations.forEach([&](Donation& d) {
        if (phoneFilter.empty() || d.getDonorPhone() == phoneFilter) {
            items[i] = &d; keys[i] = Utils::dateKey(d.getDate()); ids[i] = i; i++;
        }
    });
    MergeSort::sortByKey(keys, ids, count);

    Donation** sorted = new Donation*[count];
    for (int k = 0; k < count; k++) sorted[k] = items[ids[k]];
    delete[] items; delete[] keys; delete[] ids;
    return sorted;
}

BloodRequest** BloodBankSystem::getSortedRequests(int& count) {
    count = requests.size();
    if (count == 0) return nullptr;

    BloodRequest** items = new BloodRequest*[count];
    long long* keys = new long long[count];
    int* ids = new int[count];
    int i = 0;
    requests.forEach([&](BloodRequest& r) { items[i] = &r; keys[i] = Utils::dateKey(r.getDate()); ids[i] = i; i++; });
    MergeSort::sortByKey(keys, ids, count);

    BloodRequest** sorted = new BloodRequest*[count];
    for (int k = 0; k < count; k++) sorted[k] = items[ids[k]];
    delete[] items; delete[] keys; delete[] ids;
    return sorted;
}

void BloodBankSystem::displayPendingQueues() {
    Utils::print("EMERGENCY QUEUE (served first, FIFO): " + to_string(emergencyQueue.size()));
    Utils::blank();
    if (emergencyQueue.isEmpty()) Utils::print("   (empty)");
    int pos = 1;
    emergencyQueue.forEach([&](BloodRequest*& r) {
        Utils::print(to_string(pos++) + ". #" + to_string(r->getId()) + "  " + r->getPatientName() + "  " +
                     r->getBloodGroup() + " x" + to_string(r->getUnits()) + "  @ " + r->getLocation());
    });
    Utils::blank();
    Utils::print("NORMAL QUEUE (FIFO): " + to_string(normalQueue.size()));
    Utils::blank();
    if (normalQueue.isEmpty()) Utils::print("   (empty)");
    pos = 1;
    normalQueue.forEach([&](BloodRequest*& r) {
        Utils::print(to_string(pos++) + ". #" + to_string(r->getId()) + "  " + r->getPatientName() + "  " +
                     r->getBloodGroup() + " x" + to_string(r->getUnits()) + "  @ " + r->getLocation());
    });
}

// plan[i] = units to take from groupAt(i). Exact group first, then compatible groups.
// Compatible groups are tried from AB+ down to O-, so the universal donor (O-) is used last.
bool BloodBankSystem::planIssue(const string& group, int units, int* plan) {
    for (int i = 0; i < BloodTree::TOTAL; i++) plan[i] = 0;
    int remaining = units;
    int exact = BloodTree::groupIndex(group);
    int take = min(remaining, stockOf(group));
    plan[exact] = take;
    remaining -= take;
    for (int i = BloodTree::TOTAL - 1; i >= 0 && remaining > 0; i--) {
        if (i == exact || !bloodTree.canDonate(BloodTree::groupAt(i), group)) continue;
        take = min(remaining, stockOf(BloodTree::groupAt(i)));
        plan[i] = take;
        remaining -= take;
    }
    return remaining == 0;
}

// Queue in action: the emergency queue is served first, then the normal queue (both FIFO).
void BloodBankSystem::processNextRequest() {
    BloodRequest* r = nullptr;
    bool fromEmergency = false;
    if (emergencyQueue.front(r)) fromEmergency = true;
    else if (!normalQueue.front(r)) {
        Utils::print("No pending requests in the queue.");
        Utils::pause();
        return;
    }

    Utils::print("Next request in queue" + string(fromEmergency ? " (EMERGENCY):" : ":"));
    Utils::blank();
    r->displayDetails();
    Utils::blank();

    int plan[BloodTree::TOTAL];
    bool ok = planIssue(r->getBloodGroup(), r->getUnits(), plan);
    BloodRequest* dummy = nullptr;

    if (ok) {
        Utils::print("Blood that will be issued from inventory:");
        for (int i = 0; i < BloodTree::TOTAL; i++)
            if (plan[i] > 0) Utils::print("   " + BloodTree::groupAt(i) + " : " + to_string(plan[i]) + " unit(s)");
        Utils::blank();
        if (!Utils::askYesNo("Issue blood and mark request as Fulfilled?")) return;
        for (int i = 0; i < BloodTree::TOTAL; i++)
            if (plan[i] > 0) adjustStock(BloodTree::groupAt(i), -plan[i]);
        if (fromEmergency) emergencyQueue.dequeue(dummy); else normalQueue.dequeue(dummy);
        r->setStatus("Fulfilled");
        logActivity("Request #" + to_string(r->getId()) + " fulfilled");
        logActivity("Inventory updated: issued " + to_string(r->getUnits()) + " unit(s) for " + r->getBloodGroup() + " request");
        saveAll();
        Utils::print("Request fulfilled and inventory updated.");
    } else {
        Utils::print("Not enough compatible blood in stock for this request.");
        Utils::blank();
        Utils::print("1. Reject this request");
        Utils::print("2. Keep it in the queue (update inventory first)");
        int c = Utils::askInt("Enter choice", 1, 2);
        if (c == 1) {
            if (fromEmergency) emergencyQueue.dequeue(dummy); else normalQueue.dequeue(dummy);
            r->setStatus("Rejected");
            logActivity("Request #" + to_string(r->getId()) + " rejected (insufficient stock)");
            saveAll();
            Utils::print("Request rejected.");
        }
    }
    Utils::pause();
}

// =====================================================================================
//  PROGRAMMATIC / API METHODS (used by Web Server & REST API)
// =====================================================================================
bool BloodBankSystem::registerUserApi(const string& name, const string& phone, int age, const string& gender,
                                     const string& address, const string& hall, const string& bloodGroup,
                                     const string& password, string& errorMsg) {
    if (name.empty()) { errorMsg = "Name cannot be empty"; return false; }
    if (!isValidPhone(phone)) { errorMsg = "Phone must be 11 digits starting with 01"; return false; }
    if (Search::findUserByPhone(users, phone)) { errorMsg = "Phone number is already registered"; return false; }
    if (age < 15 || age > 80) { errorMsg = "Age must be between 15 and 80"; return false; }
    if (!BloodTree::isValidGroup(bloodGroup)) { errorMsg = "Invalid blood group"; return false; }
    if (password.length() < 4) { errorMsg = "Password must be at least 4 characters"; return false; }

    users.insertBack(User(name, phone, age, gender, address, hall, bloodGroup, Utils::hashPassword(password)));
    logActivity("User registered via Web: " + name + " (" + phone + ")");
    saveAll();
    return true;
}

User* BloodBankSystem::authenticateUser(const string& phone, const string& password) {
    User* u = Search::findUserByPhone(users, phone);
    if (u && u->checkPassword(password)) {
        return u;
    }
    return nullptr;
}

bool BloodBankSystem::becomeDonorApi(const string& phone, const string& lastDonationDate, string& errorMsg) {
    User* u = Search::findUserByPhone(users, phone);
    if (!u) { errorMsg = "User not found"; return false; }
    if (Search::findDonorByPhone(donors, phone)) { errorMsg = "You are already registered as a donor"; return false; }
    if (u->getAge() < 18) { errorMsg = "Donors must be at least 18 years old"; return false; }

    string last = lastDonationDate;
    if (last.empty() || Utils::iequals(last, "n") || Utils::iequals(last, "no") || Utils::iequals(last, "n/a")) {
        last = "N/A";
    } else {
        if (!Utils::isValidDate(last)) { errorMsg = "Invalid date format (use DD-MM-YYYY)"; return false; }
        if (Utils::dateKey(last) > Utils::dateKey(Utils::today())) { errorMsg = "Date cannot be in the future"; return false; }
    }

    donors.insertBack(Donor(u->getName(), u->getBloodGroup(), u->getHall(), u->getPhone(), last, true));
    logActivity("New donor registered via Web: " + u->getName() + " (" + u->getBloodGroup() + ")");
    saveAll();
    return true;
}

BloodRequest* BloodBankSystem::createRequestApi(const string& patientName, const string& bloodGroup, int units,
                                               const string& location, bool emergency, const string& requestedBy, int& queuePos) {
    if (patientName.empty() || !BloodTree::isValidGroup(bloodGroup) || units <= 0 || units > 10) return nullptr;

    BloodRequest* r = requests.insertBack(BloodRequest(nextRequestId++, patientName, bloodGroup, units, location,
                                                       emergency, requestedBy, Utils::today(), "Pending"));
    if (emergency) {
        emergencyQueue.enqueue(r);
        queuePos = emergencyQueue.size();
    } else {
        normalQueue.enqueue(r);
        queuePos = emergencyQueue.size() + normalQueue.size();
    }

    logActivity("Blood request #" + to_string(r->getId()) + " (" + bloodGroup + " x" + to_string(units) +
                (emergency ? ", EMERGENCY" : "") + ") for " + patientName);
    saveAll();
    return r;
}

bool BloodBankSystem::fulfillRequestApi(int requestId, string& errorMsg) {
    BloodRequest* req = Search::findRequestById(requests, requestId);
    if (!req) { errorMsg = "Request not found"; return false; }
    if (!req->isPending()) { errorMsg = "Request is not in Pending status"; return false; }

    int plan[BloodTree::TOTAL];
    if (!planIssue(req->getBloodGroup(), req->getUnits(), plan)) {
        errorMsg = "Insufficient stock to fulfill this request";
        return false;
    }

    for (int i = 0; i < BloodTree::TOTAL; i++) {
        if (plan[i] > 0) adjustStock(BloodTree::groupAt(i), -plan[i]);
    }

    req->setStatus("Fulfilled");
    logActivity("Request #" + to_string(req->getId()) + " fulfilled via API");
    saveAll();
    return true;
}

bool BloodBankSystem::rejectRequestApi(int requestId, string& errorMsg) {
    BloodRequest* req = Search::findRequestById(requests, requestId);
    if (!req) { errorMsg = "Request not found"; return false; }
    if (!req->isPending()) { errorMsg = "Request is not in Pending status"; return false; }

    req->setStatus("Rejected");
    logActivity("Request #" + to_string(req->getId()) + " rejected via API");
    saveAll();
    return true;
}

bool BloodBankSystem::processNextQueueRequestApi(string& outMessage, bool& outFulfilled) {
    BloodRequest* r = nullptr;
    bool fromEmergency = false;
    if (emergencyQueue.front(r)) fromEmergency = true;
    else if (!normalQueue.front(r)) {
        outMessage = "No pending requests in the queue.";
        outFulfilled = false;
        return false;
    }

    int plan[BloodTree::TOTAL];
    bool ok = planIssue(r->getBloodGroup(), r->getUnits(), plan);
    BloodRequest* dummy = nullptr;

    if (ok) {
        for (int i = 0; i < BloodTree::TOTAL; i++) {
            if (plan[i] > 0) adjustStock(BloodTree::groupAt(i), -plan[i]);
        }
        if (fromEmergency) emergencyQueue.dequeue(dummy); else normalQueue.dequeue(dummy);
        r->setStatus("Fulfilled");
        logActivity("Request #" + to_string(r->getId()) + " fulfilled from queue");
        saveAll();
        outMessage = "Request #" + to_string(r->getId()) + " (" + r->getBloodGroup() + " x" + to_string(r->getUnits()) + ") fulfilled successfully.";
        outFulfilled = true;
        return true;
    } else {
        outMessage = "Insufficient compatible blood stock for Request #" + to_string(r->getId()) + " (" + r->getBloodGroup() + " x" + to_string(r->getUnits()) + ").";
        outFulfilled = false;
        return false;
    }
}

