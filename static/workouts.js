// Workouts page: list, log form, detail view.
// Coexists with planner script.js; assumes DOM is ready when script.js' DOMContentLoaded fires.

(function () {
    let workoutsLoaded = false;
    let currentDetailId = null;

    document.addEventListener('DOMContentLoaded', () => {
        const dateInput = document.getElementById('workout-date');
        if (dateInput) dateInput.valueAsDate = new Date();
    });

    // --- Page switching --------------------------------------------------

    window.showPage = function (page) {
        const planner = document.getElementById('page-planner');
        const workouts = document.getElementById('page-workouts');
        const crumb = document.getElementById('page-crumb');
        const weekLabel = document.getElementById('week-label');
        const navPlanner = document.getElementById('nav-planner');
        const navWorkouts = document.getElementById('nav-workouts');

        if (page === 'workouts') {
            planner.classList.add('hidden');
            workouts.classList.remove('hidden');
            crumb.textContent = 'Workouts';
            if (weekLabel) weekLabel.textContent = 'Log';
            navPlanner.classList.remove('active');
            navWorkouts.classList.add('active');
            if (!workoutsLoaded) {
                fetchWorkouts();
                workoutsLoaded = true;
            }
        } else {
            workouts.classList.add('hidden');
            planner.classList.remove('hidden');
            crumb.textContent = 'Weekly Planner';
            if (weekLabel) weekLabel.textContent = 'This week';
            navWorkouts.classList.remove('active');
            navPlanner.classList.add('active');
        }
    };

    // --- History list ----------------------------------------------------

    async function fetchWorkouts() {
        const list = document.getElementById('workouts-list');
        const loading = document.getElementById('workouts-loading');
        const empty = document.getElementById('workouts-empty');
        loading.classList.remove('hidden');
        list.classList.add('hidden');
        empty.classList.add('hidden');

        try {
            const res = await fetch('/api/workouts');
            if (!res.ok) throw new Error('http ' + res.status);
            const items = await res.json();
            loading.classList.add('hidden');
            if (!items || items.length === 0) {
                empty.classList.remove('hidden');
                list.innerHTML = '';
                return;
            }
            list.classList.remove('hidden');
            renderWorkoutList(items);
        } catch (e) {
            loading.classList.add('hidden');
            list.classList.remove('hidden');
            list.innerHTML = '<p style="color:var(--error-color)">Failed to load workouts.</p>';
            console.error(e);
        }
    }

    function renderWorkoutList(items) {
        const list = document.getElementById('workouts-list');
        list.innerHTML = '';
        items.forEach((w) => {
            const card = document.createElement('div');
            card.className = 'workout-card';
            card.onclick = () => openWorkoutDetail(w.id);
            const title = w.name || w.performed_on;
            card.innerHTML = `
                <div class="workout-card-title">${escapeHtml(title)}</div>
                <div class="workout-card-meta">
                    <span>${escapeHtml(w.performed_on)}</span>
                    <span>•</span>
                    <span>${formatDuration(w.duration_seconds)}</span>
                    <span>•</span>
                    <span>${w.exercise_count} exercise${w.exercise_count === 1 ? '' : 's'}</span>
                </div>`;
            list.appendChild(card);
        });
    }

    // --- Log form --------------------------------------------------------

    let editingWorkoutId = null;

    window.openWorkoutForm = function () {
        editingWorkoutId = null;
        document.getElementById('workout-modal-title').textContent = 'Log Workout';
        document.getElementById('workout-form').reset();
        document.getElementById('workout-date').valueAsDate = new Date();
        document.getElementById('workout-blocks').innerHTML = '';
        addBlock();  // start with one straight block, one empty exercise
        document.getElementById('workout-modal').classList.remove('hidden');
    };

    window.closeWorkoutModal = function () {
        document.getElementById('workout-modal').classList.add('hidden');
    };

    window.closeModal = window.closeModal || function (id) {
        document.getElementById(id).classList.add('hidden');
    };

    window.addBlock = function (preset) {
        const container = document.getElementById('workout-blocks');
        const idx = container.children.length;
        const block = document.createElement('div');
        block.className = 'workout-block';
        block.dataset.idx = idx;
        block.innerHTML = `
            <div class="workout-block-h">
                <div class="block-controls">
                    <label class="block-type-toggle">
                        <select class="form-control block-type" onchange="onBlockTypeChange(this)">
                            <option value="straight">Straight sets</option>
                            <option value="circuit">Circuit</option>
                        </select>
                    </label>
                    <label class="block-rounds-wrap hidden">
                        Repeat <input type="number" class="form-control block-rounds" min="1" value="3" style="width:5em"> rounds
                    </label>
                    <label class="block-rest-wrap">
                        Rest <input type="number" class="form-control block-rest" min="0" value="0" style="width:5em"> <span class="block-rest-label">sec between sets</span>
                    </label>
                </div>
                <button type="button" class="btn-close" onclick="removeBlock(this)" title="Remove block">×</button>
            </div>
            <div class="block-exercises"></div>
            <button type="button" class="btn btn-secondary btn-sm add-exercise-btn" onclick="addExercise(this)">+ Add Exercise</button>
        `;
        container.appendChild(block);
        if (preset) applyBlockPreset(block, preset);
        else addExercise(block.querySelector('.add-exercise-btn'));
    };

    window.removeBlock = function (btn) {
        btn.closest('.workout-block').remove();
    };

    window.onBlockTypeChange = function (sel) {
        const block = sel.closest('.workout-block');
        const roundsWrap = block.querySelector('.block-rounds-wrap');
        const restLabel = block.querySelector('.block-rest-label');
        if (sel.value === 'circuit') {
            roundsWrap.classList.remove('hidden');
            if (restLabel) restLabel.textContent = 'sec between rounds';
        } else {
            roundsWrap.classList.add('hidden');
            if (restLabel) restLabel.textContent = 'sec between sets';
        }
    };

    window.addExercise = function (btn) {
        const block = btn.closest('.workout-block');
        const exList = block.querySelector('.block-exercises');
        const row = document.createElement('div');
        row.className = 'exercise-row';
        row.innerHTML = `
            <div class="exercise-row-h">
                <input type="text" class="form-control ex-name" placeholder="Exercise (e.g., Deadlift)" required style="flex:1">
                <select class="form-control ex-type" onchange="onExerciseTypeChange(this)" style="width: 9em">
                    <option value="reps">Reps</option>
                    <option value="distance">Distance</option>
                    <option value="time">Time</option>
                </select>
                <button type="button" class="btn-close" onclick="removeExercise(this)" title="Remove">×</button>
            </div>
            <div class="exercise-fields ex-fields-reps">
                <label>Sets <input type="number" class="form-control ex-sets" min="0" value="1" style="width:4.5em"></label>
                <label>Reps <input type="number" class="form-control ex-reps" min="0" value="10" style="width:4.5em"></label>
                <label>Weight (lbs) <input type="number" step="0.5" class="form-control ex-weight" min="0" value="0" style="width:5.5em"></label>
                <label>Rest (sec) <input type="number" class="form-control ex-rest" min="0" value="0" style="width:5em"></label>
            </div>
            <div class="exercise-fields ex-fields-distance hidden">
                <label>Distance <input type="number" step="0.01" class="form-control ex-distance" min="0" value="0.25" style="width:6em"></label>
                <label>Unit
                    <select class="form-control ex-distance-unit" style="width:5em">
                        <option value="mi">mi</option>
                        <option value="km">km</option>
                        <option value="m">m</option>
                    </select>
                </label>
                <label>Duration (mm:ss) <input type="text" class="form-control ex-duration" placeholder="optional" pattern="\\d{0,3}:?\\d{0,2}" style="width:6em"></label>
            </div>
            <div class="exercise-fields ex-fields-time hidden">
                <label>Duration (mm:ss) <input type="text" class="form-control ex-duration-time" placeholder="1:00" pattern="\\d{0,3}:?\\d{0,2}" style="width:6em"></label>
                <label>Sets <input type="number" class="form-control ex-sets-time" min="0" value="1" style="width:4.5em"></label>
                <label>Rest (sec) <input type="number" class="form-control ex-rest-time" min="0" value="0" style="width:5em"></label>
            </div>
        `;
        exList.appendChild(row);
    };

    window.removeExercise = function (btn) {
        btn.closest('.exercise-row').remove();
    };

    window.onExerciseTypeChange = function (sel) {
        const row = sel.closest('.exercise-row');
        const type = sel.value;
        row.querySelectorAll('.exercise-fields').forEach((el) => el.classList.add('hidden'));
        const target = row.querySelector('.ex-fields-' + type);
        if (target) target.classList.remove('hidden');
    };

    function collectWorkoutFromForm() {
        const name = document.getElementById('workout-name').value.trim();
        const performed_on = document.getElementById('workout-date').value;
        const duration_seconds = parseDurationToSeconds(document.getElementById('workout-duration').value);
        const notes = document.getElementById('workout-notes').value.trim();

        const blocks = [];
        document.querySelectorAll('#workout-blocks .workout-block').forEach((blockEl) => {
            const type = blockEl.querySelector('.block-type').value;
            const rounds = type === 'circuit'
                ? Math.max(1, parseInt(blockEl.querySelector('.block-rounds').value || '1', 10))
                : 1;
            const rest_seconds = parseInt(blockEl.querySelector('.block-rest').value || '0', 10);

            const exercises = [];
            blockEl.querySelectorAll('.exercise-row').forEach((row) => {
                const exName = row.querySelector('.ex-name').value.trim();
                if (!exName) return;
                const exType = row.querySelector('.ex-type').value;
                const ex = { name: exName, exercise_type: exType };
                if (exType === 'reps') {
                    ex.sets = intVal(row, '.ex-sets');
                    ex.reps = intVal(row, '.ex-reps');
                    ex.weight_lbs = floatVal(row, '.ex-weight');
                    ex.rest_seconds = intVal(row, '.ex-rest');
                } else if (exType === 'distance') {
                    ex.distance = floatVal(row, '.ex-distance');
                    ex.distance_unit = row.querySelector('.ex-distance-unit').value;
                    const durStr = row.querySelector('.ex-duration').value;
                    if (durStr) ex.duration_seconds = parseDurationToSeconds(durStr);
                } else if (exType === 'time') {
                    ex.duration_seconds = parseDurationToSeconds(row.querySelector('.ex-duration-time').value);
                    ex.sets = intVal(row, '.ex-sets-time');
                    ex.rest_seconds = intVal(row, '.ex-rest-time');
                }
                exercises.push(ex);
            });
            if (exercises.length > 0) {
                blocks.push({ block_type: type, rounds, rest_seconds, exercises });
            }
        });

        return { name, performed_on, duration_seconds, notes, blocks };
    }

    window.saveWorkout = async function (event) {
        event.preventDefault();
        const payload = collectWorkoutFromForm();
        if (payload.blocks.length === 0) {
            alert('Add at least one exercise.');
            return;
        }
        try {
            const url = editingWorkoutId ? '/api/workouts/' + editingWorkoutId : '/api/workouts';
            const method = editingWorkoutId ? 'PUT' : 'POST';
            const res = await fetch(url, {
                method,
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload),
            });
            if (!res.ok) throw new Error('http ' + res.status);
            closeWorkoutModal();
            fetchWorkouts();
        } catch (e) {
            alert('Failed to save workout: ' + e.message);
        }
    };

    // --- Detail view -----------------------------------------------------

    async function openWorkoutDetail(id) {
        currentDetailId = id;
        try {
            const res = await fetch('/api/workouts/' + id);
            if (!res.ok) throw new Error('http ' + res.status);
            const w = await res.json();
            renderDetail(w);
            document.getElementById('workout-detail-modal').classList.remove('hidden');
        } catch (e) {
            alert('Failed to load workout: ' + e.message);
        }
    }
    window.openWorkoutDetail = openWorkoutDetail;

    function renderDetail(w) {
        document.getElementById('workout-detail-title').textContent = w.name || w.performed_on;
        const body = document.getElementById('workout-detail-body');
        const parts = [];
        parts.push(`<div class="detail-meta">
            <span><b>Date:</b> ${escapeHtml(w.performed_on)}</span>
            <span><b>Duration:</b> ${formatDuration(w.duration_seconds)}</span>
        </div>`);
        if (w.notes) {
            parts.push(`<div class="detail-notes"><b>Notes:</b> ${escapeHtml(w.notes)}</div>`);
        }
        (w.blocks || []).forEach((b, bi) => {
            const heading = b.block_type === 'circuit'
                ? `Circuit — ${b.rounds} round${b.rounds === 1 ? '' : 's'}` + (b.rest_seconds ? ` (rest ${b.rest_seconds}s)` : '')
                : 'Straight sets' + (b.rest_seconds ? ` (rest ${b.rest_seconds}s)` : '');
            parts.push(`<div class="detail-block"><h4>Block ${bi + 1}: ${heading}</h4><ul>`);
            (b.exercises || []).forEach((e) => {
                parts.push(`<li>${formatExerciseDetail(e)}</li>`);
            });
            parts.push('</ul></div>');
        });
        body.innerHTML = parts.join('');
    }

    function formatExerciseDetail(e) {
        const name = escapeHtml(e.name || '');
        if (e.exercise_type === 'reps') {
            let line = `<b>${name}</b> — ${e.sets || 1} × ${e.reps || 0}`;
            if (e.weight_lbs && e.weight_lbs > 0) line += ` @ ${e.weight_lbs} lbs`;
            if (e.rest_seconds) line += ` <span class="muted">(rest ${e.rest_seconds}s)</span>`;
            return line;
        }
        if (e.exercise_type === 'distance') {
            let line = `<b>${name}</b> — ${e.distance} ${escapeHtml(e.distance_unit || '')}`;
            if (e.duration_seconds) line += ` in ${formatDuration(e.duration_seconds)}`;
            return line;
        }
        if (e.exercise_type === 'time') {
            let line = `<b>${name}</b> — ${formatDuration(e.duration_seconds || 0)}`;
            if (e.sets && e.sets > 1) line += ` × ${e.sets}`;
            if (e.rest_seconds) line += ` <span class="muted">(rest ${e.rest_seconds}s)</span>`;
            return line;
        }
        return `<b>${name}</b>`;
    }

    window.editCurrentWorkout = async function () {
        if (!currentDetailId) return;
        const res = await fetch('/api/workouts/' + currentDetailId);
        if (!res.ok) return alert('Could not load workout');
        const w = await res.json();
        document.getElementById('workout-detail-modal').classList.add('hidden');

        editingWorkoutId = w.id;
        document.getElementById('workout-modal-title').textContent = 'Edit Workout';
        document.getElementById('workout-name').value = w.name || '';
        document.getElementById('workout-date').value = w.performed_on || '';
        document.getElementById('workout-duration').value = formatDuration(w.duration_seconds || 0);
        document.getElementById('workout-notes').value = w.notes || '';
        document.getElementById('workout-blocks').innerHTML = '';
        (w.blocks || []).forEach((b) => addBlock(b));
        document.getElementById('workout-modal').classList.remove('hidden');
    };

    window.deleteCurrentWorkout = async function () {
        if (!currentDetailId) return;
        if (!confirm('Delete this workout?')) return;
        const res = await fetch('/api/workouts/' + currentDetailId, { method: 'DELETE' });
        if (!res.ok) return alert('Failed to delete');
        document.getElementById('workout-detail-modal').classList.add('hidden');
        fetchWorkouts();
    };

    function applyBlockPreset(blockEl, preset) {
        const typeSelect = blockEl.querySelector('.block-type');
        typeSelect.value = preset.block_type || 'straight';
        onBlockTypeChange(typeSelect);
        if (preset.rounds) blockEl.querySelector('.block-rounds').value = preset.rounds;
        if (preset.rest_seconds) blockEl.querySelector('.block-rest').value = preset.rest_seconds;

        const exList = blockEl.querySelector('.block-exercises');
        exList.innerHTML = '';
        (preset.exercises || []).forEach((e) => {
            addExercise(blockEl.querySelector('.add-exercise-btn'));
            const row = exList.lastElementChild;
            row.querySelector('.ex-name').value = e.name || '';
            const typeSel = row.querySelector('.ex-type');
            typeSel.value = e.exercise_type || 'reps';
            onExerciseTypeChange(typeSel);
            if (e.exercise_type === 'reps') {
                row.querySelector('.ex-sets').value = e.sets || 0;
                row.querySelector('.ex-reps').value = e.reps || 0;
                row.querySelector('.ex-weight').value = e.weight_lbs || 0;
                row.querySelector('.ex-rest').value = e.rest_seconds || 0;
            } else if (e.exercise_type === 'distance') {
                row.querySelector('.ex-distance').value = e.distance || 0;
                row.querySelector('.ex-distance-unit').value = e.distance_unit || 'mi';
                row.querySelector('.ex-duration').value = e.duration_seconds
                    ? formatDuration(e.duration_seconds) : '';
            } else if (e.exercise_type === 'time') {
                row.querySelector('.ex-duration-time').value = formatDuration(e.duration_seconds || 0);
                row.querySelector('.ex-sets-time').value = e.sets || 0;
                row.querySelector('.ex-rest-time').value = e.rest_seconds || 0;
            }
        });
        if (exList.children.length === 0) {
            addExercise(blockEl.querySelector('.add-exercise-btn'));
        }
    }

    // --- Helpers ---------------------------------------------------------

    function intVal(row, sel) {
        const el = row.querySelector(sel);
        return el ? parseInt(el.value || '0', 10) : 0;
    }
    function floatVal(row, sel) {
        const el = row.querySelector(sel);
        return el ? parseFloat(el.value || '0') : 0;
    }

    function parseDurationToSeconds(text) {
        if (!text) return 0;
        const parts = String(text).split(':');
        if (parts.length === 1) return parseInt(parts[0], 10) || 0;
        const m = parseInt(parts[0], 10) || 0;
        const s = parseInt(parts[1], 10) || 0;
        return m * 60 + s;
    }

    function formatDuration(sec) {
        sec = Math.max(0, parseInt(sec, 10) || 0);
        const m = Math.floor(sec / 60);
        const s = sec % 60;
        return m + ':' + (s < 10 ? '0' : '') + s;
    }

    function escapeHtml(s) {
        return String(s == null ? '' : s)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;');
    }
})();
