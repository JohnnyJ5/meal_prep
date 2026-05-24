// Renders the shared sidebar/topbar chrome at the top of each page.
// Each page's <body> includes <script src="/shared/chrome.js"></script>
// and then calls `mountChrome({ activePage, crumbLabel, weekLabel })`.
//
// activePage is one of: 'planner', 'workouts'.

(function () {
    const PAGES = [
        { id: 'planner', section: 'Plan', label: 'Meal Prep', href: '/planner', icon: '▤' },
        { id: 'workouts', section: 'Train', label: 'Workouts', href: '/workouts', icon: '◆' },
    ];

    function buildSidebar(activeId) {
        const sections = {};
        for (const p of PAGES) {
            (sections[p.section] = sections[p.section] || []).push(p);
        }

        let html = `
            <div class="ws">
                <div class="ws-ico">M</div>
                <div>
                    <div class="ws-name">Meal Prep</div>
                    <div class="ws-sub">Meal Prep</div>
                </div>
            </div>`;

        for (const sectionName of Object.keys(sections)) {
            html += `<div class="nav-section">${sectionName}</div><nav class="sidebar-nav">`;
            for (const p of sections[sectionName]) {
                const cls = 'nav-item' + (p.id === activeId ? ' active' : '');
                html += `<a class="${cls}" href="${p.href}"><span class="nav-ico">${p.icon}</span><span>${p.label}</span></a>`;
            }
            html += `</nav>`;
        }

        html += `
            <div class="nav-section">Integrations</div>
            <a class="nav-item nav-item-stack" href="/auth/google" id="sidebar-calendar-link">
                <span class="nav-ico">↗</span>
                <span class="nav-item-body">
                    <span>Google Calendar</span>
                    <span class="nav-item-meta" id="sidebar-calendar-meta">Checking…</span>
                </span>
                <span class="nav-status-dot" id="sidebar-calendar-dot" aria-hidden="true"></span>
            </a>
            <div class="sidebar-spacer"></div>`;
        return html;
    }

    function buildTopbar(crumbLabel, weekLabel) {
        const week = weekLabel
            ? `<span class="sep">/</span><span id="week-label">${weekLabel}</span>`
            : '';
        return `
            <div class="crumbs">
                <span>Workspace</span>
                <span class="sep">/</span>
                <b id="page-crumb">${crumbLabel}</b>
                ${week}
            </div>`;
    }

    window.mountChrome = function (opts) {
        const o = opts || {};
        const sidebar = document.createElement('aside');
        sidebar.className = 'app-sidebar';
        sidebar.innerHTML = buildSidebar(o.activePage);
        document.body.insertBefore(sidebar, document.body.firstChild);

        const main = document.querySelector('main.app-main');
        if (main) {
            const header = document.createElement('header');
            header.className = 'topbar';
            header.innerHTML = buildTopbar(o.crumbLabel || '', o.weekLabel || '');
            main.insertBefore(header, main.firstChild);
        }
    };
})();
