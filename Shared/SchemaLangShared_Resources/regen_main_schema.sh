#!/bin/bash

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCHEMAS_DIR="$SCRIPT_DIR/Schemas"
MAIN_SCHEMA="$SCHEMAS_DIR/main.schema"

# Check if Schemas directory exists
if [ ! -d "$SCHEMAS_DIR" ]; then
    echo "Error: Schemas directory not found at $SCHEMAS_DIR"
    exit 1
fi

# Create/overwrite main.schema file
> "$MAIN_SCHEMA"

# Find all .schema files (excluding main.schema) and add include statements
find "$SCHEMAS_DIR" -maxdepth 1 -name "*.schema" -not -name "main.schema" -type f | sort | while read -r schema_file; do
    # Get just the filename
    filename=$(basename "$schema_file")
    # Write include statement
    echo "include \"./$filename\"" >> "$MAIN_SCHEMA"
done

echo "Generated $MAIN_SCHEMA with $(grep -c "include" "$MAIN_SCHEMA") schema files"