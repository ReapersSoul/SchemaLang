CREATE TABLE
    IF NOT EXISTS {{enum}} (
        id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        name TEXT NOT NULL UNIQUE,
        value INTEGER NOT NULL
    );


-- insert into if not exists if conflict do nothing
INSERT INTO {{enum}} (name, value)
{% for value in values %}
VALUES ('{{value.name}}', {{value.value}})
{% if not loop.is_last %}, {% endif %}
{% endfor %}
ON CONFLICT(name) DO NOTHING;