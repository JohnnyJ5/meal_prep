let selectedMeals = new Set();
let isPlanning = false;

function isMobile() {
    return window.matchMedia('(max-width: 768px)').matches;
}

document.addEventListener('DOMContentLoaded', () => {
    initializeWeekDates();
    initMobileTabs();
    fetchMeals();
    fetchIngredients();
    fetchCalendarEventsForWeek();
    refreshCalendarLinkStatus();
});

async function refreshCalendarLinkStatus() {
    const meta = document.getElementById('sidebar-calendar-meta');
    const dot = document.getElementById('sidebar-calendar-dot');
    const link = document.getElementById('sidebar-calendar-link');
    if (!meta || !dot || !link) return;
    try {
        const probeStart = new Date();
        const probeEnd = new Date(probeStart);
        probeEnd.setDate(probeStart.getDate() + 1);
        const url = `/api/calendar/events?timeMin=${encodeURIComponent(probeStart.toISOString())}&timeMax=${encodeURIComponent(probeEnd.toISOString())}`;
        const response = await fetch(url);
        if (response.ok) {
            meta.textContent = 'Linked';
            dot.classList.remove('is-unlinked');
            dot.classList.add('is-linked');
            link.setAttribute('title', 'Calendar linked — click to re-authenticate');
        } else {
            meta.textContent = 'Click to connect';
            dot.classList.remove('is-linked');
            dot.classList.add('is-unlinked');
            link.setAttribute('title', 'Connect your Google Calendar');
        }
    } catch (e) {
        meta.textContent = 'Click to connect';
        dot.classList.add('is-unlinked');
    }
}

let availableIngredients = [];

async function fetchIngredients() {
    try {
        const response = await fetch('/api/ingredients');
        if (response.ok) {
            availableIngredients = await response.json();
            updateIngredientsDatalist();
        }
    } catch (e) {
        console.error("Error fetching ingredients:", e);
    }
}

function updateIngredientsDatalist() {
    const datalist = document.getElementById('available-ingredients');
    if (!datalist) return;
    datalist.innerHTML = '';
    availableIngredients.forEach(ing => {
        const option = document.createElement('option');
        option.value = ing.name;
        datalist.appendChild(option);
    });
}

async function fetchMeals() {
    const loadingState = document.getElementById('loading');
    const errorState = document.getElementById('error');
    const plannerLayout = document.getElementById('planner-layout');
    const actionBar = document.getElementById('action-bar');

    loadingState.classList.remove('hidden');
    errorState.classList.add('hidden');
    if (plannerLayout) plannerLayout.classList.add('hidden');
    actionBar.classList.add('hidden');

    try {
        const response = await fetch('/api/meals');
        if (!response.ok) throw new Error('Network response was not ok');

        const meals = await response.json();
        renderMeals(meals);

        loadingState.classList.add('hidden');
        document.getElementById('planner-layout').classList.remove('hidden');
        actionBar.classList.remove('hidden');
    } catch (error) {
        console.error('Error fetching meals:', error);
        loadingState.classList.add('hidden');
        errorState.classList.remove('hidden');
        document.getElementById('error-message').textContent = 'Failed to load meals from the server.';
    }
}

let mealMetaById = {}; // { mealId: { hasOptional: bool } }

function renderMeals(meals) {
    const grid = document.getElementById('meal-grid');
    grid.innerHTML = '';
    mealMetaById = {};

    // Format meal names (e.g. "baked-chicken-breast" -> "Baked Chicken Breast")
    const formatName = (str) => {
        return str.split('-')
            .map(word => word.charAt(0).toUpperCase() + word.slice(1))
            .join(' ');
    };

    const grouped = {};
    meals.forEach(meal => {
        const cat = meal.category || 'Uncategorized';
        if (!grouped[cat]) grouped[cat] = [];
        grouped[cat].push(meal);
        mealMetaById[meal.name] = { hasOptional: !!meal.has_optional_ingredients };
    });

    for (const [category, categoryMeals] of Object.entries(grouped)) {
        let index = 0;
        const header = document.createElement('h3');
        header.className = 'category-header collapsed';
        header.innerHTML = `<span class="category-chevron">&#9654;</span> ${category}`;
        grid.appendChild(header);

        const group = document.createElement('div');
        group.className = 'category-group hidden';
        grid.appendChild(group);

        header.addEventListener('click', () => {
            const isCollapsed = header.classList.toggle('collapsed');
            group.classList.toggle('hidden', isCollapsed);
        });

        // Touch tap on header — same scroll-guard
        let hTouchStartX = 0, hTouchStartY = 0;
        header.addEventListener('touchstart', (e) => {
            hTouchStartX = e.touches[0].clientX;
            hTouchStartY = e.touches[0].clientY;
        }, { passive: true });
        header.addEventListener('touchend', (e) => {
            const dx = e.changedTouches[0].clientX - hTouchStartX;
            const dy = e.changedTouches[0].clientY - hTouchStartY;
            if (Math.abs(dx) > 8 || Math.abs(dy) > 8) return;
            e.preventDefault();
            header.click();
        }, { passive: false });

        categoryMeals.forEach((meal) => {
            const mealId = meal.name;
            const card = document.createElement('div');
            card.className = 'meal-card';
            card.style.animationDelay = `${index * 0.05}s`;
            index++;

            // Add emoji based on name just for visual flair
            let emoji = '🍲';
            if (mealId.includes('chicken')) emoji = '🍗';
            if (mealId.includes('turkey')) emoji = '🦃';
            if (mealId.includes('pasta') || mealId.includes('penne')) emoji = '🍝';
            if (mealId.includes('burger') || mealId.includes('hamburger')) emoji = '🍔';
            if (mealId.includes('pancake')) emoji = '🥞';

            const hasOptional = !!meal.has_optional_ingredients;
            const customizeHint = hasOptional ? '<span class="meal-card-customize">+ add-ons</span>' : '';

            card.innerHTML = `
                <div class="meal-card-header">
                    <h3>${emoji} ${formatName(mealId)}</h3>
                    ${customizeHint}
                </div>
                <div class="meal-card-addons" hidden></div>
                <button type="button" class="meal-card-remove" draggable="false" onclick="removeMealFromSlot(event, this)" title="Remove from day" aria-label="Remove from day">&times;</button>
            `;
            card.id = `meal-${mealId}-${index}`;
            card.setAttribute('data-meal-id', mealId);
            card.setAttribute('draggable', 'true');
            card.setAttribute('ondragstart', 'drag(event)');
            if (hasOptional) card.dataset.hasOptional = '1';

            // Click behaviour: if the meal has optional ingredients, the click
            // opens the add-on picker for this tile. Otherwise, fall back to
            // the existing selection toggle used by "View Ingredients".
            card.addEventListener('click', (e) => {
                if (card.classList.contains('dragging')) return;

                if (hasOptional) {
                    openAddonsModal(card, mealId);
                    return;
                }

                if (selectedMeals.has(mealId)) {
                    selectedMeals.delete(mealId);
                    card.classList.remove('selected');
                } else {
                    selectedMeals.add(mealId);
                    card.classList.add('selected');
                }
                updateActionBar();
            });

            group.appendChild(card);
            attachMealTouchListeners(card, mealId);
        });
    }
}

// ── Add-on (optional ingredient) picker ─────────────────────────────────────

let addonsModalState = null; // { cardEl, mealId, initialAddons, onConfirm }

async function openAddonsModal(cardEl, mealId, onConfirm = null) {
    // Fetch the full meal so we know which ingredients are optional
    let meal;
    try {
        const res = await fetch(`/api/meals/${encodeURIComponent(mealId)}`);
        if (!res.ok) throw new Error('fetch failed');
        meal = await res.json();
    } catch (e) {
        console.error('Failed to load meal for add-ons:', e);
        showToast('Could not load add-ons for this recipe.', false);
        return;
    }

    const optionalIngs = (meal.ingredients || []).filter(i => i.optional);
    if (optionalIngs.length === 0) {
        // Drift between cached has_optional_ingredients and current DB state:
        // still run the confirm callback so callers aren't stranded.
        if (typeof onConfirm === 'function') onConfirm();
        return;
    }

    const currentSelections = new Set(readAddonsFromCard(cardEl));
    const list = document.getElementById('addons-list');
    list.innerHTML = '';
    optionalIngs.forEach(ing => {
        const label = document.createElement('label');
        label.className = 'addon-row';
        const checked = currentSelections.has(ing.name) ? 'checked' : '';
        label.innerHTML = `
            <input type="checkbox" class="addon-check" data-name="${ing.name}" ${checked}>
            <span>${ing.name}</span>
        `;
        list.appendChild(label);
    });

    const title = document.getElementById('addons-title');
    if (title) {
        const formatName = mealId.split('-').map(w => w.charAt(0).toUpperCase() + w.slice(1)).join(' ');
        title.textContent = `Add-ons for ${formatName}`;
    }

    addonsModalState = {
        cardEl,
        mealId,
        initialAddons: Array.from(currentSelections),
        onConfirm,
    };
    document.getElementById('addons-modal').classList.remove('hidden');
}

function confirmAddonsModal() {
    if (!addonsModalState) return;
    const checks = document.querySelectorAll('#addons-list .addon-check');
    const selected = [];
    checks.forEach(c => { if (c.checked) selected.push(c.dataset.name); });
    const { cardEl, onConfirm } = addonsModalState;
    writeAddonsToCard(cardEl, selected);
    closeModal('addons-modal');
    addonsModalState = null;
    if (typeof onConfirm === 'function') onConfirm();
}

function cancelAddonsModal() {
    closeModal('addons-modal');
    addonsModalState = null;
}

function readAddonsFromCard(cardEl) {
    if (!cardEl || !cardEl.dataset.addons) return [];
    try {
        const parsed = JSON.parse(cardEl.dataset.addons);
        return Array.isArray(parsed) ? parsed : [];
    } catch (e) {
        return [];
    }
}

function writeAddonsToCard(cardEl, addons) {
    if (!cardEl) return;
    if (!addons || addons.length === 0) {
        delete cardEl.dataset.addons;
    } else {
        cardEl.dataset.addons = JSON.stringify(addons);
    }
    renderAddonsBadge(cardEl);
}

function renderAddonsBadge(cardEl) {
    const badge = cardEl.querySelector('.meal-card-addons');
    if (!badge) return;
    const addons = readAddonsFromCard(cardEl);
    if (addons.length === 0) {
        badge.hidden = true;
        badge.textContent = '';
    } else {
        badge.hidden = false;
        badge.textContent = '+ ' + addons.join(', ');
    }
}

// Add drag and drop functions
function allowDrop(ev) {
    ev.preventDefault();
}

function dragEnter(ev) {
    ev.preventDefault();
    const dayCol = ev.target.closest('.day-col');
    if (dayCol) {
        dayCol.classList.add('drag-over');
    }
}

function dragLeave(ev) {
    const dayCol = ev.target.closest('.day-col');
    if (dayCol) {
        // Only remove if we're actually leaving the day column, not entering a child
        const rect = dayCol.getBoundingClientRect();
        if (ev.clientX <= rect.left || ev.clientX >= rect.right || ev.clientY <= rect.top || ev.clientY >= rect.bottom) {
            dayCol.classList.remove('drag-over');
        }
    }
}

function drag(ev) {
    ev.dataTransfer.setData("text", ev.currentTarget.id);
    ev.currentTarget.classList.add('dragging');
}

document.addEventListener('dragend', (ev) => {
    document.querySelectorAll('.meal-card').forEach(c => c.classList.remove('dragging'));
});

function drop(ev) {
    ev.preventDefault();
    const data = ev.dataTransfer.getData("text");
    const draggedElt = document.getElementById(data);

    // Find the closest droppable container (meal-slot or meal-grid)
    let dropTarget = ev.target;
    if (!dropTarget.classList.contains('meal-slot') && !dropTarget.id.includes('meal-grid')) {
        dropTarget = dropTarget.closest('.meal-slot') || dropTarget.closest('.meal-grid') || dropTarget.closest('.day-col')?.querySelector('.meal-slot');
    }

    // Remove drag-over class from all columns
    document.querySelectorAll('.day-col').forEach(col => col.classList.remove('drag-over'));

    if (dropTarget && draggedElt) {
        const fromGrid = !!draggedElt.closest('#meal-grid');
        const toGrid = dropTarget.id === 'meal-grid';
        const toSlot = dropTarget.classList.contains('meal-slot');

        if (fromGrid && toSlot) {
            // Clone when moving from grid to a slot so it remains available
            const clone = draggedElt.cloneNode(true);
            clone.id = draggedElt.id + '-clone-' + Date.now() + '-' + Math.floor(Math.random() * 1000);
            clone.classList.remove('selected', 'dragging');
            const dayCol = dropTarget.closest('.day-col');
            if (dayCol) clone.dataset.placedDate = dayCol.dataset.date;
            dropTarget.appendChild(clone);
            renderAddonsBadge(clone);
            // Reset the source recipe tile so the next drag starts vanilla
            writeAddonsToCard(draggedElt, []);
        } else if (!fromGrid && toGrid) {
            // If dragging from a slot back to the grid, just remove it from the planner
            draggedElt.remove();
        } else if (toSlot && !fromGrid) {
            // Moving from slot to slot — update the placed date
            const dayCol = dropTarget.closest('.day-col');
            if (dayCol) draggedElt.dataset.placedDate = dayCol.dataset.date;
            dropTarget.appendChild(draggedElt);
            draggedElt.classList.remove('selected');
        }

        updateActionBar();
    }
}

function removeMealFromSlot(ev, btn) {
    ev.stopPropagation();
    const card = btn.closest('.meal-card');
    if (card && card.closest('.meal-slot')) {
        card.remove();
        updateActionBar();
    }
}

// ── Mobile tab bar ───────────────────────────────────────────────────────────

function initMobileTabs() {
    const tabBar = document.querySelector('.mobile-tab-bar');
    if (!tabBar) return;

    const tabs = tabBar.querySelectorAll('.mobile-tab');
    const panels = document.querySelectorAll('[data-panel]');

    function activateTab(targetTab) {
        tabs.forEach(t => t.classList.toggle('active', t === targetTab));
        const targetPanelId = targetTab.dataset.tab;
        panels.forEach(p => p.classList.toggle('panel-active', p.dataset.panel === targetPanelId));
    }

    if (isMobile()) activateTab(tabs[0]); // default: meals panel on load

    tabs.forEach(tab => tab.addEventListener('click', () => activateTab(tab)));
}

// ── Mobile touch-tap assignment ──────────────────────────────────────────────

let mobilePendingMeal = null; // { mealId, cardEl }

function handleMealTap(mealId, cardEl) {
    if (!isMobile()) return;

    // Tapping the same card again deselects it
    if (mobilePendingMeal && mobilePendingMeal.cardEl === cardEl) {
        cancelMobileTapSelection();
        return;
    }

    // Variant meals: open the add-on picker first. The pending-tap state is
    // only entered after the user clicks OK in the modal.
    if (cardEl.dataset.hasOptional === '1' && readAddonsFromCard(cardEl).length === 0) {
        openAddonsModal(cardEl, mealId, () => enterMobilePending(mealId, cardEl));
        return;
    }

    enterMobilePending(mealId, cardEl);
}

function enterMobilePending(mealId, cardEl) {
    if (mobilePendingMeal) {
        mobilePendingMeal.cardEl.classList.remove('pending-tap');
        selectedMeals.delete(mobilePendingMeal.mealId);
    }

    mobilePendingMeal = { mealId, cardEl };
    cardEl.classList.add('pending-tap');
    selectedMeals.add(mealId);

    const banner = document.getElementById('mobile-selected-meal-banner');
    const nameSpan = document.getElementById('mobile-selected-meal-name');
    if (banner && nameSpan) {
        const baseName = cardEl.querySelector('h3')?.textContent || mealId;
        const addons = readAddonsFromCard(cardEl);
        nameSpan.textContent = addons.length > 0
            ? `${baseName} (+ ${addons.join(', ')})`
            : baseName;
        banner.classList.remove('hidden');
    }

    document.querySelectorAll('.day-col').forEach(col => col.classList.add('tap-target'));
    updateActionBar();

    // Auto-switch to calendar tab so the user can tap a day
    const calTab = document.querySelector('.mobile-tab[data-tab="calendar"]');
    if (calTab) calTab.click();
}

function handleDayTap(dayColEl) {
    if (!isMobile() || !mobilePendingMeal) return;

    const { mealId, cardEl } = mobilePendingMeal;
    const fromGrid = !!cardEl.closest('#meal-grid');
    const mealSlot = dayColEl.querySelector('.meal-slot');

    if (mealSlot) {
        if (fromGrid) {
            // Clone from sidebar so the original stays available
            const clone = cardEl.cloneNode(true);
            clone.id = cardEl.id + '-clone-' + Date.now();
            clone.classList.remove('selected', 'dragging', 'pending-tap');
            clone.dataset.placedDate = dayColEl.dataset.date;
            attachMealTouchListeners(clone, mealId);
            mealSlot.appendChild(clone);
            renderAddonsBadge(clone);
            // Reset the source recipe tile so the next tap starts vanilla
            writeAddonsToCard(cardEl, []);
        } else {
            // Moving between slots
            cardEl.dataset.placedDate = dayColEl.dataset.date;
            mealSlot.appendChild(cardEl);
        }
        updateActionBar();
    }

    cancelMobileTapSelection();
}

function cancelMobileTapSelection() {
    if (mobilePendingMeal) {
        mobilePendingMeal.cardEl.classList.remove('pending-tap');
        selectedMeals.delete(mobilePendingMeal.mealId);
        mobilePendingMeal = null;
    }
    document.querySelectorAll('.day-col').forEach(col => col.classList.remove('tap-target'));
    const banner = document.getElementById('mobile-selected-meal-banner');
    if (banner) banner.classList.add('hidden');
    updateActionBar();
}

function attachMealTouchListeners(cardEl, mealId) {
    let touchStartX = 0;
    let touchStartY = 0;
    cardEl.addEventListener('touchstart', (e) => {
        touchStartX = e.touches[0].clientX;
        touchStartY = e.touches[0].clientY;
    }, { passive: true });
    cardEl.addEventListener('touchend', (e) => {
        if (!isMobile()) return;
        const dx = e.changedTouches[0].clientX - touchStartX;
        const dy = e.changedTouches[0].clientY - touchStartY;
        if (Math.abs(dx) > 8 || Math.abs(dy) > 8) return; // was a scroll, not a tap
        e.preventDefault(); // suppress ghost click
        handleMealTap(mealId, cardEl);
    }, { passive: false });
}

function attachDayTouchListeners(dayColEl) {
    if (dayColEl.dataset.touchBound) return; // guard against double-registration
    dayColEl.dataset.touchBound = '1';
    let touchStartX = 0;
    let touchStartY = 0;
    dayColEl.addEventListener('touchstart', (e) => {
        touchStartX = e.touches[0].clientX;
        touchStartY = e.touches[0].clientY;
    }, { passive: true });
    dayColEl.addEventListener('touchend', (e) => {
        if (!isMobile() || !mobilePendingMeal) return;
        const dx = e.changedTouches[0].clientX - touchStartX;
        const dy = e.changedTouches[0].clientY - touchStartY;
        if (Math.abs(dx) > 8 || Math.abs(dy) > 8) return;
        e.preventDefault();
        handleDayTap(dayColEl);
    }, { passive: false });
}

function updateActionBar() {
    const planBtn = document.getElementById('plan-btn');
    const viewBtn = document.getElementById('view-ingredients-btn');
    const viewBtnText = viewBtn.querySelector('.btn-text');

    let count = 0;
    const days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
    days.forEach(day => {
        const slot = document.querySelector(`#day-${day} .meal-slot`);
        if (slot) {
            count += slot.querySelectorAll('.meal-card').length;
        }
    });

    const totalSelected = count + selectedMeals.size;
    viewBtnText.textContent = totalSelected > 1 ? `View Ingredients (${totalSelected})` : 'View Ingredients';

    if (totalSelected > 0) {
        planBtn.removeAttribute('disabled');
    } else {
        planBtn.setAttribute('disabled', 'true');
    }
    if (selectedMeals.size > 0) {
        viewBtn.removeAttribute('disabled');
    } else {
        viewBtn.setAttribute('disabled', 'true');
    }
}

async function planMeals() {
    if (document.getElementById('plan-btn').hasAttribute('disabled') || isPlanning) return;

    isPlanning = true;
    const btn = document.getElementById('plan-btn');
    const btnText = btn.querySelector('.btn-text');
    const btnLoader = btn.querySelector('.btn-loader');

    // UI Loading state
    btn.setAttribute('disabled', 'true');
    btnText.textContent = 'Generating...';
    btnLoader.classList.remove('hidden');

    try {
        const schedule = {};
        const days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
        days.forEach(day => {
            schedule[day] = [];
            const slot = document.querySelector(`#day-${day} .meal-slot`);
            if (slot) {
                const cards = slot.querySelectorAll('.meal-card');
                cards.forEach(c => {
                    const mealId = c.getAttribute('data-meal-id');
                    const addons = readAddonsFromCard(c);
                    if (addons.length > 0) {
                        schedule[day].push({ name: mealId, add_ons: addons });
                    } else {
                        schedule[day].push(mealId);
                    }
                });
            }
        });

        const response = await fetch('/api/plan', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(schedule)
        });

        const result = await response.json();

        if (response.ok) {
            window._lastIngredientsText = result.ingredients_text || '';
            window._lastSyncedEventIds = [];
            try {
                const eventIds = await doCalendarSync();
                window._lastSyncedEventIds = eventIds;
                await fetchCalendarEventsForWeek();
                // Remove dragged meal cards from calendar slots, leaving only Google Calendar event cards
                const days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
                days.forEach(day => {
                    const slot = document.querySelector(`#day-${day} .meal-slot`);
                    if (slot) slot.innerHTML = '';
                });
                selectedMeals.clear();
                document.querySelectorAll('.meal-card').forEach(c => c.classList.remove('selected'));
                updateActionBar();
            } catch (e) {
                if (e.message === 'not_linked') {
                    showToast('Please link your Google account to sync meals to the calendar.', false);
                    setTimeout(() => { window.location.href = '/auth/google'; }, 2000);
                    return;
                }
                console.error('Pre-sync error:', e);
            }
            openOrderModal();
        } else {
            throw new Error(result.error || 'Failed to generate plan');
        }
    } catch (error) {
        console.error('Error starting plan:', error);
        showToast('An error occurred while generating the plan.', false);
    } finally {
        isPlanning = false;
        btn.removeAttribute('disabled');
        btnText.textContent = 'Generate Plan';
        btnLoader.classList.add('hidden');
        updateActionBar(); // Re-evaluates disabled state based on selection count
    }
}


async function doCalendarSync() {
    const schedule = {};
    const days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
    days.forEach(day => {
        const dayCol = document.getElementById(`day-${day}`);
        const meals = [];
        const slot = document.querySelector(`#day-${day} .meal-slot`);
        if (slot) {
            slot.querySelectorAll('.meal-card').forEach(c => {
                meals.push(c.getAttribute('data-meal-id'));
            });
        }
        schedule[day] = { date: dayCol ? dayCol.dataset.date : null, meals };
    });

    const response = await fetch('/api/calendar/sync', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(schedule)
    });

    if (response.status === 403) throw new Error('not_linked');
    const data = await response.json();
    return data.event_ids || [];
}

function closeModal(modalId = 'result-modal') {
    const modal = document.getElementById(modalId);
    if (modal) {
        modal.classList.add('hidden');
    }
}

function openOrderModal() {
    const tomorrow = new Date();
    tomorrow.setDate(tomorrow.getDate() + 1);
    tomorrow.setHours(9, 0, 0, 0);
    const pad = n => String(n).padStart(2, '0');
    const localISO = `${tomorrow.getFullYear()}-${pad(tomorrow.getMonth()+1)}-${pad(tomorrow.getDate())}T${pad(tomorrow.getHours())}:${pad(tomorrow.getMinutes())}`;
    document.getElementById('order-datetime').value = localISO;
    document.getElementById('order-ingredients').textContent = window._lastIngredientsText || '';
    document.getElementById('order-modal').classList.remove('hidden');
}

function skipOrderModal() {
    closeModal('order-modal');
    showToast('Meals synced to Google Calendar!');
}

async function cancelPlan() {
    closeModal('order-modal');
    const ids = window._lastSyncedEventIds || [];
    if (ids.length === 0) return;
    try {
        await fetch('/api/calendar/delete-events', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ event_ids: ids })
        });
        window._lastSyncedEventIds = [];
        _calendarCache.clear();
        await fetchCalendarEventsForWeek();
        showToast('Plan cancelled — calendar events removed.', false);
    } catch (e) {
        console.error('Failed to delete events:', e);
        showToast('Could not remove calendar events.', false);
    }
}

function showToast(message, success = true) {
    const toast = document.getElementById('toast');
    toast.textContent = message;
    toast.style.background = success ? 'rgba(34,197,94,0.9)' : 'rgba(239,68,68,0.9)';
    toast.style.color = '#fff';
    toast.classList.remove('hidden');
    setTimeout(() => toast.classList.add('hidden'), 3500);
}

function setOrderStatus(message, success) {
    const el = document.getElementById('order-status');
    el.textContent = message;
    el.style.background = success ? 'rgba(34,197,94,0.15)' : 'rgba(239,68,68,0.15)';
    el.style.color = success ? '#16a34a' : '#dc2626';
    el.classList.remove('hidden');
}

async function scheduleOrderEvent() {
    const datetimeInput = document.getElementById('order-datetime').value;
    if (!datetimeInput) {
        setOrderStatus('Please select a date and time.', false);
        return;
    }
    const start = new Date(datetimeInput);
    const end = new Date(start.getTime() + 60 * 60 * 1000);

    const btn = document.getElementById('order-confirm-btn');
    btn.setAttribute('disabled', 'true');
    btn.textContent = 'Creating...';
    document.getElementById('order-status').classList.add('hidden');

    try {
        const response = await fetch('/api/calendar/order', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                start: start.toISOString(),
                end: end.toISOString(),
                ingredients: window._lastIngredientsText || ''
            })
        });
        if (response.ok) {
            window._lastSyncedEventIds = [];
            _calendarCache.clear();
            closeModal('order-modal');
            await fetchCalendarEventsForWeek();
            showToast('Events created and synced to Google Calendar!');
        } else if (response.status === 403) {
            setOrderStatus('Please link your Google account first.', false);
            setTimeout(() => { window.location.href = '/auth/google'; }, 1500);
        } else {
            setOrderStatus('Failed to create calendar event.', false);
        }
    } catch (e) {
        console.error('Order event error:', e);
        setOrderStatus('An error occurred. Please try again.', false);
    } finally {
        btn.removeAttribute('disabled');
        btn.textContent = 'Create Events';
    }
}


// --- Management Logic ---

// Enum mapping for units
const UnitEnum = {
    0: 'Grams', 1: 'Ounces', 2: 'Cups', 3: 'Tablespoons',
    4: 'Teaspoons', 5: 'Pounds', 6: 'Whole', 7: 'Half',
    8: 'Small', 9: 'Cloves', 10: 'Heads'
};

async function viewIngredients() {
    if (selectedMeals.size === 0) return;

    const perMealList = document.getElementById('per-meal-list');
    const consolidatedList = document.getElementById('consolidated-list');

    perMealList.innerHTML = '<div class="spinner"></div>';
    consolidatedList.innerHTML = '';

    document.getElementById('ingredients-modal').classList.remove('hidden');

    try {
        const fetchPromises = Array.from(selectedMeals).map(mealId =>
            fetch(`/api/meals/${encodeURIComponent(mealId)}`)
                .then(res => res.ok ? res.json() : null)
                .catch(() => null)
        );

        const results = await Promise.allSettled(fetchPromises);
        const mealsData = results
            .filter(r => r.status === 'fulfilled' && r.value)
            .map(r => r.value);

        // 1. Render Per Meal
        perMealList.innerHTML = '';
        const consolidated = {};

        mealsData.forEach(meal => {
            if (!meal || !meal.name) return;

            const group = document.createElement('div');
            group.className = 'meal-ing-group';

            const formatName = meal.name.split('-').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
            let html = `<h4>${formatName}</h4><ul>`;

            meal.ingredients.forEach(ing => {
                const unitStr = UnitEnum[ing.unit] || '';
                const amountStr = Number.isInteger(ing.amount) ? ing.amount : ing.amount.toFixed(2);
                html += `<li>${ing.name}: ${amountStr} ${unitStr} ${ing.preparation !== 'None' ? `(${ing.preparation})` : ''}</li>`;

                // Build consolidated
                const key = `${ing.name.toLowerCase()}_${ing.unit}`;
                if (!consolidated[key]) {
                    consolidated[key] = {
                        name: ing.name,
                        amount: 0,
                        unitStr: unitStr
                    };
                }
                consolidated[key].amount += ing.amount;
            });

            html += '</ul>';
            group.innerHTML = html;
            perMealList.appendChild(group);
        });

        // 2. Render Consolidated
        let consHtml = '<ul>';
        for (const key in consolidated) {
            const item = consolidated[key];
            const amountStr = Number.isInteger(item.amount) ? item.amount : item.amount.toFixed(2);
            consHtml += `<li><strong>${item.name}</strong>: ${amountStr} ${item.unitStr}</li>`;
        }
        consHtml += '</ul>';

        if (Object.keys(consolidated).length === 0) {
            consHtml = '<p class="text-secondary">No ingredients found.</p>';
        }

        consolidatedList.innerHTML = consHtml;

    } catch (error) {
        console.error("Error fetching ingredients:", error);
        perMealList.innerHTML = '<p class="text-error">Failed to load ingredients.</p>';
        consolidatedList.innerHTML = '';
    }
}

async function openManageModal() {
    document.getElementById('manage-modal').classList.remove('hidden');
    document.getElementById('manage-list-view').classList.remove('hidden');
    document.getElementById('manage-edit-view').classList.add('hidden');
    await fetchManageMeals();
}

async function fetchManageMeals() {
    const listContainer = document.getElementById('manage-meal-list');
    listContainer.innerHTML = '<div class="spinner"></div>';

    try {
        const response = await fetch('/api/meals');
        const meals = await response.json();

        listContainer.innerHTML = '';
        if (meals.length === 0) {
            listContainer.innerHTML = '<p class="text-secondary">No meals found.</p>';
            return;
        }

        meals.forEach(meal => {
            const mealId = meal.name;
            const item = document.createElement('div');
            item.className = 'manage-item';

            const formatName = str => str.split('-').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');

            item.innerHTML = `
                <div class="manage-item-info">
                    <strong>${formatName(mealId)}</strong>
                    <span class="badge" style="margin-left:8px; font-size:0.8rem; background:var(--primary-color); padding:2px 6px; border-radius:4px; color:white;">${meal.category}</span>
                    <br><small style="color: var(--text-secondary);">${mealId}</small>
                    <br><small style="color: var(--text-secondary);">ID: ${meal.id}</small>
                </div>
                <div class="manage-item-actions">
                    <button class="btn btn-secondary btn-sm" onclick="editMeal('${mealId}')">Edit</button>
                    <button class="btn btn-danger btn-sm" onclick="deleteMeal('${mealId}')">Delete</button>
                </div>
            `;
            listContainer.appendChild(item);
        });
    } catch (e) {
        listContainer.innerHTML = '<p style="color:var(--error-color)">Error loading meals.</p>';
    }
}

let currentEditMeal = null;

async function editMeal(mealId) {
    document.getElementById('manage-list-view').classList.add('hidden');
    document.getElementById('manage-edit-view').classList.remove('hidden');

    const form = document.getElementById('meal-form');
    form.reset();
    document.getElementById('ingredients-list').innerHTML = '';

    currentEditMeal = mealId;
    document.getElementById('edit-view-title').textContent = 'Edit Meal: ' + mealId;
    document.getElementById('meal-name').value = mealId;

    try {
        const mealRes = await fetch(`/api/meals/${mealId}`);
        if (mealRes.ok) {
            const mealData = await mealRes.json();
            document.getElementById('meal-category').value = mealData.category || "Uncategorized";
            mealData.ingredients.forEach(ing => {
                addIngredientRow(ing.name, ing.amount, ing.unit, !!ing.optional);
            });
        } else {
            addIngredientRow(); // Fallback to empty row
        }
    } catch (e) {
        addIngredientRow();
    }
}

function openEditView(mealId = null) {
    if (mealId) {
        editMeal(mealId);
        return;
    }
    document.getElementById('manage-list-view').classList.add('hidden');
    document.getElementById('manage-edit-view').classList.remove('hidden');

    const form = document.getElementById('meal-form');
    form.reset();
    document.getElementById('ingredients-list').innerHTML = '';

    currentEditMeal = null;
    document.getElementById('edit-view-title').textContent = 'Add New Meal';
    document.getElementById('meal-category').value = "Uncategorized";
    addIngredientRow();
}

function closeEditView() {
    document.getElementById('manage-list-view').classList.remove('hidden');
    document.getElementById('manage-edit-view').classList.add('hidden');
}

function addIngredientRow(name = "", amount = "", unit = 0, optional = false) {
    const container = document.getElementById('ingredients-list');
    const row = document.createElement('div');
    row.className = 'ingredient-row';

    // Create unit options
    let unitOptions = '';
    for (const [val, label] of Object.entries(UnitEnum)) {
        unitOptions += `<option value="${val}" ${parseInt(val) === unit ? 'selected' : ''}>${label}</option>`;
    }

    row.innerHTML = `
        <div style="flex: 1; display: flex; align-items: center; gap: 8px;">
            <input type="text" list="available-ingredients" placeholder="Ingredient Name (e.g., Chicken Breast)" value="${name}" required class="form-control name-input" style="flex: 1;">
            <button type="button" class="btn btn-secondary btn-sm" onclick="openAddIngredientModal(this)" title="New Ingredient">+</button>
        </div>
        <input type="number" step="0.01" placeholder="Amount" value="${amount}" required class="form-control amount-input">
        <select class="form-control unit-input">
            ${unitOptions}
        </select>
        <label class="optional-toggle" title="Optional add-on: users can opt in per meal plan">
            <input type="checkbox" class="optional-input" ${optional ? 'checked' : ''}>
            <span>Optional</span>
        </label>
        <button type="button" class="btn btn-danger btn-sm" onclick="this.parentElement.remove()">X</button>
    `;

    container.appendChild(row);
    const nameInput = row.querySelector('.name-input');
    const amountInput = row.querySelector('.amount-input');

    nameInput.focus();

    // Auto-focus amount when ingredient is selected or Enter is pressed
    nameInput.addEventListener('input', (e) => {
        const val = e.target.value;
        if (availableIngredients.some(ing => ing.name === val)) {
            setTimeout(() => amountInput.focus(), 10);
        }
    });

    nameInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
            e.preventDefault();
            setTimeout(() => amountInput.focus(), 10);
        }
    });
}

async function saveMeal(e) {
    e.preventDefault();

    const name = document.getElementById('meal-name').value.trim().toLowerCase().replace(/\s+/g, '-');
    const category = document.getElementById('meal-category').value.trim() || 'Uncategorized';
    const ingredientRows = document.querySelectorAll('.ingredient-row');

    if (ingredientRows.length === 0) {
        alert("Please add at least one ingredient.");
        return;
    }

    const ingredients = [];
    ingredientRows.forEach(row => {
        ingredients.push({
            name: row.querySelector('.name-input').value,
            amount: parseFloat(row.querySelector('.amount-input').value),
            unit: parseInt(row.querySelector('.unit-input').value),
            preparation: "None",
            optional: row.querySelector('.optional-input')?.checked || false
        });
    });

    const payload = { name, category, ingredients };

    try {
        const method = currentEditMeal ? 'PUT' : 'POST';
        const url = currentEditMeal ? `/api/meals/${currentEditMeal}` : '/api/meals/add';

        const response = await fetch(url, {
            method: method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        if (response.ok) {
            closeEditView();
            fetchManageMeals(); // Refresh list
            fetchMeals();       // Refresh main grid
        } else {
            alert("Failed to save meal.");
        }
    } catch (err) {
        console.error(err);
        alert("Error saving meal.");
    }
}

async function deleteMeal(mealId) {
    if (!confirm(`Are you sure you want to delete ${mealId}?`)) return;

    try {
        const response = await fetch(`/api/meals/${mealId}`, { method: 'DELETE' });
        if (response.ok) {
            fetchManageMeals(); // Refresh list
            fetchMeals();       // Refresh main grid

            // If they had it selected on the main screen, deselect it
            if (selectedMeals.has(mealId)) {
                selectedMeals.delete(mealId);
                updateActionBar();
            }
        } else {
            alert("Failed to delete meal.");
        }
    } catch (err) {
        console.error(err);
        alert("Error deleting meal.");
    }
}

let lastActiveInput = null;

function openAddIngredientModal(btn) {
    lastActiveInput = btn.previousElementSibling;
    const currentVal = lastActiveInput.value.trim();
    document.getElementById('new-ing-name').value = currentVal;
    document.getElementById('add-ingredient-modal').classList.remove('hidden');
    document.getElementById('new-ing-name').focus();
}

async function createNewIngredient(e) {
    e.preventDefault();
    const name = document.getElementById('new-ing-name').value.trim();
    const category = document.getElementById('new-ing-category').value.trim();

    if (!name || !category) return;

    try {
        const response = await fetch('/api/ingredients/add', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name, category })
        });

        if (response.ok) {
            closeModal('add-ingredient-modal');
            await fetchIngredients(); // Refresh list
            if (lastActiveInput) {
                lastActiveInput.value = name;
                // Focus the amount input in the same row
                const row = lastActiveInput.closest('.ingredient-row');
                if (row) {
                    const amountInput = row.querySelector('.amount-input');
                    if (amountInput) setTimeout(() => amountInput.focus(), 10);
                }
            }
            document.getElementById('new-ingredient-form').reset();
        } else {
            alert("Failed to add ingredient. It might already exist.");
        }
    } catch (err) {
        console.error(err);
        alert("Error saving ingredient.");
    }
}

// --- Specific Weekly Google Calendar Logic ---
const daysOfWeek = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
const dayNamesLong = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];

function getMondayOfCurrentWeek() {
    const d = new Date();
    const day = d.getDay();
    const diff = d.getDate() - day + (day === 0 ? -6 : 1);
    const monday = new Date(d.setDate(diff));
    monday.setHours(0, 0, 0, 0);
    return monday;
}

// Tracks the first day shown in the calendar window
let windowStart = (() => { const d = new Date(); d.setHours(0,0,0,0); return d; })();

function visibleColumnCount() {
    return isMobile() ? 3 : 7;
}

function initializeWeekDates() {
    const today = new Date();
    today.setHours(0, 0, 0, 0);
    const visibleCount = visibleColumnCount();

    daysOfWeek.forEach((colId, index) => {
        const dayCol = document.getElementById(`day-${colId}`);
        if (!dayCol) return;

        if (index >= visibleCount) {
            dayCol.style.display = 'none';
            return;
        }
        dayCol.style.display = '';

        const currentDate = new Date(windowStart);
        currentDate.setDate(windowStart.getDate() + index);

        const dayNameSpan = dayCol.querySelector('.day-name');
        if (dayNameSpan) {
            dayNameSpan.textContent = dayNamesLong[currentDate.getDay()];
        }
        const dateLabel = dayCol.querySelector('.date-label');
        if (dateLabel) {
            dateLabel.textContent = currentDate.toLocaleDateString('en-US', { month: 'short', day: 'numeric' });
        }
        const y = currentDate.getFullYear();
        const m = String(currentDate.getMonth() + 1).padStart(2, '0');
        const d = String(currentDate.getDate()).padStart(2, '0');
        dayCol.dataset.date = `${y}-${m}-${d}`;

        dayCol.classList.toggle('today', currentDate.getTime() === today.getTime());
        attachDayTouchListeners(dayCol);
    });
}

function repositionPlacedCards() {
    // Build date → meal-slot map for the current window
    const dateToSlot = {};
    daysOfWeek.forEach(colId => {
        const col = document.getElementById(`day-${colId}`);
        if (col && col.dataset.date) {
            dateToSlot[col.dataset.date] = col.querySelector('.meal-slot');
        }
    });

    // Collect all placed cards across all slots
    const allCards = Array.from(document.querySelectorAll('.day-col .meal-slot .meal-card'));

    allCards.forEach(card => {
        const placedDate = card.dataset.placedDate;
        if (!placedDate) return;

        const targetSlot = dateToSlot[placedDate];
        if (targetSlot) {
            // Card belongs in a visible column — move it there
            if (card.parentElement !== targetSlot) targetSlot.appendChild(card);
            card.style.display = '';
        } else {
            // Date is outside the current window — hide but keep in DOM
            card.style.display = 'none';
        }
    });
}

const _calendarCache = new Map();
let _calendarFetchSeq = 0;
let _lastShiftDelta = 1;

function calendarCacheKey(date) {
    return date.toISOString().slice(0, 10);
}

function clearEventsSlots() {
    daysOfWeek.forEach(colId => {
        const slot = document.querySelector(`#day-${colId} .events-slot`);
        if (slot) slot.innerHTML = '';
    });
}

async function shiftWindow(delta) {
    if (delta === 0) {
        windowStart = new Date(); windowStart.setHours(0,0,0,0);
    } else {
        _lastShiftDelta = delta;
        windowStart = new Date(windowStart);
        windowStart.setDate(windowStart.getDate() + delta);
    }
    initializeWeekDates();
    repositionPlacedCards();

    const key = calendarCacheKey(windowStart);
    if (_calendarCache.has(key)) {
        // Render from cache instantly — no flicker, no blank state
        renderCalendarEvents(_calendarCache.get(key));
    } else {
        clearEventsSlots();
    }

    await fetchCalendarEventsForWeek();
}

async function fetchCalendarEventsForWeek() {
    const seq = ++_calendarFetchSeq;
    const fetchStart = new Date(windowStart);
    const key = calendarCacheKey(fetchStart);
    const windowEnd = new Date(fetchStart);
    windowEnd.setDate(fetchStart.getDate() + 7);

    try {
        const url = `/api/calendar/events?timeMin=${encodeURIComponent(fetchStart.toISOString())}&timeMax=${encodeURIComponent(windowEnd.toISOString())}`;
        const response = await fetch(url);

        if (seq !== _calendarFetchSeq) return;

        if (response.ok) {
            let eventsText = await response.text();
            if (!eventsText) return;
            try {
                const eventsArray = JSON.parse(eventsText);
                _calendarCache.set(key, eventsArray);
                renderCalendarEvents(eventsArray);
            } catch (e) {
                console.error("Failed to parse calendar events JSON:", e);
            }
        } else if (response.status === 403) {
            console.log("Not linked to Google Calendar or error fetching events.");
        }
    } catch (e) {
        console.error("Error fetching calendar events:", e);
    }

    // Prefetch one step ahead in the last navigation direction
    _prefetchWindow(_lastShiftDelta);
}

async function _prefetchWindow(delta) {
    const prefetchStart = new Date(windowStart);
    prefetchStart.setDate(windowStart.getDate() + delta);
    const key = calendarCacheKey(prefetchStart);
    if (_calendarCache.has(key)) return;

    const prefetchEnd = new Date(prefetchStart);
    prefetchEnd.setDate(prefetchStart.getDate() + 7);

    try {
        const url = `/api/calendar/events?timeMin=${encodeURIComponent(prefetchStart.toISOString())}&timeMax=${encodeURIComponent(prefetchEnd.toISOString())}`;
        const response = await fetch(url);
        if (response.ok) {
            const text = await response.text();
            if (text) _calendarCache.set(key, JSON.parse(text));
        }
    } catch (e) { /* silent — prefetch failure is non-critical */ }
}

function renderCalendarEvents(calendarDataArray) {
    // Build a map from YYYY-MM-DD (local) → column element based on current window
    const dateToCol = {};
    daysOfWeek.forEach(colId => {
        const col = document.getElementById(`day-${colId}`);
        if (col && col.dataset.date) dateToCol[col.dataset.date] = col;
    });

    daysOfWeek.forEach(colId => {
        const slot = document.querySelector(`#day-${colId} .events-slot`);
        if (slot) slot.innerHTML = '';
    });

    const legend = document.getElementById('calendar-legend');
    if (legend) {
        legend.innerHTML = '';
        legend.classList.add('hidden');
    }

    if (!Array.isArray(calendarDataArray)) return;

    calendarDataArray.forEach(calData => {
        const events = calData.events?.items || [];
        const bgColor = calData.backgroundColor;
        const fgColor = calData.foregroundColor;
        const summary = calData.summary;

        if (events.length > 0 && legend) {
            legend.classList.remove('hidden');
            const item = document.createElement('div');
            item.className = 'legend-item';
            item.innerHTML = `
                <div class="legend-color" style="background-color: ${bgColor}"></div>
                <span>${summary}</span>
            `;
            legend.appendChild(item);
        }

        events.forEach(event => {

            const startStr = event.start.dateTime || event.start.date;
            if (!startStr) return;

            // Compute local YYYY-MM-DD for matching against data-date
            let startDate;
            let dateKey;
            if (event.start.dateTime) {
                startDate = new Date(startStr);
                const y = startDate.getFullYear();
                const mo = String(startDate.getMonth() + 1).padStart(2, '0');
                const d = String(startDate.getDate()).padStart(2, '0');
                dateKey = `${y}-${mo}-${d}`;
            } else {
                // All-day: date string is already YYYY-MM-DD, parse as local
                const [y, mo, d] = startStr.split('-').map(Number);
                startDate = new Date(y, mo - 1, d);
                dateKey = startStr.slice(0, 10);
            }

            const dayCol = dateToCol[dateKey];
            if (!dayCol) return;
            const slot = dayCol.querySelector('.events-slot');
            
            if (slot) {
                const card = document.createElement('div');
                card.className = 'event-card';
                card.style.backgroundColor = `${bgColor}33`; // 20% opacity
                card.style.borderLeftColor = bgColor;
                card.style.color = 'var(--text-primary)';
                card.dataset.eventId = event.id;

                const timeStr = event.start.dateTime ?
                    startDate.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'}) : 'All Day';

                const isMealPrepEvent = event.extendedProperties?.private?.mealPrepApp === 'true';
                card.innerHTML = `
                    <div class="event-card-title" title="${event.summary}">${event.summary}</div>
                    <div class="event-card-time" style="color: ${fgColor}; opacity: 0.8">${timeStr}</div>
                    ${isMealPrepEvent ? `<button class="event-card-delete" onclick="deleteCalendarEvent('${event.id}', this.parentElement)" title="Delete event">&times;</button>` : ''}
                `;
                slot.appendChild(card);
            }
        });
    });
}

async function deleteCalendarEvent(eventId, cardEl) {
    cardEl.style.opacity = '0.4';
    cardEl.style.pointerEvents = 'none';
    try {
        const response = await fetch('/api/calendar/delete-events', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ event_ids: [eventId] })
        });
        if (response.ok) {
            _calendarCache.clear();
            cardEl.remove();
        } else {
            cardEl.style.opacity = '';
            cardEl.style.pointerEvents = '';
            showToast('Failed to delete event.', false);
        }
    } catch (e) {
        console.error('Delete event error:', e);
        cardEl.style.opacity = '';
        cardEl.style.pointerEvents = '';
        showToast('Failed to delete event.', false);
    }
}
