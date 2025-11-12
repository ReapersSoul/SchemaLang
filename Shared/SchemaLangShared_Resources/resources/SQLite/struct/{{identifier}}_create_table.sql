CREATE TABLE
    IF NOT EXISTS {{identifier}} (
        {% for field in member_variables -%}
            {{- field.identifier }} {{ field.type.estimated }}{% if field.required %} NOT NULL{% endif %}{% if field.unique %} UNIQUE{% endif %}{% if field.primary_key %} PRIMARY KEY{% endif %}{% if field.auto_increment %} AUTOINCREMENT{% endif %}{% if not field.reference.struct_name=="" %} REFERENCES {{field.reference.struct_name}}({{field.reference.variable_name}}){% endif %}{% if field.default_value %} DEFAULT {{field.default_value}}{% endif %}{% if not loop.is_last %},{% endif %}
        {% endfor -%});