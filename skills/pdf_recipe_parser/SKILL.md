---
name: PDF Recipe Parser
description: Parse a recipe PDF into a JSON payload suitable for uploading to the meal prep database via the API.
---

# PDF Recipe Parser Skill

This skill extracts recipe data from a PDF file and produces a JSON payload that matches the `/api/meals/add` endpoint format. The workflow has a **mandatory manual verification** step before uploading.

## Prerequisites

Install the required Python packages (host machine, not Docker):

```bash
pip3 install PyPDF2 requests
```

## Workflow

### Step 1: Parse the PDF

```bash
python3 skills/pdf_recipe_parser/scripts/parse_recipe.py <path-to-pdf>
```

This extracts text from the PDF and produces a JSON file at `recipes/<recipe-name>.json`. The JSON is also printed to stdout.

### Step 2: User Reviews the JSON

**IMPORTANT**: The user MUST review and edit the generated JSON before uploading. The parser uses heuristics and may not perfectly extract all ingredients, amounts, or units.

Open the generated JSON file, verify:
- Recipe name is correct (kebab-case, lowercase)
- Category is appropriate
- Each ingredient has the correct name, amount, unit, and preparation
- No ingredients are missing or duplicated

### Step 3: Upload to the Database

```bash
python3 scripts/upload_recipe.py recipes/<recipe-name>.json
```

Optional flags:
- `--server <url>` — API server URL (default: `http://localhost:8080`)
- `--dry-run` — Print what would be sent without actually uploading

The upload script automatically adds any new ingredients to the `available_ingredients` table.

## JSON Format

```json
{
  "name": "recipe-name-kebab-case",
  "category": "Category",
  "ingredients": [
    {
      "name": "Ingredient Name",
      "amount": 1.0,
      "unit": 2,
      "preparation": "Chopped"
    }
  ]
}
```

## MeasurementUnit Enum (integer values)

| Value | Unit        |
|-------|-------------|
| 0     | GRAM        |
| 1     | OUNCE       |
| 2     | CUP         |
| 3     | TABLESPOON  |
| 4     | TEASPOON    |
| 5     | POUND       |
| 6     | WHOLE       |
| 7     | HALF        |
| 8     | SMALL       |
| 9     | CLOVE       |
| 10    | HEAD        |
