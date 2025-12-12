-- Schema version tracking table
-- This table tracks which migrations have been applied to each struct
CREATE TABLE IF NOT EXISTS `_schema_versions` (
    `table_name` VARCHAR(255) PRIMARY KEY,
    `version_major` INT NOT NULL,
    `version_minor` INT NOT NULL,
    `version_patch` INT NOT NULL,
    `applied_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `migration_file` VARCHAR(512)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
