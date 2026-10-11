#ifndef QUEUE_H
#define QUEUE_H

#include "LinkedList.h"

// Custom Queue (FIFO) built on linked nodes.
// Real-world use: Blood Request Processing (first request is served first)
// and level-by-level BFS in the hall graph.
template <typename T>
class Queue {
private:
    ListNode<T>* head;
    ListNode<T>* tail;
    int count;

public:
    Queue() : head(nullptr), tail(nullptr), count(0) {}
    ~Queue() {
        while (head) {
            ListNode<T>* t = head;
            head = head->next;
            delete t;
        }
    }
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(const T& value) {
        ListNode<T>* node = new ListNode<T>(value);
        if (!tail) head = tail = node;
        else { tail->next = node; tail = node; }
        ++count;
    }

    bool dequeue(T& out) {
        if (!head) return false;
        ListNode<T>* t = head;
        out = t->data;
        head = t->next;
        if (!head) tail = nullptr;
        delete t;
        --count;
        return true;
    }

    bool front(T& out) const {
        if (!head) return false;
        out = head->data;
        return true;
    }

    bool isEmpty() const { return count == 0; }
    int size() const { return count; }

    // T must support operator<<
    void display(const string& indent = "") const {
        if (!head) { cout << indent << "(queue is empty)\n"; return; }
        int i = 1;
        for (ListNode<T>* p = head; p; p = p->next, ++i)
            cout << indent << i << ". " << p->data << "\n";
    }

    template <typename F>
    void forEach(F f) {
        for (ListNode<T>* p = head; p; p = p->next) f(p->data);
    }
};

#endif
