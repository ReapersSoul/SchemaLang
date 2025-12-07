{% set additional_field_count = 0 %}
{% for inner_struct in structs %}
    {% for mv in inner_struct.member_variables %}
        {% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == identifier %}
            {% set additional_field_count = additional_field_count + 1 %}
        {% endif %}
    {% endfor %}
{% endfor %}
CREATE TABLE IF NOT EXISTS {{identifier}} (
{% set field_count = 0 %}
{% for field in member_variables %}
    {% set field_count = field_count + 1 %}
{% endfor %}
{% set current_field = 0 %}
{% for field in member_variables %}
    {% set current_field = current_field + 1 %}

    {% if field.type.is_array %}
    {% else if field.type.is_struct %}
        {{field.type.identifier}}_id INTEGER
    {% else if field.type.is_enum %}
        {{field.type.identifier}}_id INTEGER
    {% else %}
        {{ field.identifier }} {{ SQLite_convert_to_local_type(field.type) }} 
    {% endif %}

    {% if not field.type.is_array %}
        {% if field.type.required %} NOT NULL {% endif %}
        {% if field.unique %} UNIQUE {% endif %}
        {% if field.primary_key %} PRIMARY KEY {% endif %}
        {% if field.auto_increment %} AUTOINCREMENT {% endif %}
    {% endif %}

    {% if field.type.is_array %}
    {% else if field.type.is_struct %}
        REFERENCES {{field.type.identifier}}(id) ON DELETE CASCADE
        DEFAULT 0
    {% else if field.type.is_enum %}
        REFERENCES {{field.type.identifier}}(id) ON DELETE CASCADE
        DEFAULT 0
    {% else %}
        {% if field.reference.struct_name!="" %} REFERENCES {{field.reference.struct_name}}({{field.reference.variable_name}}) ON DELETE CASCADE {% endif %}
        {% if field.default_value != "" %} DEFAULT {{SQLite_format_default(field.type, field.default_value)}}{% endif %}
    {% endif %}
    {% if not field.type.is_array %}
        {% if current_field < field_count or additional_field_count > 0 %},{% endif %}    
    {% endif %}
{% endfor %}
{% set current_field = 0 %}
{% for inner_struct in structs %}
    {% for mv in inner_struct.member_variables %}
        {% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == identifier %}{% set current_field = current_field + 1 %}
            {{inner_struct.identifier}}_id INTEGER REFERENCES {{inner_struct.identifier}}(id) ON DELETE CASCADE{% if current_field < additional_field_count %},{% endif %}
        {% endif %}
    {% endfor %}
{% endfor %}
);
{% for mv in member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
CREATE TABLE IF NOT EXISTS {{identifier}}_{{mv.identifier}} (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    {{identifier}}_id INTEGER NOT NULL REFERENCES {{identifier}}(id) ON DELETE CASCADE,
    sequence INTEGER NOT NULL,
    value {{ SQLite_convert_to_local_type(mv.type.elem_type) }} NOT NULL{% if mv.unique %},
    UNIQUE({{identifier}}_id, value){% endif %},
    UNIQUE({{identifier}}_id, sequence)
);
CREATE INDEX IF NOT EXISTS idx_{{identifier}}_{{mv.identifier}}_parent_id ON {{identifier}}_{{mv.identifier}}({{identifier}}_id);
{% endif %}
{% endfor %}