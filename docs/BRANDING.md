# Branding & Design System

Reference for anyone — human or agent — building UI for the meal-prep app.
Every new screen, modal, and component should pull from this document so the
look stays consistent.

The reference implementation is the live app itself: the per-page documents
under `static/pages/<page>/index.html` plus the shared `static/shared/base.css`
(design tokens, sidebar, topbar, buttons, modals) and `static/shared/chrome.js`
(sidebar + topbar shell). When in doubt, open them and copy the pattern.

## Brand Vibe

**Warm editorial.** A cookbook crossed with a workshop notebook. Cream paper,
espresso ink, terracotta accent, with sage and mustard as quiet secondary
tones. Typography does the heavy lifting: a soft, slightly book-y serif
(Fraunces) for display set against a refined geometric body face (Inter Tight).

The product should feel like a recipe binder that happens to be a webapp —
considered, hand-touched, with character — not a dashboard. Think *Apartamento
magazine* and *Joshua Weissman's "An Unapologetic Cookbook"*, not Notion.

Anti-patterns: cool blue/indigo accents, pure-white surfaces, all-uppercase
tracked eyebrow labels stacked on every section, soft-pillow rounded cards.
We retired those with the previous direction.

## Color Palette

Implement as CSS custom properties at `:root`. These token names are normative
— new CSS should reference them, not raw hex codes.

```css
:root {
  /* Surfaces — cream paper warmth, not sterile white */
  --paper:       #F8F4EC;  /* primary surface, body background */
  --paper-2:     #FBF8F2;  /* card / panel background (slightly brighter) */
  --bg:          #F3EDE1;  /* sidebar, day columns, recessed surfaces */
  --bg-2:        #ECE3D2;  /* hover surfaces, sunken regions */

  /* Lines — warm taupe, never cool gray */
  --line:        #E0D5BF;  /* default 1px border */
  --line-2:      #C9B998;  /* stronger divider, focus borders, dashed drop zones */

  /* Text — espresso family, not pure black */
  --ink:         #1A1410;  /* primary text, headings */
  --ink-2:       #5A4A3C;  /* body, secondary text */
  --ink-3:       #93826C;  /* tertiary, placeholders, metadata, italics */

  /* Accent — terracotta */
  --accent-color:#C2410C;  /* buttons, today indicator, primary stripe, link color */
  --accent-2:    #9A330A;  /* hover / darker terracotta */
  --accent-soft: #F3E2D0;  /* selected meal card, soft fills */
  --accent-mid:  #E8B89A;  /* mid-tone, soft borders */

  /* Category / secondary accents */
  --sage:        #3E5C44;  /* workouts, calendar events, "kept" actions */
  --sage-soft:   #DDE5DA;  /* drag-over target, sage tag fills */
  --mustard:     #B7791F;  /* templates, alternate tags */
  --rose:        #B91C5C;  /* red-meat meals (reserved) */

  /* Status — tuned warm */
  --success-color: #2F7D32;
  --error-color:   #B91C1C;
}
```

**Color usage rules:**

- Terracotta (`--accent-color`) is the brand color. One sharp accent, used
  sparingly — it should feel like a stamp, not a wash. Don't fill backgrounds
  or large regions with it.
- Sage is the *companion* accent used for workouts, calendar events, and
  "secondary positive" states (kept items, drop targets). Treat it as the
  brand color's quiet counterpart, not a third primary.
- Mustard appears on templates (a saved-workout signal) and may be used for
  cautionary or "in-progress" affordances. Use sparingly.
- Rose is reserved for red-meat meal cards if/when categorical color tagging
  returns. Do not use for chrome.
- Day columns: default day uses `--paper-2`. The current day uses `--paper`
  background with `--accent-color` border and a "TODAY" chip badge — not just
  a fill change. *Today should feel like a stamp, not a wash.*

## Typography

Two faces, loaded together from Google Fonts:

```html
<link href="https://fonts.googleapis.com/css2?family=Fraunces:opsz,wght,SOFT,WONK@9..144,400;9..144,500;9..144,600;9..144,700;9..144,800&family=Inter+Tight:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
```

```css
--font-display: 'Fraunces', 'Iowan Old Style', Georgia, serif;
--font-body:    'Inter Tight', system-ui, -apple-system, sans-serif;
--font-mono:    'JetBrains Mono', ui-monospace, SFMono-Regular, Menlo, monospace;
```

- **Fraunces** is a contemporary variable serif with soft optical sizing. Use
  `font-variation-settings: 'opsz' <size>` on large headings to opt into the
  display cut (warmer, more characterful) and `'SOFT' 50–100` for the rounded
  variant on the brandmark and select titles. Italic is heavily used for
  section labels and metadata — it gives the editorial voice.
- **Inter Tight** is the body face. Tighter spacing than vanilla Inter, more
  refined for a content-forward app. Body line-height is `1.5`.
- **JetBrains Mono** is for code blocks and any monospace meta display.

Antialiasing on: `-webkit-font-smoothing: antialiased; text-rendering: optimizeLegibility`.

| Role                | Family    | Size      | Weight   | Style          | Notes                                          |
| ------------------- | --------- | --------- | -------- | -------------- | ---------------------------------------------- |
| Page title (`h1`)   | Display   | 44 px     | 600      | normal         | `opsz: 144, SOFT: 50`, `tracking: -0.025em`    |
| Panel heading       | Display   | 22 px     | 600      | normal         | Recipe library, workouts list                  |
| Modal title         | Display   | 28 px     | 600      | normal         | `opsz: 72`                                     |
| Section label       | Display   | 13–17 px  | 500      | **italic**     | Replaces old uppercase eyebrow                 |
| Card title (recipe) | Body      | 13.5 px   | 500      | normal         | Meal name on a kanban card                     |
| Card title (workout)| Display   | 19 px     | 600      | normal         | Workout heading on workout-card                |
| Body                | Body      | 14.5 px   | 400      | normal         | Default                                        |
| Metadata            | Body      | 12–12.5 px| 400      | **italic**     | Meta-row values, footnotes, crumbs             |
| Date numeral        | Display   | 26 px     | 600      | normal         | `opsz: 72`, today gets `SOFT: 80`              |
| Day name            | Display   | 12 px     | 500      | **italic**     | "Mon", "Tue" — mixed case, NOT uppercase       |
| Today chip          | Body      | 9 px      | 700      | normal         | `letter-spacing: 0.18em`, uppercase, accent-on-paper |
| Button              | Body      | 13 px     | 500–600  | normal         |                                                |
| Mono / code         | Mono      | 12 px     | 400      | normal         |                                                |

**Italic is a brand tool.** When in doubt about a section label or piece of
metadata, set it in display italic at 13–16 px and `--ink-3`. That's the
voice. Avoid uppercase tracked labels — they belong to the old direction.

## Layout

### App shell
- Two-column: fixed `220px` left sidebar, fluid main column. The sidebar is
  intentionally narrow so the calendar grid can claim as much width as
  possible.
- Sidebar background is `--bg`, separated from main by a `1px --line` border.
- Main column background is `--paper`.
- Body has a fixed-attachment paper-grain noise overlay (subtle, ~3.5% opacity)
  applied via inline SVG `data:` URL. The grain is part of the brand — don't
  remove it without redoing the surface tokens.

### Main column structure
1. **Topbar** — 52 px tall, breadcrumbs in italic crumbs left, integration
   status / actions right. Bottom border `1px --line`. Background `--paper-2`.
   Side padding `var(--s-5)`.
2. **Page header (`.page-h`)** — editorial masthead. Top padding `var(--s-8)`,
   side padding `var(--s-5)`, big serif `h1`, italic description below. A
   soft gradient hairline replaces the hard bottom border.
3. **Planner body** — two-column panel-and-grid. `240px 1fr` grid with
   `var(--s-4)` gap and `var(--s-6) var(--s-5) var(--s-8)` padding. The
   240px recipe rail keeps the calendar grid wide.
4. **Workouts body** — full-width list and templates with `var(--s-5)` side
   padding throughout (workouts list, empty state, templates section).

### Spacing scale

Use **only** the tokens. No raw pixel values for spacing in new CSS.

| Token   | Value | Used for                                  |
| ------- | ----- | ----------------------------------------- |
| `--s-1` |  4 px | Inline gaps inside chips, tight rhythm    |
| `--s-2` |  8 px | Card internal padding, button gaps        |
| `--s-3` | 12 px | Inter-section gap inside a panel          |
| `--s-4` | 16 px | Form-group separation, card padding       |
| `--s-5` | 20 px | Panel padding                             |
| `--s-6` | 24 px | Modal padding                             |
| `--s-8` | 32 px | Page side padding                         |
| `--s-10`| 40 px | Page-header top padding                   |
| `--s-12`| 48 px | Hero / signature spacing                  |

### Border radii

We deliberately keep radii **small** to feel like printed cards rather than
soft UI pillows.

| Use                       | Radius |
| ------------------------- | ------ |
| Buttons, inputs, tags     |  2 px  |
| Cards, panels, modals     |  3–4 px|
| Today / status pill chip  |  2 px  |

Rounded-pill shapes (`border-radius: 999px`) are out. So is anything ≥ 8px.

### Shadows

Layered, paper-like depth — never the puffy SaaS shadow.

```css
/* Panel: subtle hairline + soft drop */
box-shadow: 0 1px 0 var(--line), 0 4px 12px -8px rgba(26, 20, 16, 0.08);

/* Card-in-day-column: stacked printed-paper effect */
box-shadow: 0 1px 0 var(--line), 0 2px 4px -2px rgba(26, 20, 16, 0.08);

/* Hover lift on card */
box-shadow: 0 1px 0 var(--line), 0 6px 14px -6px rgba(26, 20, 16, 0.12);

/* Modal */
box-shadow:
  0 1px 0 var(--paper-2) inset,
  0 30px 60px -20px rgba(26, 20, 16, 0.35),
  0 10px 25px -8px rgba(26, 20, 16, 0.15);
```

All shadow colors use the warm `rgba(26, 20, 16, ...)` ink, never cool
black/gray. The combination of a `0 1px 0 var(--line)` hairline + a soft drop
gives the "this card is sitting on a piece of paper" feel.

## Backgrounds & Texture

- Body has a **paper-grain noise overlay** applied via inline SVG `data:` URL,
  `background-attachment: fixed`. Around 3.5% opacity. This is non-negotiable
  brand texture — don't strip it.
- Panels and modals layer `--paper-2` over `--paper`/`--bg` to create a soft
  hierarchy without resorting to drop shadows alone.
- No gradients on chrome. No `backdrop-filter` blur except a tiny 2px blur on
  the modal scrim.

## Components

### Brandmark / workspace
- Display-italic capital "M" at 32 px, terracotta, with `font-variation-settings:
  'opsz' 144, 'SOFT' 100`. No background fill, no rounded square.
- Followed by "Meal Prep" in display 17/600, with the subtitle "weekly
  planner" set in italic at 11 px and `--ink-3`.
- Underline the whole block with a `1px --line` divider.

### Sidebar nav item
```html
<div class="nav-item active">
  <span class="nav-chev">▾</span>
  <span class="nav-ico">▤</span>
  <span>Weekly Planner</span>
</div>
```
- 12 px / 8 px padding, **2 px radius** (not the old 6 px).
- Hover: background `--bg-2`, color `--ink`, `padding-left` slides right by
  4 px — a small reveal animation.
- Active: **no background fill.** Instead, a 2 px terracotta rule (`::before`)
  pinned to the left edge of the item, plus weight 600 ink text. The accent
  is the rule, not a wash.

### Button

```css
.btn {
  font: 500 13px var(--font-body);
  padding: var(--s-2) var(--s-4);
  border-radius: 2px;
  border: 1px solid var(--line-2);
  background: var(--paper);
  color: var(--ink);
  letter-spacing: 0.005em;
}
.btn-primary {
  background: var(--accent-color);
  color: var(--paper-2);
  border-color: var(--accent-color);
  font-weight: 600;
  box-shadow: 0 1px 0 var(--accent-2);  /* "pressed-into-paper" lip */
}
.btn:active { transform: translateY(1px); }
```

- Three variants: default (transparent, hairline border), `primary`
  (terracotta), `secondary` (alias for default), `danger` (warm red text +
  hairline).
- No icon-only round buttons except `.iconbtn` (30×30, square, 6 px radius —
  used for week navigation).
- No emoji in button labels.
- Primary has a 1 px darker "lip" shadow so it reads as pressed into paper.

### Recipe library panel (sidebar)
```html
<aside class="recipe-library">
  <div class="recipe-library-h"><h2>Recipes</h2></div>
  …
</aside>
```
- `--paper-2` background, 1 px `--line` border, 4 px radius.
- Panel heading is display 22/600, **mixed case**, decorated with a small
  terracotta `❦` (heart fleuron) suffix. This fleuron is a brand mark —
  reuse it for major panel headings, sparingly.
- Bottom hairline under the heading.

### Meal card — recipe library variant
```html
<div class="meal-card">
  <h3>Sheet-Pan Salmon</h3>
</div>
```
- Transparent background, no visible border at rest. Just a row of text.
- Hover reveals: `--bg` background fill, an italic terracotta `→` arrow
  slides in from the left, and `padding-left` increases — the row feels like
  it leans forward to greet you.
- Selected: `--accent-soft` background, title turns `--accent-2`, weight 600.

### Meal card — kanban variant (placed in a day)
- `--paper-2` background.
- **3 px left border in `--accent-color`** — the "stamped recipe slip" look.
- 2 px corner radius.
- Layered hairline + drop shadow (see Shadows above).
- Hover: lifts 1 px and rotates `-0.3deg`. Subtle, like a printed card
  catching on the edge.

### Day column (kanban)
```html
<div class="day-col today">
  <h3>
    <span class="day-name">Mon</span>
    <span class="date-label">18</span>
  </h3>
  …
</div>
```
- Default: `--paper-2` background, 1 px `--line` border, **3 px radius**.
- Drag-over: `--sage-soft` background, `--sage` border, lifts 2 px. Drag
  targets use sage, not terracotta — terracotta is reserved for "today".
- **Today**: `--paper` background, 1 px solid terracotta border (and a `0 0
  0 1px --accent-color` ring to thicken it). A small "TODAY" pill badge
  (terracotta on paper, uppercase tracked 9 px) sits over the top-left
  corner of the column. The day name and date go terracotta with extra
  warmth from `SOFT: 80` on the Fraunces date numeral.
- Day name is display-italic mixed-case ("Mon", "Tue"). Never uppercase.

### Workout card
- `--paper-2` background.
- 3 px left border in **sage** (workouts are sage's domain — terracotta is
  for meals).
- Title in display 19/600. Meta line in italic, smaller, `--ink-3`.
- Hover swaps the left-border color to terracotta and lifts 1 px.

### Template card
- `--paper-2` background, 1 px `--line` border.
- A 2 px **mustard** stripe across the top (`::before`) — the "template"
  signal.
- Title in display 17/600. Meta in italic 12px, `--ink-3`.

### Templates section heading
- Display italic 22/500. Decorated with a leading terracotta `❦` fleuron.

### Modal
- `--paper` content surface, 4 px radius.
- Header: display 28/600 title (`opsz: 72`), `--line` divider below.
- Backdrop: warm-ink scrim (`rgba(26, 20, 16, 0.5)`) + 2 px blur.
- Layered shadow (see Shadows).

### Form input
- `--paper-2` background, `--line-2` border, **2 px radius**.
- Focus: brightens to `--paper`, border goes terracotta, 3 px ring in
  `--accent-soft`.

### Empty state
- Dashed `--line-2` border, 3 px radius, italic copy in `--ink-2`, centered.
- Max-width: ~60ch. Place inside the page side gutters, not edge-to-edge.

### Toast
- `--ink` background, paper-cream text. 3 px terracotta left stripe.
- Layered shadow.

## States & Interactions

- **Hover (rows/cards)**: subtle paper-color shift + occasional translate-up
  by 1 px. Drag-source rows additionally slide right (`padding-left`) and
  reveal an italic arrow `→`.
- **Hover (buttons)**: background brightens one step, border darkens to
  `--ink-3`. No hue shift.
- **Active / selected**: `--accent-soft` background + accent text. For
  navigation, *no fill* — use the 2 px left rule instead.
- **Today indicator**: column gets paper background + terracotta border +
  "TODAY" pill chip. The badge is the brand moment; don't replace it with
  a fill change.
- **Drag-over (drop target)**: sage tint, sage border, 2 px lift. Sage —
  not terracotta — because "you're about to place" is a positive action,
  and reserving terracotta for "today" keeps that moment special.
- **Drag source**: opacity 0.4 + `-1deg` rotation. The dragged card looks
  like it's been lifted off the surface.
- **Loading**: a circular spinner with `--accent-color` top stroke.
- **Focus ring**: 3 px `--accent-soft` glow, 1 px terracotta border.

## Motion

- Transitions default to `0.18s ease`. Hover effects, focus rings, and
  expansions all use this timing.
- Page reveal: each page (`.planner-body`, `#page-workouts`) animates in
  with `pageReveal` — a small 8 px translateY + opacity over 0.5 s on a
  `cubic-bezier(0.22, 0.9, 0.3, 1)` curve. Don't add more entry animations
  on top.
- Modals: 0.25 s translateY-up + fade.
- Card hover lift: max 1–2 px translate. Anything bigger feels like a toy.
- Slide-in arrow on recipe row hover: a tiny 0.18 s `slideIn` keyframe.

## Iconography

- Geometric Unicode marks are still used for chrome icons (`▾ ▤ ◆ ↗ ‹ ›`).
  They render acceptably and avoid an icon-font dependency. **Open
  improvement:** replace with Lucide SVGs at 1.5 px stroke as a coordinated
  migration.
- The terracotta **❦ (floral heart / fleuron)** is the only decorative
  glyph and serves as a brand-mark accent on panel and section headings.
  Use sparingly — at most twice per visible page.
- No emoji in UI chrome. Existing meal data may include 🍗/🍝 but new
  surfaces should avoid them.

## What NOT to do

- ❌ Inter (the regular face) — we moved to Inter Tight + Fraunces.
- ❌ Indigo/purple accents. Anywhere.
- ❌ Pure-white `#FFFFFF` surfaces. Always go cream.
- ❌ Uppercase-tracked eyebrow labels for section titles. Use display italic
  in mixed case instead.
- ❌ Soft-pillow rounded corners ≥ 8 px on cards and panels. Keep things
  printed-edge.
- ❌ Color-shifting hover states that change hue. Lighten/translate, don't
  recolor.
- ❌ Flat `0 4px 8px rgba(0,0,0,0.04)` shadow without a paired 1 px
  hairline. Always layer the two.
- ❌ Backdrop-blur glassmorphism on chrome. The only blur is the 2 px modal
  scrim.
- ❌ Gradients on buttons, cards, or chrome.
- ❌ Dark mode (out of scope until explicitly designed; the warm palette
  needs a deliberate dark counterpart).

## Adding a new screen

1. Copy an existing page directory under `static/pages/` (e.g. `cp -r
   static/pages/workouts static/pages/<name>`) and rename
   `workouts.js` → `<name>.js`.
2. Register the page with the sidebar by adding an entry to the `PAGES`
   array in `static/shared/chrome.js`, and update the `mountChrome` call
   inside your page's HTML to use the new `activePage` id.
3. Add a `CROW_ROUTE(app, "/<name>")` to `src/core/http/static_routes.cpp`
   that serves `static/pages/<name>/index.html`.
4. Use page-header `.page-h` markup verbatim — masthead title + italic
   description.
5. Reference CSS tokens (`--paper-2`, `--ink-2`, `--s-4`, etc.) from
   `/shared/base.css`. Never hard-code hex or px values for color/spacing.
6. Set every section/panel heading in display (Fraunces). Use italic for
   labels and metadata.
7. If a new component is needed, add a section to this file describing its
   tokens, states, and copy norms.
8. Render a 1440×900 screenshot of the new screen into `static/mockups/` so
   the design system stays self-documenting.

## File map

| File                                     | Purpose                                          |
| ---------------------------------------- | ------------------------------------------------ |
| `docs/BRANDING.md`                       | This document — single source of truth           |
| `static/shared/base.css`                 | Design tokens + shared component styles          |
| `static/shared/chrome.js`                | Sidebar + topbar shell injected by every page    |
| `static/pages/<name>/index.html`         | Per-page markup (one document per page)          |
| `static/pages/<name>/<name>.js`          | Per-page logic                                   |
| `static/mockups/`                        | Rendered screenshots per page                    |

When the live app and a mockup diverge, treat this document + the live app
as intent. Old mockups (e.g. the `notion-3-indigo-kanban.*` files from the
previous direction) are historical references only — do **not** use them as
a target.
