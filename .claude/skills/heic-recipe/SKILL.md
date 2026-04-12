---
name: heic-recipe
description: Parse a HEIC image file to extract a recipe (name and ingredients) and upload it to the production meal-prep database. Use when the user says "import recipe from photo", "add recipe from image", "upload recipe", or provides a .heic/.HEIC file path with intent to add it to the database.
argument-hint: <path-to-heic-file>
allowed-tools: [Bash, Read]
---

# HEIC Recipe Importer

Parse a HEIC photo of a recipe and upload it to the production meal-prep API.

## Arguments

The user provided: $ARGUMENTS

## MeasurementUnit Integer Reference

Map every ingredient's unit to one of these integers:
| Int | Unit       | When to use                                      |
|-----|------------|--------------------------------------------------|
| 0   | GRAM       | metric weight (g)                                |
| 1   | OUNCE      | oz (weight or fluid oz)                          |
| 2   | CUP        | cups                                             |
| 3   | TABLESPOON | tbsp                                             |
| 4   | TEASPOON   | tsp                                              |
| 5   | POUND      | lbs                                              |
| 6   | WHOLE      | countable items with no unit (eggs, cans, etc.)  |
| 7   | HALF       | "half of" something                              |
| 8   | SMALL      | small-sized item                                 |
| 9   | CLOVE      | cloves (garlic, etc.)                            |
| 10  | HEAD       | heads (garlic, lettuce, etc.)                    |

Default to **6 (WHOLE)** for countable items. Default to **2 (CUP)** for unlabeled liquid volume.

## Preparation Values

Use one of these exact strings (or `"None"` if not specified):
`WHOLE` `HALF` `CHOPPED` `DICED` `STRIPS` `SHREDDED` `MINCED` `GRATED` `THIN_SLICED` `PEELED`

---

## Steps

### Step 1 — Convert HEIC to PNG

```bash
HEIC_PATH="$ARGUMENTS"
TMP_PNG="/tmp/recipe_$$.png"

if command -v heif-convert &>/dev/null; then
    heif-convert "$HEIC_PATH" "$TMP_PNG" && echo "converted:$TMP_PNG"
elif command -v convert &>/dev/null; then
    convert "$HEIC_PATH" "$TMP_PNG" && echo "converted:$TMP_PNG"
else
    echo "ERROR: Install heif-convert (libheif-examples) or ImageMagick." >&2
    exit 1
fi
```

If the file already ends in `.jpg`, `.jpeg`, or `.png`, skip conversion and use the path directly.

### Step 2 — Read the Image

Use the **Read tool** on the PNG path to visually inspect the recipe.

### Step 3 — Extract Recipe Data

From the image, identify:
- **name**: the title/name of the dish formatted as **kebab-case** (lowercase, spaces replaced with hyphens, e.g. `"greek-yogurt-banana-pancakes"`). All existing meals use this format and Crow's URL routing requires it.
- **category**: one of `Breakfast`, `Lunch`, `Dinner`, `Snack`, `Dessert`, `Side`, `Drink` — default `Uncategorized`
- **ingredients**: array of objects, each with:
  - `name` — lowercase ingredient name (e.g. `"chicken breast"`)
  - `amount` — numeric quantity (use midpoint for ranges, 1.0 if unclear)
  - `unit` — integer from the table above
  - `preparation` — string from the list above, or `"None"`

### Step 4 — Discover Production API URL

```bash
gcloud run services describe meal-prep \
    --region=us-central1 \
    --format='value(status.url)' 2>/dev/null || echo "https://meal-prep-449200254263.us-central1.run.app"
```

Use the URL printed (strip trailing whitespace/newlines).

### Step 5 — POST to the API

Build a single `curl` command with the extracted data:

```bash
curl -s -w "\n%{http_code}" -X POST "${PROD_URL}/api/meals/add" \
    -H "Content-Type: application/json" \
    -d '{
      "name": "<recipe name>",
      "category": "<category>",
      "ingredients": [ ... ]
    }'
```

### Step 6 — Cleanup

```bash
rm -f "/tmp/recipe_$$.png"
```

### Step 7 — Report to User

Print a summary including:
- Recipe name and category extracted
- Full ingredient list (name, amount+unit, preparation)
- HTTP status code and API response body
- If HTTP 500 with "Name might already exist": tell the user the recipe is a duplicate
- If HTTP 200: confirm the recipe was successfully added to the production database
