/**
 * KUET Smart Blood Bank System - Frontend Application Logic
 * Integrates directly with C++ REST Backend (:8080)
 */

class BloodBankApp {
    constructor() {
        this.apiBase = window.location.origin.includes('localhost') || window.location.origin.includes('127.0.0.1')
            ? window.location.origin
            : 'http://localhost:8080';

        this.state = {
            user: null,
            role: 'guest', // 'guest', 'user', 'admin'
            activeTab: 'dashboard',
            theme: localStorage.getItem('kuet_bb_theme') || 'dark',
            inventory: [],
            halls: [],
            compatibilityMatrix: []
        };
    }

    async init() {
        this.applyTheme(this.state.theme);
        this.setupEventListeners();
        this.checkSavedSession();
        await this.loadHalls();
        await this.loadDashboardData();
        await this.loadCompatibilityMatrix();
        this.populateHallsDropdown();

        // Refresh periodically every 10 seconds
        setInterval(() => {
            if (this.state.activeTab === 'dashboard') {
                this.loadDashboardData(false);
            }
        }, 10000);
    }

    // =========================================================================
    // THEME & NAVIGATION
    // =========================================================================
    applyTheme(theme) {
        this.state.theme = theme;
        document.documentElement.setAttribute('data-theme', theme);
        localStorage.setItem('kuet_bb_theme', theme);
        const icon = document.querySelector('#themeToggle i');
        if (icon) {
            icon.className = theme === 'dark' ? 'fas fa-sun' : 'fas fa-moon';
        }
    }

    toggleTheme() {
        const newTheme = this.state.theme === 'dark' ? 'light' : 'dark';
        this.applyTheme(newTheme);
    }

    showTab(tabId) {
        this.state.activeTab = tabId;
        document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
        document.querySelectorAll('.nav-btn').forEach(el => el.classList.remove('active'));

        const targetTab = document.getElementById(`tab-${tabId}`);
        const targetNav = document.getElementById(`nav-${tabId}`);
        if (targetTab) targetTab.classList.add('active');
        if (targetNav) targetNav.classList.add('active');

        // Trigger specific data loader
        if (tabId === 'dashboard') this.loadDashboardData();
        else if (tabId === 'donors') this.loadDonors();
        else if (tabId === 'requests') this.loadRequests();
        else if (tabId === 'campus-map') this.renderCampusNodes();
        else if (tabId === 'reports') this.loadDonationReport('desc');
        else if (tabId === 'user-profile') this.renderUserProfile();
    }

    setupEventListeners() {
        document.getElementById('themeToggle')?.addEventListener('click', () => this.toggleTheme());
    }

    // =========================================================================
    // API CLIENT HELPER
    // =========================================================================
    async api(endpoint, options = {}) {
        try {
            const url = `${this.apiBase}${endpoint}`;
            const res = await fetch(url, {
                headers: {
                    'Content-Type': 'application/json',
                    ...(options.headers || {})
                },
                ...options
            });
            const data = await res.json();
            return data;
        } catch (err) {
            console.error(`API Error on ${endpoint}:`, err);
            return { success: false, error: 'Network error or C++ server offline' };
        }
    }

    // =========================================================================
    // DATA LOADERS: DASHBOARD & INVENTORY
    // =========================================================================
    async loadDashboardData(showLoading = true) {
        // 1. Load Inventory
        const invRes = await this.api('/api/inventory');
        if (invRes.success) {
            this.state.inventory = invRes.inventory;
            this.renderBloodStockGrid(invRes.inventory);
            document.getElementById('statTotalStock').innerText = invRes.totalStock;
        }

        // 2. Load Stats
        const statsRes = await this.api('/api/reports/summary');
        if (statsRes.success) {
            const s = statsRes.stats;
            document.getElementById('statAvailableDonors').innerText = s.availableDonors;
            document.getElementById('statFulfilledRequests').innerText = s.fulfilledRequests;
            document.getElementById('statEmergencyRequests').innerText = s.emergencyRequests;
        }

        // 3. Load Queues & Pending Requests
        const reqRes = await this.api('/api/requests');
        if (reqRes.success) {
            this.renderDashboardQueues(reqRes.requests);
            const pendingCount = reqRes.requests.filter(r => r.status === 'Pending').length;
            document.getElementById('pendingBadge').innerText = pendingCount;
        }

        // 4. Load Activity Log
        const actRes = await this.api('/api/activity');
        if (actRes.success) {
            this.renderActivityFeed(actRes.activities);
        }
    }

    renderBloodStockGrid(inventory) {
        const grid = document.getElementById('bloodStockGrid');
        if (!grid) return;

        grid.innerHTML = inventory.map(item => {
            let statusClass = 'status-ok';
            if (item.units === 0) statusClass = 'status-empty';
            else if (item.units <= 3) statusClass = 'status-low';

            return `
                <div class="blood-card" onclick="app.showBloodCompatibilityDetail('${item.group}')">
                    <div class="blood-group-tag">${item.group}</div>
                    <div class="blood-units-count">${item.units}</div>
                    <div class="blood-units-label">Units in Vault</div>
                    <span class="blood-status-badge ${statusClass}">${item.status}</span>
                </div>
            `;
        }).join('');
    }

    renderDashboardQueues(requests) {
        const emergencyList = document.getElementById('dashEmergencyList');
        const normalList = document.getElementById('dashNormalList');
        const emergencyCount = document.getElementById('dashEmergencyCount');
        const normalCount = document.getElementById('dashNormalCount');

        const emerg = requests.filter(r => r.status === 'Pending' && r.emergency);
        const norm = requests.filter(r => r.status === 'Pending' && !r.emergency);

        if (emergencyCount) emergencyCount.innerText = emerg.length;
        if (normalCount) normalCount.innerText = norm.length;

        if (emergencyList) {
            emergencyList.innerHTML = emerg.length === 0
                ? '<div class="empty-state-sm"><i class="fas fa-check"></i> No emergency requests</div>'
                : emerg.map((r, idx) => `
                    <div class="queue-item">
                        <div class="queue-item-left">
                            <span class="queue-item-title">#${r.id} ${this.escape(r.patientName)} (${r.bloodGroup} x${r.units})</span>
                            <span class="queue-item-sub"><i class="fas fa-location-dot"></i> ${this.escape(r.location)}</span>
                        </div>
                        <span class="badge badge-emergency">Queue Pos #${idx + 1}</span>
                    </div>
                `).join('');
        }

        if (normalList) {
            normalList.innerHTML = norm.length === 0
                ? '<div class="empty-state-sm"><i class="fas fa-check"></i> No normal requests</div>'
                : norm.map((r, idx) => `
                    <div class="queue-item">
                        <div class="queue-item-left">
                            <span class="queue-item-title">#${r.id} ${this.escape(r.patientName)} (${r.bloodGroup} x${r.units})</span>
                            <span class="queue-item-sub"><i class="fas fa-location-dot"></i> ${this.escape(r.location)}</span>
                        </div>
                        <span class="badge badge-info">Pos #${emerg.length + idx + 1}</span>
                    </div>
                `).join('');
        }
    }

    renderActivityFeed(activities) {
        const feed = document.getElementById('dashboardActivityFeed');
        if (!feed) return;

        feed.innerHTML = activities.slice(0, 10).map(act => `
            <div class="activity-item">${this.escape(act)}</div>
        `).join('');
    }

    // =========================================================================
    // DONOR DIRECTORY
    // =========================================================================
    async loadDonors() {
        const group = document.getElementById('donorGroupFilter')?.value || '';
        const hall = document.getElementById('donorHallFilter')?.value || '';
        const search = document.getElementById('donorSearchInput')?.value || '';
        const available = document.getElementById('donorAvailableOnly')?.checked || false;

        let query = `?group=${encodeURIComponent(group)}&hall=${encodeURIComponent(hall)}&search=${encodeURIComponent(search)}`;
        if (available) query += '&available=true';

        const res = await this.api(`/api/donors${query}`);
        const tbody = document.getElementById('donorsTableBody');
        if (!tbody) return;

        if (res.success && res.donors.length > 0) {
            tbody.innerHTML = res.donors.map(d => {
                const isEligible = d.isEligible;
                const statusBadge = d.isAvailable
                    ? '<span class="badge badge-success"><i class="fas fa-check"></i> Available</span>'
                    : `<span class="badge badge-secondary">${this.escape(d.status)}</span>`;

                const waitText = isEligible
                    ? '<span class="text-success font-bold"><i class="fas fa-check-circle"></i> Eligible Today</span>'
                    : `<span class="text-muted"><i class="fas fa-hourglass-half"></i> Wait ${d.waitDays} days</span>`;

                let actionBtns = `
                    <a href="tel:${d.phone}" class="btn btn-sm btn-outline" title="Call Donor"><i class="fas fa-phone"></i></a>
                    <a href="https://wa.me/88${d.phone}" target="_blank" class="btn btn-sm btn-outline text-success" title="WhatsApp"><i class="fab fa-whatsapp"></i></a>
                `;

                if (this.state.role === 'admin') {
                    actionBtns += `
                        <button class="btn btn-sm btn-secondary" onclick="app.adminToggleDonor('${d.phone}')" title="Toggle Willingness"><i class="fas fa-toggle-on"></i></button>
                        <button class="btn btn-sm btn-outline text-emergency" onclick="app.adminDeleteDonor('${d.phone}')" title="Remove"><i class="fas fa-trash"></i></button>
                    `;
                }

                return `
                    <tr>
                        <td><b>${this.escape(d.name)}</b></td>
                        <td><span class="badge badge-emergency font-bold">${d.bloodGroup}</span></td>
                        <td><i class="fas fa-building text-muted"></i> ${this.escape(d.hall)}</td>
                        <td><span class="font-mono">${d.phone}</span></td>
                        <td>${d.lastDonationDate}</td>
                        <td>${waitText}</td>
                        <td>${statusBadge}</td>
                        <td><div class="btn-group">${actionBtns}</div></td>
                    </tr>
                `;
            }).join('');
        } else {
            tbody.innerHTML = `<tr><td colspan="8" class="text-center text-muted p-4">No donors matching your search criteria.</td></tr>`;
        }
    }

    // =========================================================================
    // BLOOD REQUESTS
    // =========================================================================
    async loadRequests(statusFilter = '') {
        let endpoint = '/api/requests';
        if (statusFilter && statusFilter !== 'all') {
            endpoint += `?status=${encodeURIComponent(statusFilter)}`;
        }

        const res = await this.api(endpoint);
        const tbody = document.getElementById('requestsTableBody');
        if (!tbody) return;

        if (res.success && res.requests.length > 0) {
            tbody.innerHTML = res.requests.map(r => {
                let statusBadge = '<span class="badge badge-warning">Pending</span>';
                if (r.status === 'Fulfilled') statusBadge = '<span class="badge badge-success">Fulfilled</span>';
                else if (r.status === 'Rejected') statusBadge = '<span class="badge badge-secondary">Rejected</span>';

                const typeBadge = r.emergency
                    ? '<span class="badge badge-emergency"><i class="fas fa-ambulance"></i> EMERGENCY</span>'
                    : '<span class="badge badge-info">Regular</span>';

                let actionBtns = '-';
                if (this.state.role === 'admin' && r.status === 'Pending') {
                    actionBtns = `
                        <button class="btn btn-sm btn-primary" onclick="app.adminFulfillRequest(${r.id})"><i class="fas fa-check"></i> Fulfill</button>
                        <button class="btn btn-sm btn-outline text-emergency" onclick="app.adminRejectRequest(${r.id})"><i class="fas fa-times"></i></button>
                    `;
                }

                return `
                    <tr>
                        <td><b>#${r.id}</b></td>
                        <td><b>${this.escape(r.patientName)}</b></td>
                        <td><span class="badge badge-emergency font-bold">${r.bloodGroup}</span></td>
                        <td><b>${r.units}</b> unit(s)</td>
                        <td><i class="fas fa-location-dot text-muted"></i> ${this.escape(r.location)}</td>
                        <td>${typeBadge}</td>
                        <td><span class="font-mono">${r.requestedBy}</span></td>
                        <td>${r.date}</td>
                        <td>${statusBadge}</td>
                        <td>${actionBtns}</td>
                    </tr>
                `;
            }).join('');
        } else {
            tbody.innerHTML = `<tr><td colspan="10" class="text-center text-muted p-4">No blood requests found.</td></tr>`;
        }
    }

    filterRequests(status) {
        document.querySelectorAll('.filter-pills .pill-btn').forEach(btn => btn.classList.remove('active'));
        event?.target?.classList.add('active');
        this.loadRequests(status);
    }

    // =========================================================================
    // BFS NEAREST DONOR SEARCH (GRAPH + BFS)
    // =========================================================================
    async runBfsSearch() {
        const start = document.getElementById('bfsStartLocation').value;
        const group = document.getElementById('bfsBloodGroup').value;

        const res = await this.api(`/api/nearest-donor?start=${encodeURIComponent(start)}&group=${encodeURIComponent(group)}`);
        const container = document.getElementById('bfsResultsContainer');
        const summaryCard = document.getElementById('bfsSummaryCard');
        const traceOutput = document.getElementById('bfsTraceOutput');
        const donorsList = document.getElementById('bfsDonorsList');

        if (!container) return;
        container.style.display = 'block';

        if (res.success && res.found) {
            summaryCard.innerHTML = `
                <div class="flex-between">
                    <div>
                        <h3 class="text-success"><i class="fas fa-check-circle"></i> Nearest Available Donor Found</h3>
                        <p class="mt-1">Optimal Route: <b class="text-primary">${this.escape(res.path)}</b></p>
                    </div>
                    <span class="badge badge-success" style="font-size: 1rem; padding: 8px 16px;">
                        <i class="fas fa-clock"></i> ${res.minutes} Minutes • ${res.meters} Meters
                    </span>
                </div>
            `;

            traceOutput.innerText = res.trace;

            if (res.donors && res.donors.length > 0) {
                donorsList.innerHTML = res.donors.map(d => `
                    <div class="stat-box mt-2">
                        <div class="stat-icon icon-donors"><i class="fas fa-user"></i></div>
                        <div class="stat-data">
                            <h4>${this.escape(d.name)} <span class="badge badge-emergency">${d.bloodGroup}</span> ${d.exactMatch ? '<span class="badge badge-success">Exact Match</span>' : '<span class="badge badge-info">Compatible</span>'}</h4>
                            <p class="font-mono mt-1"><i class="fas fa-phone"></i> ${d.phone}</p>
                        </div>
                        <div class="btn-group">
                            <a href="tel:${d.phone}" class="btn btn-sm btn-primary"><i class="fas fa-phone"></i> Call</a>
                        </div>
                    </div>
                `).join('');
            } else {
                donorsList.innerHTML = '<p class="text-muted">No donor details available.</p>';
            }
        } else {
            summaryCard.innerHTML = `
                <h3 class="text-emergency"><i class="fas fa-circle-xmark"></i> No Available Donor Found</h3>
                <p class="mt-1">All halls in the KUET campus network were searched, but no available compatible donor was found for blood group <b>${group}</b>.</p>
            `;
            traceOutput.innerText = res.trace || 'Campus Search Completed.';
            donorsList.innerHTML = '<p class="text-muted">Try submitting an emergency blood request through the request portal.</p>';
        }
    }

    async loadHalls() {
        const res = await this.api('/api/halls');
        if (res.success) {
            this.state.halls = res.halls;
        }
    }

    populateHallsDropdown() {
        const filterSelect = document.getElementById('donorHallFilter');
        if (filterSelect && this.state.halls.length > 0) {
            filterSelect.innerHTML = '<option value="">All Halls / Locations</option>' +
                this.state.halls.map(h => `<option value="${h.name}">${h.name}</option>`).join('');
        }
    }

    renderCampusNodes() {
        const grid = document.getElementById('campusNodesGrid');
        if (!grid || this.state.halls.length === 0) return;

        grid.innerHTML = this.state.halls.map(h => `
            <div class="node-card">
                <h4><i class="fas fa-building text-primary"></i> ${this.escape(h.name)}</h4>
                <p class="text-muted" style="font-size: 0.8rem;">Node ID: #${h.id} • Connected in KUET HallGraph</p>
            </div>
        `).join('');
    }

    // =========================================================================
    // BLOOD TREE & COMPATIBILITY
    // =========================================================================
    async loadCompatibilityMatrix() {
        const res = await this.api('/api/compatibility');
        if (res.success) {
            this.state.compatibilityMatrix = res.matrix;
            const tbody = document.getElementById('compatibilityMatrixBody');
            if (tbody) {
                tbody.innerHTML = res.matrix.map(m => `
                    <tr>
                        <td><span class="badge badge-emergency" style="font-size: 0.95rem;">${m.group}</span></td>
                        <td>${m.canDonateTo.map(g => `<span class="badge badge-success">${g}</span>`).join(' ')}</td>
                        <td>${m.canReceiveFrom.map(g => `<span class="badge badge-info">${g}</span>`).join(' ')}</td>
                    </tr>
                `).join('');
            }
        }
    }

    checkQuickCompatibility() {
        const donor = document.getElementById('quickDonorGroup').value;
        const patient = document.getElementById('quickPatientGroup').value;
        const resultDiv = document.getElementById('quickResult');

        const donorInfo = this.state.compatibilityMatrix.find(m => m.group === donor);
        const canDonate = donorInfo && donorInfo.canDonateTo.includes(patient);

        if (canDonate) {
            resultDiv.innerHTML = `
                <div class="alert alert-success mt-2">
                    <i class="fas fa-circle-check"></i> <b>Compatible!</b> A donor with <b>${donor}</b> CAN safely donate to a patient with <b>${patient}</b> according to the BloodTree hierarchy.
                </div>
            `;
        } else {
            resultDiv.innerHTML = `
                <div class="alert alert-danger mt-2">
                    <i class="fas fa-circle-xmark"></i> <b>Incompatible!</b> A donor with <b>${donor}</b> CANNOT donate to a patient with <b>${patient}</b>.
                </div>
            `;
        }
    }

    showBloodCompatibilityDetail(group) {
        this.showTab('compatibility');
        this.showToast(`Showing Blood Compatibility Guide for ${group}`, 'info');
    }

    // =========================================================================
    // DSA MERGESORT REPORTS
    // =========================================================================
    switchReportTab(type) {
        document.querySelectorAll('.reports-nav .tab-pill').forEach(p => p.classList.remove('active'));
        event?.target?.classList.add('active');

        document.getElementById('reportDonationsView').style.display = type === 'donations' ? 'block' : 'none';
        document.getElementById('reportRequestsView').style.display = type === 'requests' ? 'block' : 'none';
        document.getElementById('reportSummaryView').style.display = type === 'summary' ? 'block' : 'none';

        if (type === 'donations') this.loadDonationReport();
        else if (type === 'requests') this.loadRequestReport();
        else if (type === 'summary') this.loadSummaryReport();
    }

    async loadDonationReport(order = 'desc') {
        const res = await this.api(`/api/reports/donations?order=${order}`);
        const tbody = document.getElementById('reportDonationsBody');
        if (!tbody) return;

        if (res.success && res.donations.length > 0) {
            tbody.innerHTML = res.donations.map(d => `
                <tr>
                    <td><b>#${d.id}</b></td>
                    <td>${this.escape(d.donorName)}</td>
                    <td><span class="font-mono">${d.donorPhone}</span></td>
                    <td><span class="badge badge-emergency font-bold">${d.bloodGroup}</span></td>
                    <td><b>${d.units}</b> unit(s)</td>
                    <td><i class="fas fa-calendar-alt text-primary"></i> ${d.date}</td>
                </tr>
            `).join('');
        } else {
            tbody.innerHTML = '<tr><td colspan="6" class="text-center text-muted p-4">No donation records found.</td></tr>';
        }
    }

    async loadRequestReport(order = 'desc') {
        const res = await this.api(`/api/reports/requests?order=${order}`);
        const tbody = document.getElementById('reportRequestsBody');
        if (!tbody) return;

        if (res.success && res.requests.length > 0) {
            tbody.innerHTML = res.requests.map(r => `
                <tr>
                    <td><b>#${r.id}</b></td>
                    <td>${this.escape(r.patientName)}</td>
                    <td><span class="badge badge-emergency">${r.bloodGroup}</span></td>
                    <td>${r.units}</td>
                    <td>${this.escape(r.location)}</td>
                    <td>${r.emergency ? '<span class="badge badge-emergency">YES</span>' : 'No'}</td>
                    <td>${r.date}</td>
                    <td><span class="badge badge-info">${r.status}</span></td>
                </tr>
            `).join('');
        } else {
            tbody.innerHTML = '<tr><td colspan="8" class="text-center text-muted p-4">No requests found.</td></tr>';
        }
    }

    async loadSummaryReport() {
        const res = await this.api('/api/reports/summary');
        const grid = document.getElementById('summaryReportGrid');
        if (!grid || !res.success) return;

        const s = res.stats;
        grid.innerHTML = `
            <div class="admin-grid">
                <div class="stat-box"><div class="stat-data"><h3>${s.totalUsers}</h3><p>Total Registered Users</p></div></div>
                <div class="stat-box"><div class="stat-data"><h3>${s.totalDonors}</h3><p>Registered Donors</p></div></div>
                <div class="stat-box"><div class="stat-data"><h3>${s.availableDonors}</h3><p>Available Donors Right Now</p></div></div>
                <div class="stat-box"><div class="stat-data"><h3>${s.totalUnitsDonated}</h3><p>Units Ever Donated</p></div></div>
                <div class="stat-box"><div class="stat-data"><h3>${s.totalStock}</h3><p>Current Units in Vault</p></div></div>
                <div class="stat-box"><div class="stat-data"><h3>${s.totalRequests}</h3><p>Total Blood Requests</p></div></div>
            </div>
        `;
    }

    // =========================================================================
    // AUTHENTICATION & SESSIONS
    // =========================================================================
    toggleRoleFields() {
        const isUser = document.querySelector('input[name="loginRole"]:checked').value === 'user';
        const label = document.getElementById('loginUsernameLabel');
        const input = document.getElementById('loginUsername');
        if (label && input) {
            label.innerText = isUser ? 'Phone Number:' : 'Admin ID:';
            input.placeholder = isUser ? '017xxxxxxxx' : 'admin';
        }
    }

    openLoginModal() { this.openModal('loginModal'); }
    openRegisterModal() { this.openModal('registerModal'); }

    async handleLogin(e) {
        e.preventDefault();
        const role = document.querySelector('input[name="loginRole"]:checked').value;
        const username = document.getElementById('loginUsername').value.trim();
        const password = document.getElementById('loginPassword').value;

        const res = await this.api('/api/login', {
            method: 'POST',
            body: JSON.stringify({ role, username, password })
        });

        if (res.success) {
            this.state.role = res.role;
            if (res.role === 'admin') {
                this.state.user = { role: 'admin', adminId: res.adminId };
                this.showToast('Logged in successfully as Admin', 'success');
                this.updateAuthNav();
                this.closeModal('loginModal');
                this.showTab('admin');
            } else {
                this.state.user = res.user;
                this.showToast(`Welcome back, ${res.user.name}!`, 'success');
                this.updateAuthNav();
                this.closeModal('loginModal');
                this.showTab('user-profile');
            }
            localStorage.setItem('kuet_bb_session', JSON.stringify({ role: this.state.role, user: this.state.user }));
        } else {
            this.showToast(res.error || 'Login failed. Check credentials.', 'error');
        }
    }

    async handleRegister(e) {
        e.preventDefault();
        const pw = document.getElementById('regPassword').value;
        const confirmPw = document.getElementById('regConfirmPassword').value;

        if (pw !== confirmPw) {
            this.showToast('Passwords do not match', 'error');
            return;
        }

        const body = {
            name: document.getElementById('regName').value.trim(),
            phone: document.getElementById('regPhone').value.trim(),
            age: parseInt(document.getElementById('regAge').value),
            gender: document.getElementById('regGender').value,
            bloodGroup: document.getElementById('regBloodGroup').value,
            hall: document.getElementById('regHall').value,
            address: document.getElementById('regAddress').value.trim(),
            password: pw
        };

        const res = await this.api('/api/register', {
            method: 'POST',
            body: JSON.stringify(body)
        });

        if (res.success) {
            this.showToast('Registration successful! Please log in.', 'success');
            this.closeModal('registerModal');
            this.openLoginModal();
        } else {
            this.showToast(res.error || 'Registration failed', 'error');
        }
    }

    checkSavedSession() {
        const saved = localStorage.getItem('kuet_bb_session');
        if (saved) {
            try {
                const sess = JSON.parse(saved);
                this.state.role = sess.role;
                this.state.user = sess.user;
                this.updateAuthNav();
            } catch (err) {
                localStorage.removeItem('kuet_bb_session');
            }
        }
    }

    logout() {
        this.state.role = 'guest';
        this.state.user = null;
        localStorage.removeItem('kuet_bb_session');
        this.updateAuthNav();
        this.showTab('dashboard');
        this.showToast('Logged out successfully', 'info');
    }

    updateAuthNav() {
        const container = document.getElementById('authNavContainer');
        if (!container) return;

        if (this.state.role === 'admin') {
            container.innerHTML = `
                <button class="btn btn-emergency btn-sm" onclick="app.showTab('admin')">
                    <i class="fas fa-shield-halved"></i> Admin Center
                </button>
                <button class="btn btn-outline btn-sm" onclick="app.logout()">
                    <i class="fas fa-power-off"></i>
                </button>
            `;
        } else if (this.state.role === 'user' && this.state.user) {
            container.innerHTML = `
                <button class="btn btn-secondary btn-sm" onclick="app.showTab('user-profile')">
                    <i class="fas fa-user-circle"></i> ${this.escape(this.state.user.name)}
                </button>
                <button class="btn btn-outline btn-sm" onclick="app.logout()">
                    <i class="fas fa-sign-out-alt"></i>
                </button>
            `;
        } else {
            container.innerHTML = `
                <button class="btn btn-outline" onclick="app.openLoginModal()">
                    <i class="fas fa-sign-in-alt"></i> Login
                </button>
                <button class="btn btn-primary" onclick="app.openRegisterModal()">
                    <i class="fas fa-user-plus"></i> Register
                </button>
            `;
        }
    }

    // =========================================================================
    // BLOOD REQUESTS & USER OPERATIONS
    // =========================================================================
    openRequestModal(emergency = false) {
        const chk = document.getElementById('reqEmergencyCheck');
        const title = document.getElementById('requestModalTitle');
        const phone = document.getElementById('reqContactPhone');

        if (chk) chk.checked = emergency;
        if (title) {
            title.innerHTML = emergency
                ? '<i class="fas fa-ambulance text-emergency"></i> Emergency Blood Request'
                : '<i class="fas fa-hand-holding-medical text-primary"></i> Request Blood';
        }
        if (phone && this.state.user?.phone) {
            phone.value = this.state.user.phone;
        }
        this.openModal('requestModal');
    }

    async handleCreateRequest(e) {
        e.preventDefault();
        const body = {
            patientName: document.getElementById('reqPatientName').value.trim(),
            bloodGroup: document.getElementById('reqBloodGroup').value,
            units: parseInt(document.getElementById('reqUnits').value),
            location: document.getElementById('reqLocation').value,
            emergency: document.getElementById('reqEmergencyCheck').checked,
            requestedBy: document.getElementById('reqContactPhone').value.trim()
        };

        const res = await this.api('/api/requests/create', {
            method: 'POST',
            body: JSON.stringify(body)
        });

        if (res.success) {
            this.showToast(`Request #${res.requestId} submitted! Queue Position: #${res.queuePosition}`, 'success');
            this.closeModal('requestModal');
            document.getElementById('bloodRequestForm').reset();
            this.loadDashboardData();
            this.loadRequests();
        } else {
            this.showToast(res.error || 'Failed to submit request', 'error');
        }
    }

    handleBecomeDonorClick() {
        if (this.state.role !== 'user') {
            this.showToast('Please login to register as a donor', 'info');
            this.openLoginModal();
            return;
        }
        document.getElementById('bdPhone').value = this.state.user.phone;
        this.openModal('becomeDonorModal');
    }

    async handleBecomeDonorSubmit(e) {
        e.preventDefault();
        const phone = document.getElementById('bdPhone').value;
        const lastDonationDate = document.getElementById('bdLastDate').value.trim() || 'N/A';

        const res = await this.api('/api/user/become-donor', {
            method: 'POST',
            body: JSON.stringify({ phone, lastDonationDate })
        });

        if (res.success) {
            this.showToast('Congratulations! You are now a registered donor.', 'success');
            this.closeModal('becomeDonorModal');
            this.loadDonors();
            this.renderUserProfile();
        } else {
            this.showToast(res.error || 'Failed to become donor', 'error');
        }
    }

    // =========================================================================
    // ADMIN ACTIONS
    // =========================================================================
    async adminProcessQueue() {
        const res = await this.api('/api/requests/process-queue', { method: 'POST' });
        if (res.success) {
            this.showToast(res.message, 'success');
            this.loadDashboardData();
            this.loadRequests();
        } else {
            this.showToast(res.message || 'Queue empty or insufficient stock', 'error');
        }
    }

    async adminFulfillRequest(id) {
        if (!confirm(`Fulfill Request #${id} and issue blood from inventory?`)) return;
        const res = await this.api('/api/requests/fulfill', {
            method: 'POST',
            body: JSON.stringify({ id })
        });
        if (res.success) {
            this.showToast(res.message, 'success');
            this.loadRequests();
            this.loadDashboardData();
        } else {
            this.showToast(res.error, 'error');
        }
    }

    async adminRejectRequest(id) {
        if (!confirm(`Reject Request #${id}?`)) return;
        const res = await this.api('/api/requests/reject', {
            method: 'POST',
            body: JSON.stringify({ id })
        });
        if (res.success) {
            this.showToast(res.message, 'info');
            this.loadRequests();
        } else {
            this.showToast(res.error, 'error');
        }
    }

    async adminToggleDonor(phone) {
        const res = await this.api('/api/donors/toggle', {
            method: 'POST',
            body: JSON.stringify({ phone })
        });
        if (res.success) {
            this.showToast(`Donor status changed to: ${res.status}`, 'info');
            this.loadDonors();
        }
    }

    async adminDeleteDonor(phone) {
        if (!confirm(`Are you sure you want to remove donor with phone ${phone}?`)) return;
        const res = await this.api('/api/donors/delete', {
            method: 'POST',
            body: JSON.stringify({ phone })
        });
        if (res.success) {
            this.showToast(res.message, 'success');
            this.loadDonors();
        }
    }

    async handleInventoryUpdate(e) {
        e.preventDefault();
        const group = document.getElementById('adminStockGroup').value;
        const action = document.getElementById('adminStockAction').value;
        const units = parseInt(document.getElementById('adminStockUnits').value);

        const res = await this.api('/api/inventory/update', {
            method: 'POST',
            body: JSON.stringify({ group, action, units })
        });

        if (res.success) {
            this.showToast(`Inventory updated for ${group}`, 'success');
            this.loadDashboardData();
        } else {
            this.showToast(res.error || 'Inventory update failed', 'error');
        }
    }

    async handleRecordDonation(e) {
        e.preventDefault();
        const phone = document.getElementById('adminRecordPhone').value.trim();
        const units = parseInt(document.getElementById('adminRecordUnits').value);
        const date = document.getElementById('adminRecordDate').value.trim();

        const res = await this.api('/api/donors/record-donation', {
            method: 'POST',
            body: JSON.stringify({ phone, units, date })
        });

        if (res.success) {
            this.showToast(res.message, 'success');
            document.getElementById('adminRecordDonationForm').reset();
            this.loadDashboardData();
            this.loadDonors();
        } else {
            this.showToast(res.error || 'Failed to record donation', 'error');
        }
    }

    async handleChangePassword(e) {
        e.preventDefault();
        const oldPassword = document.getElementById('adminOldPass').value;
        const newPassword = document.getElementById('adminNewPass').value;

        const res = await this.api('/api/admin/change-password', {
            method: 'POST',
            body: JSON.stringify({ oldPassword, newPassword })
        });

        if (res.success) {
            this.showToast('Admin password changed successfully', 'success');
            document.getElementById('adminPasswordForm').reset();
        } else {
            this.showToast(res.error || 'Password update failed', 'error');
        }
    }

    // =========================================================================
    // USER PROFILE VIEW
    // =========================================================================
    async renderUserProfile() {
        if (!this.state.user || this.state.role !== 'user') return;
        const res = await this.api(`/api/user/profile?phone=${this.state.user.phone}`);
        if (!res.success) return;

        const u = res.user;
        const detailsContainer = document.getElementById('userProfileDetails');
        if (detailsContainer) {
            detailsContainer.innerHTML = `
                <div class="stat-box mb-3">
                    <div class="stat-icon icon-donors"><i class="fas fa-user-shield"></i></div>
                    <div class="stat-data">
                        <h3>${this.escape(u.name)}</h3>
                        <p>${u.gender} • ${u.age} Years Old • ${this.escape(u.hall)}</p>
                    </div>
                </div>
                <div class="profile-info-list">
                    <p><b>Blood Group:</b> <span class="badge badge-emergency font-bold">${u.bloodGroup}</span></p>
                    <p><b>Phone Number:</b> <span class="font-mono">${u.phone}</span></p>
                    <p><b>Address:</b> ${this.escape(u.address)}</p>
                    <p><b>Donor Status:</b> <span class="badge ${u.isDonor ? 'badge-success' : 'badge-secondary'}">${u.donorStatus}</span></p>
                    <p><b>Last Donated:</b> ${u.lastDonationDate}</p>
                </div>
                ${!u.isDonor ? `
                    <button class="btn btn-primary w-100 mt-3" onclick="app.handleBecomeDonorClick()">
                        <i class="fas fa-plus-circle"></i> Register as a Blood Donor
                    </button>
                ` : ''}
            `;
        }

        // Fetch User Personal History
        const histContainer = document.getElementById('userPersonalHistory');
        const donRes = await this.api(`/api/reports/donations?phone=${u.phone}`);
        const reqRes = await this.api(`/api/requests?phone=${u.phone}`);

        if (histContainer) {
            let donHtml = '<h4>My Donations</h4>';
            if (donRes.success && donRes.donations.length > 0) {
                donHtml += donRes.donations.map(d => `
                    <div class="activity-item mt-1">Donated ${d.units} unit(s) of ${d.bloodGroup} on ${d.date}</div>
                `).join('');
            } else {
                donHtml += '<p class="text-muted" style="font-size:0.85rem;">No donations recorded yet.</p>';
            }

            let reqHtml = '<h4 class="mt-3">My Blood Requests</h4>';
            if (reqRes.success && reqRes.requests.length > 0) {
                reqHtml += reqRes.requests.map(r => `
                    <div class="activity-item mt-1">Request #${r.id} for ${this.escape(r.patientName)} (${r.bloodGroup} x${r.units}) - Status: <b>${r.status}</b></div>
                `).join('');
            } else {
                reqHtml += '<p class="text-muted" style="font-size:0.85rem;">No blood requests made yet.</p>';
            }

            histContainer.innerHTML = donHtml + reqHtml;
        }
    }

    // =========================================================================
    // UI HELPERS & MODALS
    // =========================================================================
    openModal(id) {
        const m = document.getElementById(id);
        if (m) m.classList.add('active');
    }

    closeModal(id) {
        const m = document.getElementById(id);
        if (m) m.classList.remove('active');
    }

    showToast(message, type = 'info') {
        const container = document.getElementById('toastContainer');
        if (!container) return;

        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        let icon = 'fa-info-circle text-info';
        if (type === 'success') icon = 'fa-check-circle text-success';
        if (type === 'error') icon = 'fa-triangle-exclamation text-emergency';

        toast.innerHTML = `<i class="fas ${icon}"></i> <span>${this.escape(message)}</span>`;
        container.appendChild(toast);

        setTimeout(() => {
            toast.style.opacity = '0';
            toast.style.transform = 'translateX(100%)';
            setTimeout(() => toast.remove(), 300);
        }, 4000);
    }

    escape(str) {
        if (!str) return '';
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#039;');
    }
}

// Global Application Instance
const app = new BloodBankApp();
document.addEventListener('DOMContentLoaded', () => app.init());
