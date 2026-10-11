#include "MergeSort.h"
#include "../Core/Utils.h"

// merge the two sorted halves [left..mid] and [mid+1..right]
void MergeSort::merge(long long* keys, int* ids, long long* tk, int* ti, int left, int mid, int right) {
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        if (keys[i] <= keys[j]) { tk[k] = keys[i]; ti[k++] = ids[i++]; }   // <= keeps it stable
        else                    { tk[k] = keys[j]; ti[k++] = ids[j++]; }
    }
    while (i <= mid)   { tk[k] = keys[i]; ti[k++] = ids[i++]; }
    while (j <= right) { tk[k] = keys[j]; ti[k++] = ids[j++]; }
    for (int x = left; x <= right; x++) { keys[x] = tk[x]; ids[x] = ti[x]; }
}

void MergeSort::sortRec(long long* keys, int* ids, long long* tk, int* ti, int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    sortRec(keys, ids, tk, ti, left, mid);
    sortRec(keys, ids, tk, ti, mid + 1, right);
    merge(keys, ids, tk, ti, left, mid, right);
}

void MergeSort::sortByKey(long long* keys, int* ids, int n) {
    if (n <= 1) return;
    long long* tk = new long long[n];
    int* ti = new int[n];
    sortRec(keys, ids, tk, ti, 0, n - 1);
    delete[] tk;
    delete[] ti;
}

void MergeSort::sortDates(string* dates, int n) {
    if (n <= 1) return;
    long long* keys = new long long[n];
    int* ids = new int[n];
    string* copy = new string[n];
    for (int i = 0; i < n; i++) { keys[i] = Utils::dateKey(dates[i]); ids[i] = i; copy[i] = dates[i]; }
    sortByKey(keys, ids, n);
    for (int i = 0; i < n; i++) dates[i] = copy[ids[i]];
    delete[] keys;
    delete[] ids;
    delete[] copy;
}
