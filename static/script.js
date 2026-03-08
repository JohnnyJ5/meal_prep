let selectedMeals = new Set();
let isPlanning = false;

document.addEventListener('DOMContentLoaded', () => {
    fetchMeals();
});

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

function renderMeals(meals) {
    const grid = document.getElementById('meal-grid');
    grid.innerHTML = '';

    // Format meal names (e.g. "baked-chicken-breast" -> "Baked Chicken Breast")
    const formatName = (str) => {
        return str.split('-')
            .map(word => word.charAt(0).toUpperCase() + word.slice(1))
            .join(' ');
    };

    meals.forEach((mealId, index) => {
        const card = document.createElement('div');
        card.className = 'meal-card';
        card.style.animationDelay = `${index * 0.05}s`;

        // Add emoji based on name just for visual flair
        let emoji = '🍲';
        if (mealId.includes('chicken')) emoji = '🍗';
        if (mealId.includes('turkey')) emoji = '🦃';
        if (mealId.includes('pasta') || mealId.includes('penne')) emoji = '🍝';
        if (mealId.includes('burger') || mealId.includes('hamburger')) emoji = '🍔';
        if (mealId.includes('pancake')) emoji = '🥞';

        card.innerHTML = `
            <div class="meal-card-header">
                <h3>${emoji} ${formatName(mealId)}</h3>
            </div>
        `;
        card.id = `meal-${mealId}-${index}`;
        card.setAttribute('data-meal-id', mealId);
        card.setAttribute('draggable', 'true');
        card.setAttribute('ondragstart', 'drag(event)');

        // Add click listener for selection
        card.addEventListener('click', (e) => {
            // Prevent triggering if dragging
            if (card.classList.contains('dragging')) return;

            if (selectedMeals.has(mealId)) {
                selectedMeals.delete(mealId);
                card.classList.remove('selected');
            } else {
                selectedMeals.add(mealId);
                card.classList.add('selected');
            }
            updateActionBar();
        });

        grid.appendChild(card);
    });
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
        const fromGrid = draggedElt.parentElement && draggedElt.parentElement.id === 'meal-grid';
        const toGrid = dropTarget.id === 'meal-grid';
        const toSlot = dropTarget.classList.contains('meal-slot');

        if (fromGrid && toSlot) {
            // Clone when moving from grid to a slot so it remains available
            const clone = draggedElt.cloneNode(true);
            clone.id = draggedElt.id + '-clone-' + Date.now() + '-' + Math.floor(Math.random() * 1000);
            clone.classList.remove('selected', 'dragging');
            dropTarget.appendChild(clone);
        } else if (!fromGrid && toGrid) {
            // If dragging from a slot back to the grid, just remove it from the planner
            draggedElt.remove();
        } else if (toSlot && !fromGrid) {
            // Moving from slot to slot
            dropTarget.appendChild(draggedElt);
            draggedElt.classList.remove('selected');
        }

        updateActionBar();
    }
}

function updateActionBar() {
    const countSpan = document.getElementById('selected-count');
    const planBtn = document.getElementById('plan-btn');
    const viewBtn = document.getElementById('view-ingredients-btn');

    let count = 0;
    const days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
    days.forEach(day => {
        const slot = document.querySelector(`#day-${day} .meal-slot`);
        if (slot) {
            count += slot.querySelectorAll('.meal-card').length;
        }
    });

    const totalSelected = count + selectedMeals.size;
    countSpan.textContent = totalSelected;

    if (totalSelected > 0) {
        planBtn.removeAttribute('disabled');
        if (selectedMeals.size > 0) {
            viewBtn.removeAttribute('disabled');
        } else {
            viewBtn.setAttribute('disabled', 'true');
        }
    } else {
        planBtn.setAttribute('disabled', 'true');
        viewBtn.setAttribute('disabled', 'true');
    }
}

async function planMeals() {
    if (document.getElementById('selected-count').textContent === "0" || isPlanning) return;

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
                    schedule[day].push(c.getAttribute('data-meal-id'));
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
            showResultModal(result.schedule);
            // Optionally clear selections after success
            // selectedMeals.clear();
            // document.querySelectorAll('.meal-card').forEach(c => c.classList.remove('selected'));
            // updateActionBar();
        } else {
            throw new Error(result.error || 'Failed to generate plan');
        }
    } catch (error) {
        console.error('Error starting plan:', error);
        alert('An error occurred while generating the plan. Check console for details.');
    } finally {
        isPlanning = false;
        btn.removeAttribute('disabled');
        btnText.textContent = 'Generate Plan & Send Email';
        btnLoader.classList.add('hidden');
        updateActionBar(); // Re-evaluates disabled state based on selection count
    }
}

function showResultModal(scheduleText) {
    const modal = document.getElementById('result-modal');
    const output = document.getElementById('schedule-output');

    output.textContent = scheduleText;
    modal.classList.remove('hidden');
}

function closeModal(modalId = 'result-modal') {
    const modal = document.getElementById(modalId);
    if (modal) {
        modal.classList.add('hidden');
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
            fetch(`/api/meals/${mealId}`).then(res => res.json())
        );

        const mealsData = await Promise.all(fetchPromises);

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

        meals.forEach(mealId => {
            const item = document.createElement('div');
            item.className = 'manage-item';

            const formatName = str => str.split('-').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');

            item.innerHTML = `
                <div class="manage-item-info">
                    <strong>${formatName(mealId)}</strong>
                    <br><small style="color: var(--text-secondary);">${mealId}</small>
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

    // Fetch the meal details to populate the ingredients form
    try {
        // Since we don't have a GET /api/meals/<name> endpoint, we can find it by fetching the plan
        // Actually, we DO need the ingredients. Looking at main.cpp, we didn't add a GET single meal endpoint!
        // To work around this without adding another endpoint, we'll extract it by a dummy plan call with just this meal
        const response = await fetch('/api/plan', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify([mealId])
        });

        // This is hacky, but the result schedule contains the ingredients, or we can just add a new endpoint to C++ easily!
        // It's better to add a GET /api/meals/<name> to main.cpp! I will do this next. But for now I'll write the JS expecting it:

        const mealRes = await fetch(`/api/meals/${mealId}`);
        if (mealRes.ok) {
            const mealData = await mealRes.json();
            mealData.ingredients.forEach(ing => {
                addIngredientRow(ing.name, ing.amount, ing.unit);
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
    addIngredientRow();
}

function closeEditView() {
    document.getElementById('manage-list-view').classList.remove('hidden');
    document.getElementById('manage-edit-view').classList.add('hidden');
}

function addIngredientRow(name = "", amount = "", unit = 0) {
    const container = document.getElementById('ingredients-list');
    const row = document.createElement('div');
    row.className = 'ingredient-row';

    // Create unit options
    let unitOptions = '';
    for (const [val, label] of Object.entries(UnitEnum)) {
        unitOptions += `<option value="${val}" ${parseInt(val) === unit ? 'selected' : ''}>${label}</option>`;
    }

    row.innerHTML = `
        <input type="text" placeholder="Ingredient Name (e.g., Chicken Breast)" value="${name}" required class="form-control name-input">
        <input type="number" step="0.01" placeholder="Amount" value="${amount}" required class="form-control amount-input">
        <select class="form-control unit-input">
            ${unitOptions}
        </select>
        <button type="button" class="btn btn-danger btn-sm" onclick="this.parentElement.remove()">X</button>
    `;

    container.appendChild(row);
}

async function saveMeal(e) {
    e.preventDefault();

    const name = document.getElementById('meal-name').value.trim().toLowerCase().replace(/\s+/g, '-');
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
            preparation: "None"
        });
    });

    const payload = { name, ingredients };

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
