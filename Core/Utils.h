#ifndef UTILS_H
#define UTILS_H

#include <bits/stdc++.h>
using namespace std;

// Utils: small static helper class used everywhere (console style, input, dates).
// Keeps the "centered console style" of the old project in one place.
class Utils {
public:
    static constexpr int PAD = 25;        // left padding for menus (centered look)
    static constexpr int TABLE_PAD = 3;   // left padding for wide tables

    // ------------------------------------------------ console output
    static string pad() { return string(PAD, ' '); }

    static void clearScreen() {
#ifdef _WIN32
        int rc = system("cls");
#else
        int rc = system("clear");
#endif
        (void)rc;
    }

    static void print(const string& text = "") { cout << pad() << text << "\n"; }
    static void blank(int n = 1) { for (int i = 0; i < n; i++) cout << "\n"; }
    static void row(const string& text) { cout << string(TABLE_PAD, ' ') << text << "\n"; }

    static void heading(const string& title) {
        clearScreen();
        blank(2);
        print(title);
        blank(2);
    }

    // pad / cut a string to an exact column width (for tables)
    static string fit(string s, size_t width) {
        if (s.size() >= width) s = s.substr(0, width - 1);
        s += string(width - s.size(), ' ');
        return s;
    }

    // ------------------------------------------------ string helpers
    static string trim(const string& s) {
        size_t a = 0, b = s.size();
        while (a < b && isspace((unsigned char)s[a])) a++;
        while (b > a && isspace((unsigned char)s[b - 1])) b--;
        return s.substr(a, b - a);
    }
    static string toUpper(string s) { for (char& c : s) c = (char)toupper((unsigned char)c); return s; }
    static string toLower(string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }
    static bool iequals(const string& a, const string& b) { return toLower(a) == toLower(b); }
    static string sanitize(string s) { for (char& c : s) if (c == '|') c = '/'; return s; }

    static bool isNumber(const string& s) {
        if (s.empty() || s.size() > 9) return false;
        for (char c : s) if (!isdigit((unsigned char)c)) return false;
        return true;
    }
    static int toInt(const string& s, int def = 0) {
        try { return stoi(s); } catch (...) { return def; }
    }

    // split "a|b|c" into out[] (at most maxParts parts). Returns number of parts.
    static int split(const string& line, char delim, string* out, int maxParts) {
        int count = 0;
        string cur;
        for (char c : line) {
            if (c == delim && count < maxParts - 1) { out[count++] = cur; cur.clear(); }
            else cur += c;
        }
        out[count++] = cur;
        return count;
    }

    // ------------------------------------------------ input
    static string readLine() {
        string s;
        if (!getline(cin, s)) { cout << "\n"; exit(0); }   // input closed -> leave cleanly
        return s;
    }
    static string ask(const string& label) {
        cout << pad() << label << ": ";
        return sanitize(trim(readLine()));
    }
    static string askNonEmpty(const string& label) {
        while (true) {
            string s = ask(label);
            if (!s.empty()) return s;
            print("Input cannot be empty. Try again.");
        }
    }
    static int askInt(const string& label, int lo, int hi) {
        while (true) {
            string s = ask(label);
            if (isNumber(s)) {
                int v = stoi(s);
                if (v >= lo && v <= hi) return v;
            }
            print("Please enter a number between " + to_string(lo) + " and " + to_string(hi) + ".");
        }
    }
    static bool askYesNo(const string& label) {
        while (true) {
            string s = toLower(ask(label + " (y/n)"));
            if (s == "y" || s == "yes") return true;
            if (s == "n" || s == "no") return false;
            print("Please enter y or n.");
        }
    }
    static void pause() {
        blank();
        cout << pad() << "Press Enter to continue...";
        readLine();
    }

    // ------------------------------------------------ password (educational hash, djb2)
    static string hashPassword(const string& plain) {
        unsigned long long h = 5381;
        for (unsigned char c : plain) h = h * 33 + c;
        return to_string(h);
    }

    // ------------------------------------------------ dates (format DD-MM-YYYY)
    static bool parseDate(const string& d, int& day, int& mon, int& year) {
        if (d.size() != 10 || d[2] != '-' || d[5] != '-') return false;
        for (int i = 0; i < 10; i++) {
            if (i == 2 || i == 5) continue;
            if (!isdigit((unsigned char)d[i])) return false;
        }
        day = stoi(d.substr(0, 2));
        mon = stoi(d.substr(3, 2));
        year = stoi(d.substr(6, 4));
        if (year < 1900 || year > 2100 || mon < 1 || mon > 12 || day < 1) return false;
        int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        int maxDay = dim[mon - 1] + ((mon == 2 && leap) ? 1 : 0);
        return day <= maxDay;
    }
    static bool isValidDate(const string& d) { int a, b, c; return parseDate(d, a, b, c); }

    // YYYYMMDD number used as the sort key for Merge Sort (0 if invalid)
    static long long dateKey(const string& d) {
        int day, mon, year;
        if (!parseDate(d, day, mon, year)) return 0;
        return (long long)year * 10000 + mon * 100 + day;
    }
    static long long daysFromCivil(long long y, int m, int d) {
        y -= (m <= 2);
        long long era = (y >= 0 ? y : y - 399) / 400;
        long long yoe = y - era * 400;
        long long doy = (153LL * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + doe - 719468;
    }
    // days from 'older' to 'newer'. Returns false if a date is invalid.
    static bool daysBetween(const string& older, const string& newer, int& out) {
        int d1, m1, y1, d2, m2, y2;
        if (!parseDate(older, d1, m1, y1) || !parseDate(newer, d2, m2, y2)) return false;
        out = (int)(daysFromCivil(y2, m2, d2) - daysFromCivil(y1, m1, d1));
        return true;
    }
    static string today() {
        time_t t = time(nullptr);
        tm* lt = localtime(&t);
        char buf[16];
        strftime(buf, sizeof(buf), "%d-%m-%Y", lt);
        return string(buf);
    }
    static string timestamp() {
        time_t t = time(nullptr);
        tm* lt = localtime(&t);
        char buf[32];
        strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M", lt);
        return string(buf);
    }
};

#endif
