#ifndef STACK_H
#define STACK_H

#include "LinkedList.h"

// Custom Stack (LIFO) built on linked nodes.
// Real-world use: Activity History - the latest activity is always on top.
template <typename T>
class Stack {
private:
    ListNode<T>* topNode;
    int count;

public:
    Stack() : topNode(nullptr), count(0) {}
    ~Stack() {
        while (topNode) {
            ListNode<T>* t = topNode;
            topNode = topNode->next;
            delete t;
        }
    }
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    void push(const T& value) {
        ListNode<T>* node = new ListNode<T>(value);
        node->next = topNode;
        topNode = node;
        ++count;
    }

    bool pop(T& out) {
        if (!topNode) return false;
        ListNode<T>* t = topNode;
        out = t->data;
        topNode = t->next;
        delete t;
        --count;
        return true;
    }

    bool top(T& out) const {
        if (!topNode) return false;
        out = topNode->data;
        return true;
    }

    bool isEmpty() const { return count == 0; }
    int size() const { return count; }
    ListNode<T>* getTop() const { return topNode; }

    // prints from top (latest) to bottom (oldest). T must support operator<<
    void display(const string& indent = "", int maxItems = -1) const {
        if (!topNode) { cout << indent << "(no activity yet)\n"; return; }
        int i = 1;
        for (ListNode<T>* p = topNode; p && (maxItems < 0 || i <= maxItems); p = p->next, ++i)
            cout << indent << i << ". " << p->data << "\n";
    }
};

#endif
