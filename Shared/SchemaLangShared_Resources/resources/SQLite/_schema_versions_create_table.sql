-- Schema version tracking table
-- This table tracks which migrations have been applied to each struct
CREATE TABLE IF NOT EXISTS _schema_versions (
    table_name TEXT PRIMARY KEY,
    version_major INTEGER NOT NULL,
    version_minor INTEGER NOT NULL,
    version_patch INTEGER NOT NULL,
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    migration_file TEXT
);
