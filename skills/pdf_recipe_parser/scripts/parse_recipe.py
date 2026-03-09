#!/usr/bin/env python3
"""
Parse a recipe PDF and extract structured data into a JSON payload
compatible with the /api/meals/add endpoint.

Usage:
    python3 parse_recipe.py <path-to-pdf> [--output <output-path>]

Output JSON format:
{
    "name": "recipe-name",
    "category": "Category",
    "ingredients": [
        {"name": "Ingredient", "amount": 1.0, "unit": 2, "preparation": "None"}
    ]
}
"""

import argparse
import json
import os
import re
import sys

try:
    from PyPDF2 import PdfReader
except ImportError:
    print("ERROR: PyPDF2 is required. Install with: pip3 install PyPDF2", file=sys.stderr)
    sys.exit(1)


# MeasurementUnit enum mapping (matches C++ MeasurementUnit in measurement.h)
UNIT_MAP = {
    # Grams
    "gram": 0, "grams": 0, "g": 0,
    # Ounces
    "ounce": 1, "ounces": 1, "oz": 1,
    # Cups
    "cup": 2, "cups": 2,
    # Tablespoons
    "tablespoon": 3, "tablespoons": 3, "tbsp": 3, "tbs": 3,
    # Teaspoons
    "teaspoon": 4, "teaspoons": 4, "tsp": 4,
    # Pounds
    "pound": 5, "pounds": 5, "lb": 5, "lbs": 5,
    # Whole
    "whole": 6,
    # Half
    "half": 7,
    # Small
    "small": 8,
    # Clove
    "clove": 9, "cloves": 9,
    # Head
    "head": 10, "heads": 10,
}

# Common fraction mappings
FRACTION_MAP = {
    "½": 0.5, "⅓": 1/3, "⅔": 2/3, "¼": 0.25, "¾": 0.75,
    "⅛": 0.125, "⅜": 0.375, "⅝": 0.625, "⅞": 0.875,
    "1/2": 0.5, "1/3": 1/3, "2/3": 2/3, "1/4": 0.25, "3/4": 0.75,
    "1/8": 0.125, "3/8": 0.375, "5/8": 0.625, "7/8": 0.875,
}

# Common category keywords
CATEGORY_KEYWORDS = {
    "chicken": "Poultry", "turkey": "Poultry", "poultry": "Poultry",
    "beef": "Beef", "steak": "Beef", "hamburger": "Beef",
    "pork": "Pork", "bacon": "Pork", "ham": "Pork",
    "fish": "Seafood", "salmon": "Seafood", "shrimp": "Seafood", "tuna": "Seafood",
    "soup": "Soup", "stew": "Soup", "chili": "Soup",
    "pasta": "Pasta", "spaghetti": "Pasta", "orzo": "Pasta", "penne": "Pasta",
    "salad": "Salad",
    "breakfast": "Breakfast", "pancake": "Breakfast", "waffle": "Breakfast",
    "vegetarian": "Vegetarian", "vegan": "Vegetarian",
}


def extract_text_from_pdf(pdf_path: str) -> str:
    """Extract all text from a PDF file.

    First tries PyPDF2 direct text extraction. If the PDF is image-based
    (no embedded text), falls back to OCR using pdftoppm + tesseract.
    """
    reader = PdfReader(pdf_path)
    text = ""
    for page in reader.pages:
        page_text = page.extract_text()
        if page_text:
            text += page_text + "\n"

    if text.strip():
        return text

    # Fallback: OCR via pdftoppm + tesseract
    print("PDF has no embedded text. Attempting OCR...", file=sys.stderr)
    try:
        import subprocess
        import tempfile

        with tempfile.TemporaryDirectory() as tmpdir:
            prefix = os.path.join(tmpdir, "page")
            subprocess.run(
                ["pdftoppm", pdf_path, prefix, "-png"],
                check=True, capture_output=True, timeout=30,
            )
            # Process each page image
            page_files = sorted(
                f for f in os.listdir(tmpdir) if f.endswith(".png")
            )
            for page_file in page_files:
                page_path = os.path.join(tmpdir, page_file)
                result = subprocess.run(
                    ["tesseract", page_path, "-"],
                    capture_output=True, text=True, timeout=30,
                )
                if result.returncode == 0:
                    text += result.stdout + "\n"

        if text.strip():
            print("OCR extraction successful.", file=sys.stderr)
        else:
            print("OCR produced no text.", file=sys.stderr)
    except FileNotFoundError:
        print(
            "OCR tools not found. Install with: "
            "sudo apt-get install tesseract-ocr poppler-utils",
            file=sys.stderr,
        )
    except subprocess.TimeoutExpired:
        print("OCR timed out.", file=sys.stderr)
    except Exception as e:
        print(f"OCR failed: {e}", file=sys.stderr)

    return text


def parse_fraction(text: str) -> float:
    """Parse a fraction string into a float value."""
    text = text.strip()

    # Check unicode fractions
    for frac_str, frac_val in FRACTION_MAP.items():
        if text == frac_str:
            return frac_val

    # Check "X/Y" format
    match = re.match(r"^(\d+)/(\d+)$", text)
    if match:
        return int(match.group(1)) / int(match.group(2))

    # Try float
    try:
        return float(text)
    except ValueError:
        return 0.0


def parse_amount(text: str) -> float:
    """Parse an amount string that may contain whole numbers and fractions.

    Examples: '1', '1/2', '1 1/2', '½', '1½', '2.5'
    """
    text = text.strip()
    if not text:
        return 0.0

    # Handle "1½" style (digit immediately followed by unicode fraction)
    for frac_str, frac_val in FRACTION_MAP.items():
        if len(frac_str) == 1 and frac_str in text:
            parts = text.split(frac_str)
            whole = 0.0
            if parts[0].strip():
                try:
                    whole = float(parts[0].strip())
                except ValueError:
                    pass
            return whole + frac_val

    # Handle "1 1/2" style (whole number + space + fraction)
    match = re.match(r"^(\d+)\s+(\d+/\d+)$", text)
    if match:
        return float(match.group(1)) + parse_fraction(match.group(2))

    return parse_fraction(text)


def detect_unit(text: str) -> tuple:
    """Detect measurement unit from text.

    Returns (unit_int, remaining_text) where remaining_text is the ingredient name.
    """
    text = text.strip()

    # Try to match unit at the beginning of the text
    for unit_str, unit_int in sorted(UNIT_MAP.items(), key=lambda x: -len(x[0])):
        pattern = re.compile(r"^" + re.escape(unit_str) + r"s?\b\.?\s*", re.IGNORECASE)
        match = pattern.match(text)
        if match:
            return unit_int, text[match.end():].strip()

    # Check for parenthesized units like "(14.5 oz)"
    # This is handled elsewhere if needed

    # Default to WHOLE
    return 6, text


def detect_preparation(name: str) -> tuple:
    """Extract preparation instructions from ingredient name.

    Returns (clean_name, preparation).
    Common patterns: 'diced onion', 'onion, diced', 'minced garlic'
    """
    prep_words = [
        "chopped", "diced", "minced", "sliced", "grated", "shredded",
        "crushed", "peeled", "thin sliced", "thinly sliced", "julienned",
        "cubed", "halved", "quartered", "torn", "melted", "softened",
        "divided", "packed", "sifted", "beaten", "whisked",
    ]

    name_lower = name.lower().strip()

    # Check for ", prep" pattern (e.g., "onion, diced")
    if "," in name:
        parts = name.split(",", 1)
        prep_candidate = parts[1].strip().lower()
        for pw in prep_words:
            if prep_candidate.startswith(pw):
                return parts[0].strip(), parts[1].strip().title()

    # Check for "prep name" pattern at the start (e.g., "diced onion")
    for pw in prep_words:
        if name_lower.startswith(pw + " "):
            clean_name = name[len(pw):].strip()
            return clean_name, pw.title()

    return name.strip(), "None"


def detect_category(recipe_name: str, text: str) -> str:
    """Guess the recipe category from the name and text content."""
    combined = (recipe_name + " " + text).lower()
    for keyword, category in CATEGORY_KEYWORDS.items():
        if keyword in combined:
            return category
    return "Uncategorized"


def to_kebab_case(name: str) -> str:
    """Convert a recipe name to kebab-case."""
    # Remove special characters, keep alphanumeric and spaces
    clean = re.sub(r"[^\w\s-]", "", name)
    # Replace whitespace with hyphens and lowercase
    return re.sub(r"[\s_]+", "-", clean).strip("-").lower()


def parse_ingredients_from_text(text: str) -> list:
    """Parse ingredient lines from extracted PDF text.

    Expects lines in formats like:
    - '2 cups orzo pasta'
    - '1/2 teaspoon salt'
    - '3 cloves garlic, minced'
    - '1 (14.5 oz) can diced tomatoes'
    """
    ingredients = []
    lines = text.strip().split("\n")

    # Regex to match ingredient lines:
    # Starts with a number (or fraction), followed by unit and name
    ingredient_pattern = re.compile(
        r"^[•\-\*\d½⅓⅔¼¾⅛⅜⅝⅞]"
    )

    amount_pattern = re.compile(
        r"^[•\-\*\s]*"                          # Optional bullet/dash
        r"([\d½⅓⅔¼¾⅛⅜⅝⅞]+(?:\s*[\d/½⅓⅔¼¾⅛⅜⅝⅞]+)?)"  # Amount (number/fraction)
        r"\s+"                                    # Space
        r"(.+)$"                                  # Rest of line (unit + name)
    )

    for line in lines:
        line = line.strip()
        if not line or not ingredient_pattern.match(line):
            continue

        # Try to parse as an ingredient line
        match = amount_pattern.match(line)
        if not match:
            continue

        amount_str = match.group(1)
        rest = match.group(2)

        amount = parse_amount(amount_str)
        if amount <= 0:
            continue

        # Handle parenthesized amounts like "(14.5 oz) can"
        paren_match = re.match(r"\((\d+\.?\d*)\s*(\w+)\)\s*(.*)", rest)
        if paren_match:
            # e.g., "(14.5 oz) can diced tomatoes"
            inner_amount = float(paren_match.group(1))
            inner_unit_str = paren_match.group(2).lower()
            remaining = paren_match.group(3)
            amount = amount * inner_amount
            unit_int = UNIT_MAP.get(inner_unit_str, 6)
            # Remove "can" or container words from name
            remaining = re.sub(r"^(can|jar|bag|box|package|container|bottle)\s+", "", remaining, flags=re.IGNORECASE)
            name, preparation = detect_preparation(remaining)
        else:
            unit_int, remaining = detect_unit(rest)
            name, preparation = detect_preparation(remaining)

        if not name:
            continue

        # Title-case the ingredient name
        name = name.strip().title()

        ingredients.append({
            "name": name,
            "amount": round(amount, 3),
            "unit": unit_int,
            "preparation": preparation,
        })

    return ingredients


def extract_recipe_name(text: str, pdf_filename: str) -> str:
    """Try to extract the recipe name from the PDF text or filename."""
    lines = [l.strip() for l in text.strip().split("\n") if l.strip()]

    # Use the first non-empty line as the recipe name (common for recipe PDFs)
    if lines:
        first_line = lines[0]
        # If it looks like a title (not too long, not a number)
        if len(first_line) < 80 and not first_line[0].isdigit():
            return first_line

    # Fall back to filename
    basename = os.path.splitext(os.path.basename(pdf_filename))[0]
    return basename


def parse_recipe(pdf_path: str) -> dict:
    """Parse a recipe PDF into a structured JSON-compatible dict."""
    text = extract_text_from_pdf(pdf_path)

    if not text.strip():
        print(f"WARNING: No text could be extracted from {pdf_path}", file=sys.stderr)
        print("The PDF may be image-based. Manual entry may be required.", file=sys.stderr)

    recipe_name = extract_recipe_name(text, pdf_path)
    kebab_name = to_kebab_case(recipe_name)
    category = detect_category(recipe_name, text)
    ingredients = parse_ingredients_from_text(text)

    return {
        "name": kebab_name,
        "category": category,
        "ingredients": ingredients,
        "_raw_text": text,  # Included for debugging; remove before upload
    }


def main():
    parser = argparse.ArgumentParser(
        description="Parse a recipe PDF into JSON for the meal prep API."
    )
    parser.add_argument("pdf_path", help="Path to the recipe PDF file")
    parser.add_argument(
        "--output", "-o",
        help="Output JSON file path (default: recipes/<recipe-name>.json)"
    )
    args = parser.parse_args()

    if not os.path.exists(args.pdf_path):
        print(f"ERROR: File not found: {args.pdf_path}", file=sys.stderr)
        sys.exit(1)

    recipe = parse_recipe(args.pdf_path)

    # Remove raw text for the output file (keep for debugging if needed)
    raw_text = recipe.pop("_raw_text", "")

    # Determine output path
    if args.output:
        output_path = args.output
    else:
        recipes_dir = os.path.join(os.path.dirname(args.pdf_path) or ".", "")
        # If the PDF is in a recipes/ folder, output there too
        if "recipes" in os.path.abspath(args.pdf_path):
            recipes_dir = os.path.dirname(os.path.abspath(args.pdf_path))
        output_path = os.path.join(recipes_dir, f"{recipe['name']}.json")

    # Write JSON
    with open(output_path, "w") as f:
        json.dump(recipe, f, indent=2)

    # Print to stdout for quick inspection
    print(json.dumps(recipe, indent=2))
    print(f"\n✅ JSON written to: {output_path}", file=sys.stderr)

    if not recipe["ingredients"]:
        print(
            "\n⚠️  No ingredients were automatically extracted.",
            file=sys.stderr,
        )
        print(
            "You will need to manually add them to the JSON file.",
            file=sys.stderr,
        )
        print("\nExtracted raw text for reference:", file=sys.stderr)
        print("-" * 40, file=sys.stderr)
        print(raw_text, file=sys.stderr)

    print(
        "\n📝 Please review and edit the JSON file before uploading.",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
