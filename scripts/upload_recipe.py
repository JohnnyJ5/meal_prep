#!/usr/bin/env python3
"""
Upload a verified recipe JSON file to the meal prep API.

Usage:
    python3 upload_recipe.py <path-to-json> [--server <url>] [--dry-run]

This script:
1. Reads the JSON file
2. Checks for new ingredients and adds them to the available_ingredients table
3. POSTs the meal to /api/meals/add

The JSON file must have been reviewed by the user before running this script.
"""

import argparse
import json
import os
import sys

try:
    import requests
except ImportError:
    print("ERROR: requests is required. Install with: pip3 install requests", file=sys.stderr)
    sys.exit(1)


DEFAULT_SERVER = "http://localhost:8080"

# Category mapping for new ingredients (best-effort guess)
INGREDIENT_CATEGORY_HINTS = {
    "broth": "Broths & Sauces", "stock": "Broths & Sauces", "sauce": "Broths & Sauces",
    "chicken": "Proteins", "turkey": "Proteins", "beef": "Proteins",
    "pork": "Proteins", "salmon": "Proteins", "shrimp": "Proteins",
    "egg": "Proteins", "tofu": "Proteins",
    "spinach": "Vegetables", "broccoli": "Vegetables", "carrot": "Vegetables",
    "onion": "Vegetables", "pepper": "Vegetables", "tomato": "Vegetables",
    "zucchini": "Vegetables", "celery": "Vegetables", "kale": "Vegetables",
    "garlic": "Produce", "lemon": "Produce", "lime": "Produce",
    "milk": "Dairy", "cream": "Dairy", "cheese": "Dairy", "butter": "Dairy",
    "yogurt": "Dairy", "feta": "Dairy", "parmesan": "Dairy",
    "salt": "Pantry Staples", "pepper": "Pantry Staples", "oil": "Pantry Staples",
    "flour": "Pantry Staples", "sugar": "Pantry Staples", "vinegar": "Pantry Staples",
    "broth": "Broths & Sauces", "stock": "Broths & Sauces", "sauce": "Broths & Sauces",
    "pasta": "Grains & Pasta", "rice": "Grains & Pasta", "orzo": "Grains & Pasta",
    "bread": "Grains & Pasta", "noodle": "Grains & Pasta",
    "cumin": "Seasonings & Spices", "paprika": "Seasonings & Spices",
    "oregano": "Seasonings & Spices", "basil": "Seasonings & Spices",
    "thyme": "Seasonings & Spices", "rosemary": "Seasonings & Spices",
    "seasoning": "Seasonings & Spices", "powder": "Seasonings & Spices",
    "honey": "Sweeteners", "maple": "Sweeteners", "syrup": "Sweeteners",
}


def guess_ingredient_category(name: str) -> str:
    """Guess a category for a new ingredient based on its name."""
    name_lower = name.lower()
    for keyword, category in INGREDIENT_CATEGORY_HINTS.items():
        if keyword in name_lower:
            return category
    return "Other"


def get_existing_ingredients(server: str) -> set:
    """Fetch the current list of available ingredient names from the API."""
    try:
        resp = requests.get(f"{server}/api/ingredients", timeout=10)
        resp.raise_for_status()
        data = resp.json()
        return {ing["name"] for ing in data}
    except requests.RequestException as e:
        print(f"WARNING: Could not fetch existing ingredients: {e}", file=sys.stderr)
        return set()


def add_new_ingredients(server: str, recipe: dict, existing: set, dry_run: bool) -> list:
    """Add any new ingredients from the recipe to the available_ingredients table."""
    added = []
    for ing in recipe.get("ingredients", []):
        name = ing["name"]
        if name not in existing:
            category = guess_ingredient_category(name)
            payload = {"name": name, "category": category}

            if dry_run:
                print(f"  [DRY RUN] Would add ingredient: {name} ({category})")
                added.append(name)
            else:
                try:
                    resp = requests.post(
                        f"{server}/api/ingredients/add",
                        json=payload,
                        timeout=10,
                    )
                    if resp.status_code == 200:
                        print(f"  ✅ Added new ingredient: {name} ({category})")
                        added.append(name)
                    else:
                        print(
                            f"  ⚠️  Failed to add ingredient '{name}': "
                            f"{resp.status_code} {resp.text}",
                            file=sys.stderr,
                        )
                except requests.RequestException as e:
                    print(f"  ❌ Error adding ingredient '{name}': {e}", file=sys.stderr)

    return added


def upload_meal(server: str, recipe: dict, dry_run: bool) -> bool:
    """POST the meal to /api/meals/add."""
    # Build the payload (exclude any extra keys like _raw_text)
    payload = {
        "name": recipe["name"],
        "category": recipe.get("category", "Uncategorized"),
        "ingredients": recipe["ingredients"],
    }

    if dry_run:
        print(f"\n[DRY RUN] Would POST to {server}/api/meals/add:")
        print(json.dumps(payload, indent=2))
        return True

    try:
        resp = requests.post(
            f"{server}/api/meals/add",
            json=payload,
            timeout=10,
        )
        if resp.status_code == 200:
            print(f"\n✅ Meal '{recipe['name']}' uploaded successfully!")
            return True
        else:
            print(
                f"\n❌ Failed to upload meal: {resp.status_code} {resp.text}",
                file=sys.stderr,
            )
            return False
    except requests.RequestException as e:
        print(f"\n❌ Error uploading meal: {e}", file=sys.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Upload a verified recipe JSON to the meal prep API."
    )
    parser.add_argument("json_path", help="Path to the recipe JSON file")
    parser.add_argument(
        "--server", "-s",
        default=DEFAULT_SERVER,
        help=f"API server URL (default: {DEFAULT_SERVER})"
    )
    parser.add_argument(
        "--dry-run", "-n",
        action="store_true",
        help="Print what would be done without actually uploading"
    )
    args = parser.parse_args()

    if not os.path.exists(args.json_path):
        print(f"ERROR: File not found: {args.json_path}", file=sys.stderr)
        sys.exit(1)

    with open(args.json_path) as f:
        recipe = json.load(f)

    # Validate required fields
    if "name" not in recipe:
        print("ERROR: JSON is missing 'name' field", file=sys.stderr)
        sys.exit(1)
    if "ingredients" not in recipe or not recipe["ingredients"]:
        print("ERROR: JSON is missing 'ingredients' or ingredients list is empty", file=sys.stderr)
        sys.exit(1)

    print(f"📦 Recipe: {recipe['name']}")
    print(f"📂 Category: {recipe.get('category', 'Uncategorized')}")
    print(f"🥗 Ingredients: {len(recipe['ingredients'])}")
    print(f"🌐 Server: {args.server}")
    if args.dry_run:
        print("🔍 Mode: DRY RUN\n")
    else:
        print()

    # Step 1: Add new ingredients
    print("Checking for new ingredients...")
    existing = get_existing_ingredients(args.server)
    added = add_new_ingredients(args.server, recipe, existing, args.dry_run)
    if added:
        print(f"  Added {len(added)} new ingredient(s)")
    else:
        print("  All ingredients already exist in database")

    # Step 2: Upload the meal
    print("\nUploading meal...")
    success = upload_meal(args.server, recipe, args.dry_run)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
