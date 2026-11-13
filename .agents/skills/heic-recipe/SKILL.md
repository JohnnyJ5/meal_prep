---
name: heic-recipe
description: Parse a HEIC, JPG, JPEG, or PNG image file (or all recipe photos in a directory) to extract a recipe (name and ingredients) and upload it to the production meal-prep database. Use when the user says "import recipe from photo", "add recipe from image", "upload recipe", or provides a .heic, .jpg, .jpeg, or .png file path or directory with intent to add recipes to the database.
---

# Photo Recipe Importer (HEIC / JPG / PNG)

Parse one or more recipe photos (`.heic`, `.jpg`, `.jpeg`, `.png`) and upload them to the production meal-prep API.

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

### Step 1 — Prepare Image(s) (HEIC / JPG / PNG)

Handle either a single image path or all matching image files (`.heic`, `.HEIC`, `.jpg`, `.jpeg`, `.JPG`, `.JPEG`, `.png`, `.PNG`) in the target directory:

- **`.jpg`, `.jpeg`, or `.png` (case-insensitive)**: Skip format conversion and use the image path directly. (If the photo is rotated sideways, you may optionally create an upright temporary copy in `/tmp` with Python PIL `ImageOps.exif_transpose` / `rotate` to make visual inspection easier, then delete the temporary file in Step 6.)
- **`.heic` / `.HEIC`**: Convert to a temporary PNG first:

```bash
HEIC_PATH="<path-to-heic-file>"
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

### Step 2 — Inspect the Image

Use the `read_file` tool on the JPG/PNG path (or temporary upright copy) to visually inspect the recipe.

### Step 3 — Extract Recipe Data

From the image, identify:
- **name**: the title/name of the dish formatted as **kebab-case** (lowercase, spaces replaced with hyphens, e.g. `"greek-yogurt-banana-pancakes"`). All existing meals use this format and Crow's URL routing requires it.
- **category**: match existing app categories where applicable (e.g. `Breakfast`, `Chicken`, `Meat`, `Pasta`, `Slow Cooker`, `Bakegoods`, `Lunch`, `Dinner`, `Snack`, `Dessert`, `Side`, `Drink`) or nested subcategories using ` / ` when the dish belongs to a recipe family (e.g. `Bakegoods / Muffins`, `Bakegoods / Energy Bites`, `Breakfast / Pancakes`, `Breakfast / Overnight Oats`, `Chicken / Soups`, `Chicken / Salads`, `Chicken / Creamy`, `Meat / Meatballs & Burgers`, `Meat / Skillets & Pasta`) — default `Uncategorized`
- **ingredients**: array of objects (skip any ingredients that are visibly crossed out on the printed page), each with:
  - `name` — ingredient name in Title Case, capitalizing the first letter of each word (e.g. `"Chicken Breast"`). Prefer matching existing names from `GET /api/ingredients` when equivalent.
  - `amount` — numeric quantity (use midpoint for ranges, 1.0 if unclear)
  - `unit` — integer from the table above
  - `preparation` — string from the list above, or `"None"`
  - `optional` — boolean (`true` if marked "(optional)" on the recipe; omit or `false` otherwise)

### Step 4 — Discover Production API URL

```bash
gcloud run services describe meal-prep \
    --region=us-central1 \
    --format='value(status.url)' 2>/dev/null || echo "https://meal-prep-449200254263.us-central1.run.app"
```

Use the URL printed (strip trailing whitespace/newlines). Note that if the Cloud Run service is scaled to 0 instances (`503 Service Unavailable`), stop and inform the user rather than modifying production storage without confirmation.

### Step 5 — Add Missing Ingredients & POST to the API

Optionally fetch `${PROD_URL}/api/ingredients` first and add any new ingredients via `POST ${PROD_URL}/api/ingredients/add` with `{"name": "<Name>", "category": "<Category>"}` so Whole Foods order categorization works for new ingredients.

Then build a `curl` command per recipe:

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
rm -f "/tmp/recipe_$$.png" /tmp/rot_*.jpg
```

### Step 7 — Report to User

Print a summary including:
- Recipe name and category extracted
- Full ingredient list (name, amount+unit, preparation)
- HTTP status code and API response body
- If HTTP 500 with "Name might already exist": tell the user the recipe is a duplicate
- If HTTP 200: confirm the recipe was successfully added to the production database
