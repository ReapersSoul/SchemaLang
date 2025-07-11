CREATE TABLE
    IF NOT EXISTS {{struct}} (
        {%for field in fields -%}
            {% if field.convert_to_reference %}{{field.type}}Id INTEGER{% else %}{{field.name}} {{field.type}}{% endif %}{% if field.required %} NOT NULL{% endif %}{% if field.unique %} UNIQUE{% endif %}{% if field.primary_key %} PRIMARY KEY{% endif %}{% if field.auto_increment %} AUTOINCREMENT{% endif %}{% if field.reference %} REFERENCES {{field.reference.struct_name}}({{field.reference.variable_name}}){% endif %}{% if field.default_value %} DEFAULT {{field.default_value}}{% endif %}{% if not loop.is_last %},{% endif %}
        {%endfor -%});