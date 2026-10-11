#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <bits/stdc++.h>
using namespace std;

// Node of a singly linked list (also reused by Stack and Queue).
template <typename T>
class ListNode {
public:
    T data;
    ListNode<T>* next;
    ListNode(const T& value) : data(value), next(nullptr) {}
};

// Custom singly linked list.
// Real-world use: stores Users, Donors, Donations and Blood Requests.
template <typename T>
class LinkedList {
private:
    ListNode<T>* head;
    ListNode<T>* tail;
    int count;

public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}
    ~LinkedList() { clear(); }
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    // O(1). Returns pointer to the stored element (stays valid until it is removed).
    T* insertBack(const T& value) {
        ListNode<T>* node = new ListNode<T>(value);
        if (!head) head = tail = node;
        else { tail->next = node; tail = node; }
        ++count;
        return &node->data;
    }

    T* insertFront(const T& value) {
        ListNode<T>* node = new ListNode<T>(value);
        node->next = head;
        head = node;
        if (!tail) tail = node;
        ++count;
        return &node->data;
    }

    // remove first element for which pred(element) is true
    template <typename Pred>
    bool removeIf(Pred pred) {
        ListNode<T>* prev = nullptr;
        ListNode<T>* cur = head;
        while (cur) {
            if (pred(cur->data)) {
                if (prev) prev->next = cur->next; else head = cur->next;
                if (cur == tail) tail = prev;
                delete cur;
                --count;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    T* getAt(int index) {
        if (index < 0 || index >= count) return nullptr;
        ListNode<T>* p = head;
        for (int i = 0; i < index; i++) p = p->next;
        return &p->data;
    }

    ListNode<T>* getHead() const { return head; }
    int size() const { return count; }
    bool isEmpty() const { return count == 0; }

    void clear() {
        while (head) {
            ListNode<T>* t = head;
            head = head->next;
            delete t;
        }
        tail = nullptr;
        count = 0;
    }

    // visit every element in order
    template <typename F>
    void forEach(F f) {
        for (ListNode<T>* p = head; p; p = p->next) f(p->data);
    }
};

#endif
