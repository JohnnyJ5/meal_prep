# Branding & Design System

Reference for anyone — human or agent — building UI for the meal-prep app.
Every new screen, modal, and component should pull from this document so the
look stays consistent.

The reference implementation lives at `static/mockups/notion-3-indigo-kanban.html`
and its rendered preview at `static/mockups/notion-3-indigo-kanban.png`. When
in doubt, open that file and copy the pattern.

## Brand Vibe

Clean productivity tool, Notion-adjacent. Minimal, text-forward, focused. A
single cool indigo accent against neutral surfaces, with sage and rose
reserved as secondary category colors. No glassmorphism, no gradients on
content, no decorative emoji. The product should feel like a tool a serious
home cook uses on a laptop, not a lifestyle app.

## Color Palette

Implement as CSS custom properties at `:root`. These names are normative —
new CSS should reference them, not raw hex codes.

```css
:root {
  /* Surfaces */
  --paper:       #FFFFFF;  /* primary card / panel background */
  --bg:          #FAFAFA;  /* app background, sidebar, day columns */
  --bg-2:        #F4F4F4;  /* hover state, segmented control track */

  /* Lines & dividers */
  --line:        #ECECEC;  /* default 1px border */
  --line-2:      #E0E0E0;  /* slightly stronger divider, dashed drop zones */

  /* Text */
  --ink:         #111111;  /* primary text, headings */
  --ink-2:       #5F5F5F;  /* body, secondary text */
  --ink-3:       #9A9A9A;  /* tertiary, placeholders, metadata */

  /* Accent (indigo) — primary brand color */
  --accent:      #4F46E5;  /* buttons, active states, links, primary stripe */
  --accent-2:    #6366F1;  /* hover / lighter accent */
  --accent-soft: #EEF0FF;  /* active row background, today-column background */
  --accent-mid:  #C7CBF5;  /* today-column border */

  /* Category accents — used on meal cards to differentiate meal types */
  --sage:        #3E5C44;  /* vegetarian / vegan meals */
  --rose:        #B91C5C;  /* red-meat meals */
}
```

**Color usage rules:**

- Indigo (`--accent`) is the only "brand" color. Never introduce a fourth
  accent without updating this document.
- Sage and rose are **categorical**, not brand colors — they tag meal type.
  Use them on meal cards, recipe tags, and category dots. Do not use them
  for buttons, links, or chrome.
- Day columns: default day uses `--bg`. The current day uses `--accent-soft`
  background with `--accent-mid` border. Past days are not styled differently.
- Soft variants (`--accent-soft`, `#ECF1ED` for sage, `#FCE7EE` for rose)
  are for backgrounds; pair with the strong variant for text or borders.

## Typography

```css
font-family: 'Inter', system-ui, sans-serif;
```

Load Inter from Google Fonts (weights 400, 500, 600, 700). No serif faces,
no display faces. Body line-height is `1.45`. Antialiasing is on:
`-webkit-font-smoothing: antialiased`.

| Role          | Size    | Weight | Letter-spacing | Notes                              |
| ------------- | ------- | ------ | -------------- | ---------------------------------- |
| Page title    | 26 px   | 700    | -0.02em        | e.g. "Week of May 18, 2026"        |
| Section title | 18 px   | 700    | -0.01em        | Column headers, modal titles       |
| Card title    | 13 px   | 600    | normal         | Meal name on a kanban card         |
| Body          | 13.5 px | 400–500| normal         | Default                            |
| Metadata      | 12 px   | 400    | normal         | Meta-row values, footnotes         |
| Eyebrow       | 10–11 px| 600    | 0.12em         | UPPERCASE labels above titles      |
| Tag / pill    | 10.5 px | 500    | normal         | Prep-time tags, count badges       |

The `Eyebrow` style (uppercase, tracked, `--ink-3`) is used liberally for
section labels — `RECIPES`, `CATEGORIES`, `WORKSPACES`, day-of-week, the
`DINNER` / `LUNCH` label inside meal cards. Always small caps via
`text-transform: uppercase`, never typed in actual caps.

## Layout

### App shell
- Two-column: fixed `248px` left sidebar, fluid main column.
- Sidebar background is `--bg`, separated from main by a `1px --line` border.
- Main column background is `--paper`.
- App is full-viewport — no outer padding around the shell.

### Main column structure
1. **Topbar** — 52 px tall, breadcrumbs left, segmented view + actions + avatar right. Bottom border `1px --line`.
2. **Page header** — title + description + meta-row + week navigation. Bottom border `1px --line`.
3. **Body** — recipe library (280 px) + 7-column kanban planner (fluid). Padding `16px 28px 22px`.

### Spacing scale
Stick to multiples of 4 px. Common values:

| Token       | Value | Used for                              |
| ----------- | ----- | ------------------------------------- |
| `xs`        | 4 px  | Inline gaps inside chips              |
| `sm`        | 6–8 px| Card internal padding, button spacing |
| `md`        | 12 px | Inter-section gap inside a panel      |
| `lg`        | 16 px | Body padding top/bottom               |
| `xl`        | 22–28 px | Page-header padding, body sides    |

### Border radii

| Token | Value | Used for                       |
| ----- | ----- | ------------------------------ |
| `sm`  | 4 px  | Inline buttons, tag chips      |
| `md`  | 6–7 px| Sidebar nav items, form inputs |
| `lg`  | 8–10 px | Cards, day columns, panels   |

No fully-rounded pill shapes (`border-radius: 999px`) except for the small
count badges and tag chips in the recipe list. Pill buttons were retired
with the old glass design.

### Shadows

Mostly flat. Only two acceptable shadows:

```css
box-shadow: 0 1px 0 rgba(0,0,0,0.02);    /* meal cards */
box-shadow: 0 1px 2px rgba(0,0,0,0.04);  /* active segmented-control button */
```

No drop-shadows on buttons, no glow, no blur, no inset shadows.

## Components

### Sidebar nav item
```html
<div class="nav-item"><span class="chev">▾</span><span class="ico">▤</span> Label <span class="ct">24</span></div>
```
- 5 px / 8 px padding, 5 px radius, 13 px text.
- Hover: `--bg-2` background.
- Active: `--accent-soft` background, `--accent` text, weight 600.
- Children indent 18 px with a `1px --line` left border.

### Button
```css
.btn { padding: 6px 12px; border-radius: 7px; border: 1px solid var(--line); background: var(--paper); color: var(--ink); font: 500 12px Inter; }
.btn.primary { background: var(--accent); color: white; border-color: var(--accent); }
```
- Three variants only: default (white, hairline border), `primary` (indigo, white text), `ghost` (transparent, no border — sparingly).
- No icon-only round buttons except `.iconbtn` (28×28, square with 6 px radius).
- No emoji on button labels.

### Segmented control (Day / Week / Month)
- Container: `--bg-2` background, `1px --line` border, 7 px radius, 2 px inner padding.
- Active button: `--paper` background, `1px 2px` shadow.
- Inactive buttons: transparent, `--ink-2` text.

### Recipe row (sidebar library)
```html
<div class="recipe">
  <span class="h">⋮⋮</span>      <!-- drag handle -->
  <span class="em">▤</span>       <!-- icon mark -->
  <span class="n">Recipe name</span>
  <span class="tag acc">35m</span> <!-- prep-time tag -->
</div>
```
- Tag color signals category: `.acc` = indigo (default), `.sage` = vegetarian, `.rose` = red meat.
- Drag handle `⋮⋮` uses `--ink-3` at 60% opacity.

### Day column (kanban)
```html
<div class="col [today]">
  <div class="col-h">
    <span class="dow">MON</span>
    <span class="dn">18</span>
    <span class="count">1</span>
  </div>
  <div class="card indigo">...</div>
  <div class="col-add">+ add</div>
</div>
```
- 10 px / 8 px padding, 10 px radius, `--bg` background, `1px --line` border.
- `.today` column: `--accent-soft` background, `--accent-mid` border.
- Column header has a dashed bottom divider (`1px dashed --line-2`).
- `.col-add` ("+ add" affordance) is `--ink-3` 11 px, becomes `--accent` on hover.

### Meal card (kanban tile)
```html
<div class="card indigo">  <!-- or .sage, .rose -->
  <div class="stripe"></div>
  <div class="t">DINNER · 19:00</div>
  <div class="n">Sheet-Pan Salmon</div>
  <div class="foot"><span class="pip"></span>35m · 4 srv</div>
</div>
```
- White background, `1px --line` border, 8 px radius, 8 px / 10 px padding.
- Top stripe (3 px) and `.foot .pip` (6 px dot) use the card's category color.
- Eyebrow `.t` is uppercase 9.5 px, includes meal-time and optionally clock time.
- Card body `.n` is 13 px / weight 600.
- Foot row shows prep time + serving count, separated by `·`.

### Drop placeholder
```html
<div class="drop">drag a recipe<br>onto this day</div>
```
- Dashed `1px --line-2` border, 6 px radius, `--ink-3` text at 75% opacity.

## Iconography

- **No emoji.** Existing meal data may include 🍗/🍝 but new UI chrome should avoid them.
- Use geometric Unicode marks for icons: `▤ ▦ ▥ ▨ ● ○ ▸ ▾ ⌕ ⌘ ⚙ ↗ ⋮⋮`.
- Future improvement: replace these with a single icon library (Lucide
  recommended) — but only as a coordinated migration, not piecemeal.
- The workspace icon (top-left "M" badge) is `--accent` filled, white text,
  26×26, 6 px radius, weight 700.

## States & Interactions

- **Hover**: surface elements lighten to `--bg-2`, links/buttons brighten one
  step. No scale transforms.
- **Active / selected**: `--accent-soft` background + `--accent` text.
- **Today indicator**: column gets `--accent-soft` bg and `--accent-mid` border; day number gets `--accent` color.
- **Drag-over (drop target)**: column border becomes `--accent`, background tints to `--accent-soft`. No scale or shadow change.
- **Loading**: a 1 px indeterminate progress bar at the top of the panel,
  `--accent` over `--accent-soft`. No spinners.
- **Empty state**: dashed-border placeholder with `--ink-3` text. Keep the copy short and lowercase ("drag a recipe", "weekend", "+ add").

## What NOT to do

- ❌ Pill-shaped buttons (`border-radius: 999px`) on anything bigger than a tag.
- ❌ Glassmorphism, `backdrop-filter: blur`, gradient backgrounds on chrome.
- ❌ Drop shadows above 1–2 px blur.
- ❌ Decorative emoji in button labels, headings, or empty states.
- ❌ Outfit font, serif headings, or any new typeface — Inter only.
- ❌ Dark mode (out of scope until explicitly designed).
- ❌ Color-shifting hover states that change hue. Lighten, don't recolor.

## Adding a new screen

1. Open `static/mockups/notion-3-indigo-kanban.html` and copy the shell.
2. Wire the new content into the main column. Reuse the topbar, page-header,
   and sidebar patterns verbatim.
3. Reference CSS variables, not hex codes.
4. Render a 1440×900 PNG of the new screen into `static/mockups/` and commit
   it alongside the implementation so the design system stays self-documenting.
5. If a new component is needed (e.g., a data table), add a section to this
   file describing its tokens, states, and copy norms.

## File map

| File                                                | Purpose                                  |
| --------------------------------------------------- | ---------------------------------------- |
| `docs/BRANDING.md`                                  | This document — single source of truth   |
| `static/mockups/notion-3-indigo-kanban.html`        | Canonical reference layout               |
| `static/mockups/notion-3-indigo-kanban.png`         | Rendered preview at 1440×900             |
| `static/index.html`                                 | Live app markup (port from mockup)       |
| `static/style.css`                                  | Live app styles (port from mockup)       |

When the live app and the mockup diverge, treat the mockup + this doc as the
intent and update the live files to match — not the other way around.
