#ifndef BLOODTREE_H
#define BLOODTREE_H

#include "LinkedList.h"
#include "Queue.h"

// One node of the compatibility tree.
class TreeNode {
public:
    string group;
    LinkedList<TreeNode*> children;

    TreeNode(const string& g) : group(g) {}
    ~TreeNode() {
        for (ListNode<TreeNode*>* p = children.getHead(); p; p = p->next) delete p->data;
    }
    TreeNode(const TreeNode&) = delete;
    TreeNode& operator=(const TreeNode&) = delete;
};

// Blood Compatibility Tree.
//
// The root is O- (universal donor). A child means "can ALSO donate to this group",
// so the whole subtree below a node = every group that node can donate to.
// Because A+, AB+, AB- ... can receive from several groups they appear more than once
// in the tree; every occurrence of a group has exactly the same subtree.
//
//   O-  -> O+, A-, B-
//   O+  -> A+, B+          A- -> A+, AB-        B- -> B+, AB-
//   A+  -> AB+             B+ -> AB+            AB- -> AB+
class BloodTree {
private:
    TreeNode* root;

    // attach 'child' under EVERY occurrence of 'parent' in the tree
    void insertRec(TreeNode* node, const string& parent, const string& child, bool& found) {
        if (node->group == parent) {
            found = true;
            bool exists = false;
            for (ListNode<TreeNode*>* p = node->children.getHead(); p; p = p->next)
                if (p->data->group == child) exists = true;
            if (!exists) node->children.insertBack(new TreeNode(child));
        }
        for (ListNode<TreeNode*>* p = node->children.getHead(); p; p = p->next)
            insertRec(p->data, parent, child, found);
    }

    TreeNode* searchRec(TreeNode* node, const string& group) const {
        if (node->group == group) return node;
        for (ListNode<TreeNode*>* p = node->children.getHead(); p; p = p->next) {
            TreeNode* r = searchRec(p->data, group);
            if (r) return r;
        }
        return nullptr;
    }

    void markRec(TreeNode* node, bool* flags) const {
        flags[groupIndex(node->group)] = true;
        for (ListNode<TreeNode*>* p = node->children.getHead(); p; p = p->next)
            markRec(p->data, flags);
    }

    void preorderRec(TreeNode* node, int depth, const string& indent) const {
        cout << indent << string(depth * 4, ' ') << (depth ? "+-- " : "") << node->group << "\n";
        for (ListNode<TreeNode*>* p = node->children.getHead(); p; p = p->next)
            preorderRec(p->data, depth + 1, indent);
    }

    int countRec(TreeNode* node) const {
        int c = 1;
        for (ListNode<TreeNode*>* p = node->children.getHead(); p; p = p->next)
            c += countRec(p->data);
        return c;
    }

    void buildHierarchy() {
        insert("O-", "O+");  insert("O-", "A-");  insert("O-", "B-");
        insert("O+", "A+");  insert("O+", "B+");
        insert("A-", "A+");  insert("A-", "AB-");
        insert("B-", "B+");  insert("B-", "AB-");
        insert("A+", "AB+"); insert("B+", "AB+"); insert("AB-", "AB+");
    }

public:
    static constexpr int TOTAL = 8;

    BloodTree() : root(new TreeNode("O-")) { buildHierarchy(); }
    ~BloodTree() { delete root; }
    BloodTree(const BloodTree&) = delete;
    BloodTree& operator=(const BloodTree&) = delete;

    // fixed display order used everywhere in the project
    static const string& groupAt(int i) {
        static const string g[TOTAL] = {"O-", "O+", "A-", "A+", "B-", "B+", "AB-", "AB+"};
        return g[i];
    }
    static int groupIndex(const string& g) {
        for (int i = 0; i < TOTAL; i++) if (groupAt(i) == g) return i;
        return -1;
    }
    static bool isValidGroup(const string& g) { return groupIndex(g) != -1; }

    // ---- Insert
    bool insert(const string& parent, const string& child) {
        bool found = false;
        insertRec(root, parent, child, found);
        return found;
    }

    // ---- Search (preorder, first occurrence)
    TreeNode* search(const string& group) const { return searchRec(root, group); }

    // flags[i] = true if 'donor' can donate to groupAt(i)  (flags must have TOTAL entries)
    void getRecipients(const string& donor, bool* flags) const {
        for (int i = 0; i < TOTAL; i++) flags[i] = false;
        TreeNode* n = search(donor);
        if (n) markRec(n, flags);
    }
    bool canDonate(const string& donor, const string& recipient) const {
        bool flags[TOTAL];
        getRecipients(donor, flags);
        int idx = groupIndex(recipient);
        return idx >= 0 && flags[idx];
    }
    // flags[i] = true if groupAt(i) can donate to 'recipient'
    void getDonors(const string& recipient, bool* flags) const {
        for (int i = 0; i < TOTAL; i++) flags[i] = canDonate(groupAt(i), recipient);
    }

    // ---- Traversals
    void preorder(const string& indent = "") const { preorderRec(root, 0, indent); }

    void levelOrder(const string& indent = "") const {
        Queue<TreeNode*> q;
        q.enqueue(root);
        int level = 0;
        while (!q.isEmpty()) {
            int sz = q.size();
            cout << indent << "Level " << level++ << " : ";
            for (int i = 0; i < sz; i++) {
                TreeNode* cur = nullptr;
                q.dequeue(cur);
                cout << cur->group << "  ";
                for (ListNode<TreeNode*>* p = cur->children.getHead(); p; p = p->next)
                    q.enqueue(p->data);
            }
            cout << "\n";
        }
    }

    int nodeCount() const { return countRec(root); }
};

#endif
