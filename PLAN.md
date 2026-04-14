# Mobile UI Plan

## Overview

Make the Meal Prep Dashboard mobile-friendly with a breakpoint at 768px. No new dependencies — pure CSS/JS. Desktop behavior is completely unchanged. All changes live in `static/`.

---

## Step 1 — HTML additions (`static/index.html`)

Three structural additions, all invisible on desktop via CSS.

### 1a. Mobile tab bar

Insert immediately after `<section class="planner-layout hidden" id="planner-layout">`, before `.sidebar-container`:

```html
<div class="mobile-tab-bar">
  <button class="mobile-tab active" data-tab="meals">Meals</button>
  <button class="mobile-tab" data-tab="calendar">Calendar</button>
</div>
```

### 1b. `data-panel` attributes

- Add `data-panel="meals"` to `<div class="sidebar-container">`
- Add `data-panel="calendar"` to `<div class="calendar-container">`

### 1c. Touch hint paragraph

Below the existing drag hint inside `.sidebar-container`:

```html
<p class="text-secondary touch-hint" style="font-size: 0.9rem; margin-bottom: 1rem;">
  Tap a meal to select it, then tap a day to assign it.
</p>
```

### 1d. Pending-assignment banner

Inside `.calendar-container`, immediately before `<div class="calendar-grid">`:

```html
<div id="mobile-selected-meal-banner" class="hidden">
  <span id="mobile-selected-meal-name"></span>
  <button onclick="cancelMobileTapSelection()">Cancel</button>
</div>
```

---

## Step 2 — CSS additions (`static/style.css`)

### 2a. Base styles (outside any media query)

```css
.mobile-tab-bar { display: none; }
.touch-hint { display: none; }
#mobile-selected-meal-banner { display: none; }

.meal-card.pending-tap {
    border-color: var(--accent-color);
    background: rgba(88, 166, 255, 0.12);
    box-shadow: 0 0 14px var(--accent-glow);
}

.day-col.tap-target {
    background: rgba(88, 166, 255, 0.12);
    border: 1px solid var(--accent-color);
}
```

### 2b. `@media (max-width: 768px)` block

Append to the bottom of `style.css`:

```css
@media (max-width: 768px) {

    /* Container & Header */
    .container { padding: 1rem; }
    h1 { font-size: 1.8rem; }
    header { margin-bottom: 1.5rem; }
    header p { font-size: 0.9rem; }
    .header-content { flex-direction: column; align-items: flex-start; gap: 1rem; }
    .auth-buttons { width: 100%; justify-content: flex-start; flex-wrap: wrap; gap: 0.5rem; }
    .auth-buttons .btn, .auth-buttons a.btn { font-size: 0.85rem; padding: 0.5rem 1rem; }

    /* Planner layout: stack vertically, one panel visible at a time */
    .planner-layout { flex-direction: column; gap: 0; }
    .sidebar-container, .calendar-container { width: 100%; flex: none; border-radius: 0; border-left: none; border-right: none; }
    .sidebar-container[data-panel], .calendar-container[data-panel] { display: none; }
    .sidebar-container[data-panel].panel-active, .calendar-container[data-panel].panel-active { display: block; }

    /* Tab bar */
    .mobile-tab-bar { display: flex; width: 100%; background: var(--surface-color); border-bottom: 1px solid var(--border-color); position: sticky; top: 0; z-index: 50; }
    .mobile-tab { flex: 1; padding: 0.85rem 0; background: transparent; border: none; border-bottom: 3px solid transparent; color: var(--text-secondary); font-family: var(--font-body); font-size: 1rem; font-weight: 600; cursor: pointer; transition: color 0.2s, border-color 0.2s; }
    .mobile-tab.active { color: var(--accent-color); border-bottom-color: var(--accent-color); }

    /* Drag hint hidden / touch hint visible */
    .sidebar-container > p.text-secondary:not(.touch-hint) { display: none; }
    .touch-hint { display: block; }

    /* Calendar grid: horizontal scroll */
    .calendar-container { overflow-x: auto; -webkit-overflow-scrolling: touch; }
    .calendar-grid { min-width: 560px; }
    .day-col { min-width: 80px; padding: 0.6rem 0.4rem; }
    .day-col h3 { font-size: 0.8rem; margin-bottom: 0.5rem; }
    .date-label { font-size: 0.7rem; }
    .calendar-header { flex-direction: column; align-items: flex-start; gap: 0.5rem; margin-bottom: 0.75rem; }

    /* Mobile selected-meal banner */
    #mobile-selected-meal-banner {
        display: flex; align-items: center; justify-content: space-between; gap: 0.75rem;
        background: rgba(88, 166, 255, 0.15); border: 1px solid var(--accent-color);
        border-radius: 10px; padding: 0.6rem 1rem; margin-bottom: 0.75rem;
        font-size: 0.9rem; color: var(--text-primary); font-weight: 600;
    }
    #mobile-selected-meal-banner.hidden { display: none !important; }
    #mobile-selected-meal-banner button {
        background: transparent; border: 1px solid var(--text-secondary);
        color: var(--text-secondary); border-radius: 6px; padding: 0.25rem 0.6rem;
        font-size: 0.8rem; cursor: pointer; white-space: nowrap;
    }

    /* Ingredient rows: name full-width on row 1; amount | unit | delete on row 2 */
    .ingredient-row { display: grid; grid-template-columns: 1fr 1fr auto; grid-template-rows: auto auto; gap: 0.4rem; }
    .ingredient-row > div:first-child { grid-column: 1 / -1; }

    /* Action bar: full-width */
    .action-bar {
        left: 0; right: 0; bottom: 0; width: 100%;
        transform: none; border-radius: 0; border-left: none; border-right: none; border-bottom: none;
        gap: 0.75rem; padding: 0.75rem 1rem; flex-wrap: wrap; animation: none;
    }
    .action-bar .selection-count { font-size: 0.9rem; }
    .action-bar .btn { flex: 1; min-width: 0; padding: 0.6rem 0.75rem; font-size: 0.9rem; }
    .action-bar #view-ingredients-btn { margin-left: 0; }

    /* Modals → bottom sheets */
    .modal { align-items: flex-end; }
    .modal-content { width: 100%; max-width: 100%; border-radius: 20px 20px 0 0; padding: 1.5rem 1.25rem; max-height: 90vh; overflow-y: auto; transform: translateY(100%); }
    .modal:not(.hidden) .modal-content { transform: translateY(0); }
    .large-modal { max-width: 100%; max-height: 92vh; }

    /* Ingredients layout: stack vertically */
    .ingredients-layout { flex-direction: column; }

    /* Form actions */
    .form-actions { flex-wrap: wrap; gap: 0.5rem; }
    .form-actions .btn { flex: 1; min-width: 120px; }

    /* Meal grid: no height cap (full panel scrolls) */
    .meal-grid { max-height: none; }

    /* Manage list items: stack vertically */
    .manage-item { flex-direction: column; align-items: flex-start; gap: 0.75rem; }
    .manage-item-actions { width: 100%; justify-content: flex-end; }
}
```

---

## Step 3 — JavaScript additions (`static/script.js`)

### 3a. Mobile detection helper

Add after line 1 (after `let selectedMeals`):

```js
function isMobile() {
    return window.matchMedia('(max-width: 768px)').matches;
}
```

### 3b. Tab bar initialization

Add this function and call it from `DOMContentLoaded`:

```js
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

    if (isMobile()) activateTab(tabs[0]); // default: meals panel

    tabs.forEach(tab => tab.addEventListener('click', () => activateTab(tab)));
}
```

Update `DOMContentLoaded`:

```js
document.addEventListener('DOMContentLoaded', () => {
    initializeWeekDates();
    initMobileTabs();            // new
    fetchMeals();
    fetchIngredients();
    fetchCalendarEventsForWeek();
});
```

### 3c. Touch-tap assignment system

Add after the `drop()` function:

```js
// ── Mobile touch-tap assignment ──────────────────────────────────────────────

let mobilePendingMeal = null; // { mealId, cardEl }

function handleMealTap(mealId, cardEl) {
    if (!isMobile()) return;

    if (mobilePendingMeal && mobilePendingMeal.cardEl === cardEl) {
        cancelMobileTapSelection();
        return;
    }

    if (mobilePendingMeal) mobilePendingMeal.cardEl.classList.remove('pending-tap');

    mobilePendingMeal = { mealId, cardEl };
    cardEl.classList.add('pending-tap');

    const banner = document.getElementById('mobile-selected-meal-banner');
    const nameSpan = document.getElementById('mobile-selected-meal-name');
    if (banner && nameSpan) {
        nameSpan.textContent = cardEl.querySelector('h3')?.textContent || mealId;
        banner.classList.remove('hidden');
    }

    document.querySelectorAll('.day-col').forEach(col => col.classList.add('tap-target'));

    // Auto-switch to calendar tab
    const calTab = document.querySelector('.mobile-tab[data-tab="calendar"]');
    if (calTab) calTab.click();
}

function handleDayTap(dayColEl) {
    if (!isMobile() || !mobilePendingMeal) return;

    const { mealId, cardEl } = mobilePendingMeal;
    const fromGrid = cardEl.parentElement?.id === 'meal-grid';
    const mealSlot = dayColEl.querySelector('.meal-slot');

    if (mealSlot) {
        if (fromGrid) {
            const clone = cardEl.cloneNode(true);
            clone.id = cardEl.id + '-clone-' + Date.now();
            clone.classList.remove('selected', 'dragging', 'pending-tap');
            clone.dataset.placedDate = dayColEl.dataset.date;
            attachMealTouchListeners(clone, mealId);
            mealSlot.appendChild(clone);
        } else {
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
        mobilePendingMeal = null;
    }
    document.querySelectorAll('.day-col').forEach(col => col.classList.remove('tap-target'));
    const banner = document.getElementById('mobile-selected-meal-banner');
    if (banner) banner.classList.add('hidden');
}

function attachMealTouchListeners(cardEl, mealId) {
    cardEl.addEventListener('touchend', (e) => {
        if (!isMobile()) return;
        e.preventDefault();
        handleMealTap(mealId, cardEl);
    }, { passive: false });
}

function attachDayTouchListeners(dayColEl) {
    if (dayColEl.dataset.touchBound) return;
    dayColEl.dataset.touchBound = '1';
    dayColEl.addEventListener('touchend', (e) => {
        if (!isMobile() || !mobilePendingMeal) return;
        e.preventDefault();
        handleDayTap(dayColEl);
    }, { passive: false });
}
```

### 3d. Wire up touch listeners

In `renderMeals()`, after `grid.appendChild(card)`:
```js
attachMealTouchListeners(card, mealId);
```

In `initializeWeekDates()`, after each day column is built:
```js
attachDayTouchListeners(dayCol);
```

---

## Step 4 — Implementation order

Execute in this order so each step is independently testable:

1. Header + action-bar CSS
2. Planner layout stacking + tab bar HTML/CSS
3. `initMobileTabs()` JS
4. Calendar horizontal scroll CSS
5. Modal bottom-sheet CSS
6. Ingredient row reflow CSS
7. Touch hint + banner HTML/CSS
8. Touch interaction JS (`handleMealTap`, `handleDayTap`, etc.)

---

## Step 5 — Gotchas

- **Ghost clicks**: `touchend` handlers need `e.preventDefault()` + `{ passive: false }` to suppress the browser's synthesized click ~300ms later.
- **`cloneNode(true)` drops JS listeners**: Re-call `attachMealTouchListeners(clone, mealId)` in `handleDayTap` before appending the clone.
- **`shiftWindow` re-runs `initializeWeekDates`**: Guard `attachDayTouchListeners` with `data-touch-bound` to prevent stacking listeners.
- **Inline `margin-left: auto`** on `#view-ingredients-btn` (HTML line 113): overridden in the media query with `margin-left: 0`.
- **Action bar entry animation**: Disable with `animation: none` inside the media query to avoid the `-50%` translate glitch on mobile.
- **Meal grid `max-height`**: Set to `none` on mobile — the sidebar panel scrolls as a whole page, avoiding nested scroll traps.
