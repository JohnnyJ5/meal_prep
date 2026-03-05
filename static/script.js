let selectedMeals = new Set();
let isPlanning = false;

document.addEventListener('DOMContentLoaded', () => {
    fetchMeals();
});

async function fetchMeals() {
    const loadingState = document.getElementById('loading');
    const errorState = document.getElementById('error');
    const mealGrid = document.getElementById('meal-grid');
    const actionBar = document.getElementById('action-bar');

    loadingState.classList.remove('hidden');
    errorState.classList.add('hidden');
    mealGrid.classList.add('hidden');
    actionBar.classList.add('hidden');

    try {
        const response = await fetch('/api/meals');
        if (!response.ok) throw new Error('Network response was not ok');

        const meals = await response.json();
        renderMeals(meals);

        loadingState.classList.add('hidden');
        mealGrid.classList.remove('hidden');
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
            <h3>${emoji} ${formatName(mealId)}</h3>
            <div class="checkbox"></div>
        `;

        card.addEventListener('click', () => toggleMeal(mealId, card));
        grid.appendChild(card);
    });
}

function toggleMeal(mealId, cardElement) {
    if (isPlanning) return;

    if (selectedMeals.has(mealId)) {
        selectedMeals.delete(mealId);
        cardElement.classList.remove('selected');
    } else {
        selectedMeals.add(mealId);
        cardElement.classList.add('selected');
    }

    updateActionBar();
}

function updateActionBar() {
    const countSpan = document.getElementById('selected-count');
    const planBtn = document.getElementById('plan-btn');

    const count = selectedMeals.size;
    countSpan.textContent = count;

    if (count > 0) {
        planBtn.removeAttribute('disabled');
    } else {
        planBtn.setAttribute('disabled', 'true');
    }
}

async function planMeals() {
    if (selectedMeals.size === 0 || isPlanning) return;

    isPlanning = true;
    const btn = document.getElementById('plan-btn');
    const btnText = btn.querySelector('.btn-text');
    const btnLoader = btn.querySelector('.btn-loader');

    // UI Loading state
    btn.setAttribute('disabled', 'true');
    btnText.textContent = 'Generating...';
    btnLoader.classList.remove('hidden');

    try {
        const response = await fetch('/api/plan', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(Array.from(selectedMeals))
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

function closeModal() {
    const modal = document.getElementById('result-modal');
    modal.classList.add('hidden');
}
