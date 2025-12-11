-- Create ON DELETE triggers for cascade deletions
{% for field in member_variables %}
{% if field.type.is_struct %}
CREATE TRIGGER IF NOT EXISTS fk_{{identifier}}_{{field.identifier}}_delete
BEFORE DELETE ON {{field.type.identifier}}
FOR EACH ROW
BEGIN
    DELETE FROM {{identifier}} WHERE {{field.type.identifier}}_id = OLD.id;
END;
{% endif %}
{% endfor %}
{% for field in member_variables %}
{% if field.type.is_enum %}
CREATE TRIGGER IF NOT EXISTS fk_{{identifier}}_{{field.identifier}}_delete
BEFORE DELETE ON {{field.type.identifier}}
FOR EACH ROW
BEGIN
    DELETE FROM {{identifier}} WHERE {{field.type.identifier}}_id = OLD.id;
END;
{% endif %}
{% endfor %}
{% for field in member_variables %}
{% if field.reference.struct_name!="" and not field.type.is_struct and not field.type.is_enum %}
CREATE TRIGGER IF NOT EXISTS fk_{{identifier}}_{{field.identifier}}_delete
BEFORE DELETE ON {{field.reference.struct_name}}
FOR EACH ROW
BEGIN
    DELETE FROM {{identifier}} WHERE {{field.identifier}} = OLD.{{field.reference.variable_name}};
END;
{% endif %}
{% endfor %}
{% for inner_struct in structs %}
    {% for mv in inner_struct.member_variables %}
        {% if mv.type.is_array and mv.type.elem_type.is_struct and mv.type.elem_type.identifier == identifier %}
CREATE TRIGGER IF NOT EXISTS fk_{{identifier}}_{{inner_struct.identifier}}_delete
BEFORE DELETE ON {{inner_struct.identifier}}
FOR EACH ROW
BEGIN
    DELETE FROM {{identifier}} WHERE {{inner_struct.identifier}}_id = OLD.id;
END;
        {% endif %}
    {% endfor %}
{% endfor %}
{% for mv in member_variables %}
{% if mv.type.is_array and mv.type.is_array_of_base_type and not mv.type.is_array_of_enum %}
CREATE TRIGGER IF NOT EXISTS fk_{{identifier}}_{{mv.identifier}}_delete
BEFORE DELETE ON {{identifier}}
FOR EACH ROW
BEGIN
    DELETE FROM {{identifier}}_{{mv.identifier}} WHERE {{identifier}}_id = OLD.id;
END;
{% endif %}
{% endfor %}
-- Create triggers for structs that reference this struct
{% for other_struct in structs %}
{% for field in other_struct.member_variables %}
{% if field.type.is_struct and field.type.identifier == identifier %}
CREATE TRIGGER IF NOT EXISTS fk_{{other_struct.identifier}}_{{field.identifier}}_delete_check
BEFORE DELETE ON {{identifier}}
FOR EACH ROW
BEGIN
    DELETE FROM {{other_struct.identifier}} WHERE {{field.identifier}}_id = OLD.id;
END;
{% endif %}
{% endfor %}
{% endfor %}
