# Upload Recipe from PDF

A guided task for importing a recipe from a PDF into the meal prep database.

## Steps

1. **Parse the PDF**
   ```bash
   python3 skills/pdf_recipe_parser/scripts/parse_recipe.py <path-to-pdf>
   ```
   This generates a JSON file at `recipes/<recipe-name>.json`.

2. **Review the JSON**
   - Open the generated JSON file
   - Verify recipe name, category, ingredients, amounts, and units
   - Edit as needed (refer to the MeasurementUnit table in SKILL.md)

3. **Upload to database** (after user approval)
   ```bash
   python3 scripts/upload_recipe.py recipes/<recipe-name>.json
   ```
   Use `--dry-run` flag first to preview what will be uploaded.
