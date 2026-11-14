#!/usr/bin/env python3
"""Idempotent script to deduplicate and normalize ingredient names in meals.db."""

import argparse
from datetime import datetime
import os
import shutil
import sqlite3
import sys

# Map of lowercase source ingredient name -> (canonical_name, optional_prep_override)
# If optional_prep_override is not None and current preparation == 'None',
# preparation is updated to optional_prep_override.
CANONICAL_MAP = {
    # Exact case-only duplicates
    "all-purpose flour": ("All-Purpose Flour", None),
    "baking powder": ("Baking Powder", None),
    "baking soda": ("Baking Soda", None),
    "blueberries": ("Blueberries", None),
    "breadcrumbs": ("Breadcrumbs", None),
    "cinnamon": ("Cinnamon", None),
    "cream cheese": ("Cream Cheese", None),
    "garlic": ("Garlic", None),
    "greek yogurt": ("Greek Yogurt", None),
    "honey": ("Honey", None),
    "maple syrup": ("Maple Syrup", None),
    "milk": ("Milk", None),
    "olive oil": ("Olive Oil", None),
    "onion": ("Onion", None),
    "paprika": ("Paprika", None),
    "parmesan cheese": ("Parmesan Cheese", None),
    "peanut butter": ("Peanut Butter", None),
    "rolled oats": ("Rolled Oats", None),
    "sun-dried tomatoes": ("Sun-Dried Tomatoes", None),
    "vanilla extract": ("Vanilla Extract", None),

    # Typos, spelling & abbreviations
    "low sodumn chicken broth": ("Reduced Sodium Chicken Broth", None),
    "cornstarch": ("Corn Starch", None),
    "mayo": ("Mayonnaise", None),

    # Chicken (user rule: anything that isn't chicken thighs -> Chicken Breast)
    "chicken": ("Chicken Breast", None),
    "boneless skinless chicken breast": ("Chicken Breast", None),

    # Salt (user rule: merge all -> Salt)
    "salt": ("Salt", None),
    "kosher salt": ("Salt", None),
    "sea salt": ("Salt", None),

    # Pepper (user rule: merge all -> Pepper)
    "black pepper": ("Pepper", None),
    "ground black pepper": ("Pepper", None),

    # Dairy (user rule: Butter; Almond Milk; Heavy Cream)
    "unsalted butter": ("Butter", None),
    "unsweetened almond milk": ("Almond Milk", None),
    "heavy whipping cream": ("Heavy Cream", None),
    "whole milk cottage cheese": ("Cottage Cheese", None),
    "plain whole milk cottage cheese": ("Cottage Cheese", None),
    "grated parmesan cheese": ("Parmesan Cheese", "Grated"),

    # Eggs
    "egg": ("Eggs", None),
    "eggs": ("Eggs", None),
    "large egg": ("Eggs", None),
    "large eggs": ("Eggs", None),

    # Ground beef / flour / pantry / herbs / produce
    "80% lean ground beef": ("Ground Beef", None),
    "plain flour": ("All-Purpose Flour", None),
    "mini chocolate chips": ("Chocolate Chips", None),
    "natural peanut butter": ("Peanut Butter", None),
    "ground flaxseed": ("Flax Seed", "Ground"),
    "ground cinnamon": ("Cinnamon", None),
    "dried oregano": ("Oregano", "Dried"),
    "fresh basil leaves": ("Fresh Basil", None),
    "fresh italian flat leaf parsley": ("Parsley", "Fresh"),
    "garlic cloves": ("Garlic", None),
    "fresh blueberries": ("Blueberries", None),
    "banana": ("Banana", None),
    "mashed banana": ("Banana", "Mashed"),
    "boiling water": ("Water", "Boiling"),
    "water": ("Water", None),

    # Capitalize remaining meal-only ingredients cleanly
    "chicken stock cube": ("Chicken Stock Cube", None),
    "chorizo": ("Chorizo", None),
    "coconut oil": ("Coconut Oil", None),
    "coconut sugar": ("Coconut Sugar", None),
    "dried chilli flakes": ("Dried Chilli Flakes", None),
    "gluten-free protein oats": ("Gluten-Free Protein Oats", None),
    "mixed berries": ("Mixed Berries", None),
    "prenatal protein powder": ("Prenatal Protein Powder", None),
    "rice": ("Rice", None),
    "tostones": ("Tostones", None),
    "vanilla bean paste": ("Vanilla Bean Paste", None),
    "vanilla protein powder": ("Vanilla Protein Powder", None),
    "veggies": ("Veggies", None),
    "walnuts": ("Walnuts", None),
}

# Test/placeholder entries to remove from available_ingredients
TEST_INGREDIENTS = {"Test Ingredient", "Verification Ingredient"}

# Categories for canonical meal ingredients not yet in available_ingredients
MISSING_CATEGORIES = {
    "Banana": "Produce",
    "Chicken Stock Cube": "Broths & Sauces",
    "Chorizo": "Proteins",
    "Coconut Oil": "Pantry Staples",
    "Coconut Sugar": "Sweeteners",
    "Cream Cheese": "Dairy",
    "Dried Chilli Flakes": "Seasonings & Spices",
    "Fresh Basil": "Produce",
    "Frozen Broccoli": "Vegetables",
    "Gluten-Free Protein Oats": "Grains & Pasta",
    "Marinara Sauce": "Broths & Sauces",
    "Mini Shell Pasta": "Grains & Pasta",
    "Mixed Berries": "Produce",
    "Neutral Oil": "Pantry Staples",
    "Prenatal Protein Powder": "Pantry Staples",
    "Reduced Sodium Chicken Broth": "Broths & Sauces",
    "Rice": "Grains & Pasta",
    "Sesame Oil": "Pantry Staples",
    "Shredded Cheddar Cheese": "Dairy",
    "Sun-Dried Tomatoes": "Produce",
    "Tostones": "Other",
    "Vanilla Bean Paste": "Seasonings & Spices",
    "Vanilla Protein Powder": "Pantry Staples",
    "Veggies": "Vegetables",
    "Walnuts": "Pantry Staples",
    "Water": "Other",
}


def deduplicate(db_path: str, dry_run: bool = False) -> None:
    if not os.path.exists(db_path):
        print(f"Error: database not found at {db_path}", file=sys.stderr)
        sys.exit(1)

    if not dry_run:
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_path = f"{db_path}.bak.{ts}"
        shutil.copy2(db_path, backup_path)
        print(f"Created backup: {backup_path}")

    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    # 1. Update ingredients table
    rows = cur.execute("SELECT id, name, preparation FROM ingredients").fetchall()
    ing_updates = 0
    for row_id, name, prep in rows:
        key = name.strip().lower()
        new_name = name.strip()
        new_prep = prep
        if key in CANONICAL_MAP:
            new_name, prep_override = CANONICAL_MAP[key]
            if prep_override and (not prep or prep == "None"):
                new_prep = prep_override
        if new_name != name or new_prep != prep:
            ing_updates += 1
            print(f"  [ingredients #{row_id}] '{name}' (prep={prep}) -> '{new_name}' (prep={new_prep})")
            if not dry_run:
                cur.execute(
                    "UPDATE ingredients SET name = ?, preparation = ? WHERE id = ?",
                    (new_name, new_prep, row_id),
                )

    # 2. Clean up available_ingredients (test entries, renames/merges)
    avail_rows = cur.execute("SELECT id, name, category FROM available_ingredients").fetchall()
    existing_avail_names = {r[1] for r in avail_rows}
    avail_changes = 0

    for row_id, name, category in avail_rows:
        if name in TEST_INGREDIENTS:
            avail_changes += 1
            print(f"  [available_ingredients #{row_id}] DELETE test entry '{name}'")
            if not dry_run:
                cur.execute("DELETE FROM available_ingredients WHERE id = ?", (row_id,))
            existing_avail_names.discard(name)
            continue

        key = name.strip().lower()
        if key in CANONICAL_MAP:
            target_name, _ = CANONICAL_MAP[key]
            if target_name != name:
                avail_changes += 1
                if target_name in existing_avail_names:
                    print(f"  [available_ingredients #{row_id}] DELETE duplicate '{name}' (canonical '{target_name}' exists)")
                    if not dry_run:
                        cur.execute("DELETE FROM available_ingredients WHERE id = ?", (row_id,))
                    existing_avail_names.discard(name)
                else:
                    print(f"  [available_ingredients #{row_id}] RENAME '{name}' -> '{target_name}'")
                    if not dry_run:
                        cur.execute(
                            "UPDATE available_ingredients SET name = ? WHERE id = ?",
                            (target_name, row_id),
                        )
                    existing_avail_names.discard(name)
                    existing_avail_names.add(target_name)

    # 3. Add missing canonical meal ingredients to available_ingredients
    if dry_run:
        # Simulate post-update distinct ingredient names
        distinct_ings = set()
        for _, name, _ in rows:
            key = name.strip().lower()
            distinct_ings.add(CANONICAL_MAP[key][0] if key in CANONICAL_MAP else name.strip())
    else:
        distinct_ings = {r[0] for r in cur.execute("SELECT DISTINCT name FROM ingredients").fetchall()}

    avail_lower = {n.lower() for n in existing_avail_names}
    added = 0
    for ing_name in sorted(distinct_ings):
        if ing_name.lower() not in avail_lower:
            cat = MISSING_CATEGORIES.get(ing_name, "Uncategorized")
            added += 1
            print(f"  [available_ingredients] INSERT '{ing_name}' ({cat})")
            if not dry_run:
                cur.execute(
                    "INSERT INTO available_ingredients (name, category) VALUES (?, ?)",
                    (ing_name, cat),
                )
            avail_lower.add(ing_name.lower())

    if not dry_run:
        conn.commit()
    conn.close()

    mode = "DRY RUN" if dry_run else "APPLIED"
    print(f"\n[{mode}] ingredients updated: {ing_updates}, available_ingredients modified/deleted: {avail_changes}, added: {added}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Deduplicate ingredients in meals.db")
    parser.add_argument("--db", default="meals.db", help="Path to SQLite database")
    parser.add_argument("--dry-run", action="store_true", help="Preview changes without modifying DB")
    args = parser.parse_args()
    deduplicate(args.db, dry_run=args.dry_run)
