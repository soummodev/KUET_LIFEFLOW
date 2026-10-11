#ifndef MERGESORT_H
#define MERGESORT_H

#include <bits/stdc++.h>
using namespace std;

// Manual Merge Sort (no std::sort).
// Real-world use: sorting donation / request reports by date.
//
// The sort works on a key array (date as YYYYMMDD) and moves a parallel "ids" array
// with it. The caller then reads its records in the sorted order using ids[].
class MergeSort {
private:
    static void merge(long long* keys, int* ids, long long* tk, int* ti, int left, int mid, int right);
    static void sortRec(long long* keys, int* ids, long long* tk, int* ti, int left, int right);

public:
    // Sorts keys[0..n-1] ascending; ids[] is rearranged the same way. Stable, O(n log n).
    static void sortByKey(long long* keys, int* ids, int n);

    // Convenience: sorts date strings (DD-MM-YYYY) oldest -> newest.
    static void sortDates(string* dates, int n);
};

#endif
