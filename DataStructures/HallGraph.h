#ifndef HALLGRAPH_H
#define HALLGRAPH_H

#include "LinkedList.h"
#include "Queue.h"
#include "Stack.h"

// One edge of the graph: destination + distance + travel time.
class GraphEdge {
public:
    int to;
    int meters;
    int minutes;
    GraphEdge(int t = 0, int m = 0, int mi = 0) : to(t), meters(m), minutes(mi) {}
};

// Result of a BFS nearest-donor search.
class NearestResult {
public:
    bool found;
    int node;
    int hops;
    int minutes;
    int meters;
    string path;    // e.g. "KUET Medical Center -> Rokeya Hall"
    string trace;   // BFS log, one line per visited hall
    NearestResult() : found(false), node(-1), hops(0), minutes(0), meters(0) {}
};

// KUET Hall network: undirected weighted graph stored as adjacency lists.
class HallGraph {
private:
    static constexpr int MAX_NODES = 20;
    string names[MAX_NODES];
    LinkedList<GraphEdge> adj[MAX_NODES];
    int n;

public:
    HallGraph() : n(0) {}

    static const string& medicalCenterName() {
        static const string s = "KUET Medical Center";
        return s;
    }

    int addNode(const string& name) {
        int idx = findNode(name);
        if (idx != -1) return idx;
        if (n >= MAX_NODES) return -1;
        names[n] = name;
        return n++;
    }
    int findNode(const string& name) const {
        for (int i = 0; i < n; i++) if (names[i] == name) return i;
        return -1;
    }
    bool addEdge(int a, int b, int meters, int minutes) {
        if (a < 0 || b < 0 || a >= n || b >= n || a == b) return false;
        adj[a].insertBack(GraphEdge(b, meters, minutes));
        adj[b].insertBack(GraphEdge(a, meters, minutes));
        return true;
    }
    void clear() {
        for (int i = 0; i < n; i++) { adj[i].clear(); names[i].clear(); }
        n = 0;
    }
    int nodeCount() const { return n; }
    const string& nameOf(int i) const { return names[i]; }
    LinkedList<GraphEdge>& neighbors(int i) { return adj[i]; }

    // ---- BFS: nearest hall that has a suitable donor.
    // hasDonor(int node) -> bool tells whether a hall has a matching available donor.
    // BFS visits halls level by level (fewest hops first). Among halls of the same
    // level the one with the smaller travel time is chosen.
    template <typename F>
    NearestResult bfsNearest(int start, F hasDonor) const {
        NearestResult res;
        if (start < 0 || start >= n) return res;

        bool visited[MAX_NODES];
        int level[MAX_NODES], parent[MAX_NODES], minutes[MAX_NODES], meters[MAX_NODES];
        for (int i = 0; i < MAX_NODES; i++) {
            visited[i] = false; level[i] = -1; parent[i] = -1; minutes[i] = 0; meters[i] = 0;
        }

        Queue<int> q;
        q.enqueue(start);
        visited[start] = true;
        level[start] = 0;
        int best = -1;

        while (!q.isEmpty()) {
            int u = -1;
            q.dequeue(u);
            if (best != -1 && level[u] > level[best]) break;   // nearer level already had a donor

            if (u != start) {
                bool ok = hasDonor(u);
                res.trace += "Level " + to_string(level[u]) + ": " + names[u] +
                             (ok ? "  -> donor found" : "  -> no matching donor") + "\n";
                if (ok && (best == -1 || minutes[u] < minutes[best])) best = u;
            }
            for (ListNode<GraphEdge>* p = adj[u].getHead(); p; p = p->next) {
                int v = p->data.to;
                if (!visited[v]) {
                    visited[v] = true;
                    level[v] = level[u] + 1;
                    parent[v] = u;
                    minutes[v] = minutes[u] + p->data.minutes;
                    meters[v] = meters[u] + p->data.meters;
                    q.enqueue(v);
                }
            }
        }

        if (best == -1) return res;
        res.found = true;
        res.node = best;
        res.hops = level[best];
        res.minutes = minutes[best];
        res.meters = meters[best];

        // rebuild the path start -> best using a Stack (parents are stored backwards)
        Stack<int> st;
        for (int v = best; v != -1; v = parent[v]) st.push(v);
        int x;
        bool first = true;
        while (st.pop(x)) {
            if (!first) res.path += " -> ";
            res.path += names[x];
            first = false;
        }
        return res;
    }

    void display(const string& indent = "") {
        for (int i = 0; i < n; i++) {
            cout << indent << names[i] << "\n";
            for (ListNode<GraphEdge>* p = adj[i].getHead(); p; p = p->next)
                cout << indent << "     -> " << names[p->data.to] << "  ("
                     << p->data.minutes << " min, " << p->data.meters << " m)\n";
        }
    }
};

#endif
