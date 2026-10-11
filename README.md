# KUET LifeFlow — Smart Blood Bank Management System

Modern Blood Bank Management System for Khulna University of Engineering & Technology (KUET).
Connecting donors, medical centers, and emergency patients across campus through intelligent blood matching and real-time dispatch.

---

## 🌟 How it Works (Full-Stack Architecture)

1. **C++ Backend (`Server/HttpServer.cpp`, `Core/BloodBankSystem.cpp`)**:
   - Runs a lightweight embedded HTTP/REST API web server using Winsock (`-lws2_32`).
   - Handles all business logic, authentication, emergency queue scheduling, graph routing, and file persistence.
   - Preserves all core DSA implementations (Linked List, Stack, Queue, BloodTree, HallGraph BFS, MergeSort).
2. **Frontend UI (`web/index.html`, `web/css/style.css`, `web/js/app.js`)**:
   - Modern Glassmorphism & Crimson Dark/Light Theme with Google Fonts (`Outfit`, `Inter`, `JetBrains Mono`).
   - Dynamic real-time dashboard, blood stock counters, interactive donor directory with direct call/WhatsApp integration.
   - Interactive BFS Nearest Donor finder across KUET Halls & Medical Center.
   - Live Emergency FIFO Queue tracking and Admin Command Center.

---

## 🚀 How to Run

### Option 1: Quick Launch on Windows (Double-Click)
- Double-click **`run_web.bat`** to launch the Web Frontend (automatically opens `http://localhost:8080` in your browser).
- Double-click **`run_terminal.bat`** to run in Terminal Console mode.

### Option 2: Build & Run from Command Line
```bash
# Build the project
make

# Launch Web Server & UI (Default port 8080)
./KUET_BloodBank.exe --web

# Or run terminal menu
./KUET_BloodBank.exe
```

---

## 🔐 Login Credentials (Sample Data)

| Role | Username / Phone | Password | Features |
|---|---|---|---|
| **Admin** | `admin` | `admin123` | Queue processor, inventory stock manager, donor control, MergeSort reports |
| **Users** | `01711111111` to `01788888888` | `user123` | Profile management, donor registration, blood request tracking |

---

## 🧠 DSA Topics Used in the System

| Topic | File(s) | Usage in System |
|---|---|---|
| **Class (OOP)** | Everywhere | Encapsulation of all models, data structures, and services |
| **Linked List** | `DataStructures/LinkedList.h` | Storing users, donors, donations, blood requests, and graph edges |
| **Stack (LIFO)** | `DataStructures/Stack.h` | Real-time system activity history & BFS shortest path reconstruction |
| **Queue (FIFO)** | `DataStructures/Queue.h` | Emergency Priority Queue & Normal Request Queue scheduling |
| **Map** | `std::map` in `BloodBankSystem` | Blood stock inventory tracking (`group -> units`) |
| **Tree** | `DataStructures/BloodTree.h` | Blood compatibility hierarchy rooted at `O-` (Universal Donor) |
| **Graph** | `DataStructures/HallGraph.h` | KUET Medical Center + campus halls weighted graph |
| **BFS Algorithm**| `HallGraph::bfsNearest` | Shortest path & nearest available donor routing |
| **Merge Sort** | `Algorithms/MergeSort.*` | O(N log N) stable date-based sorting for reports & logs |
| **Searching** | `Algorithms/Search.*` | Hand-written linear and inventory search algorithms |
| **File Handling**| `Database/FileManager.*` | Text file database persistence in `Data/*.txt` |

---

## 🌐 REST API Endpoints

- `POST /api/login` - User and Admin authentication
- `POST /api/register` - New user registration
- `GET  /api/inventory` - Live blood stock by blood group
- `POST /api/inventory/update` - Admin stock adjustment
- `GET  /api/donors` - Search & filter donors (group, hall, eligibility)
- `POST /api/donors/toggle` - Toggle donor willingness
- `POST /api/donors/record-donation` - Record donation and update vault
- `GET  /api/requests` - Blood requests & queue status
- `POST /api/requests/create` - Submit emergency or regular request
- `POST /api/requests/process-queue` - Process top request in FIFO queue
- `GET  /api/nearest-donor` - BFS campus donor route search
- `GET  /api/compatibility` - BloodTree compatibility matrix
- `GET  /api/reports/donations` - MergeSort sorted donation analytics
- `GET  /api/reports/requests` - MergeSort sorted request analytics
- `GET  /api/reports/summary` - System summary statistics
- `GET  /api/activity` - LIFO Stack activity feed
