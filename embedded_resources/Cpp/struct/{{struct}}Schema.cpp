#include {{struct_include}}

// Getter implementations
{% for mv in member_variables %}{% if not mv.required %}{% if mv.isArray %}
std::optional<{{mv.type}}>&{{struct}}Schema::get{{mv.identifierCamel}}() const {
    return this->{{mv.identifier}};
}
{% else %}
std::optional<{{mv.type}}> {{struct}}Schema::get{{mv.identifierCamel}}() const {
    return this->{{mv.identifier}};
}
{% endif %}{% else %}{% if mv.isArray %}
{{mv.type}} &{{struct}}Schema::get{{mv.identifierCamel}}() const {
    return this->{{mv.identifier}};
}
{% else %}
{{mv.type}} {{struct}}Schema::get{{mv.identifierCamel}}() const {
    return this->{{mv.identifier}};
}
{% endif %}{% endif %}{% endfor %}

// Setter implementations
{% for mv in member_variables %}
void {{struct}}Schema::set{{mv.identifierCamel}}({{mv.type}} value) {
{% for sl in before_setter_lines %}
    {{sl.line}}
{% endfor %}
    this->{{mv.identifier}} = value;
}
{% endfor %}

{% for mv in member_variables %}{% if mv.isArray %}
void {{struct}}Schema::addTo{{mv.identifierCamel}}({{mv.elementType}} value) {
    this->{{mv.identifier}}.push_back(value);
}
{% endif %}{% endfor %}

{% for mv in member_variables %}{% if mv.isArray %}
void {{struct}}Schema::clear{{mv.identifierCamel}}() {
    this->{{mv.identifier}}.clear();
}
{% endif %}{% endfor %}

// Function implementations
{% for f in functions %}
{{f.return_type}} {{struct}}Schema::{{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if not loop.is_last %}, {% endif %}{% endfor %}) {
{% if f.can_generate_function %}
    {{f.generate_function}}
{% endif %}
}
{% endfor %}

// Generator-specific function implementations
{% for key, g in generators %}
// Generator: {{key}}
{% for gcf in g.functions %}
{{gcf.return_type}} {{struct}}Schema::{{gcf.identifier}}({% for param in gcf.parameters %}{{param.type}} {{param.identifier}}{% if not loop.is_last %}, {% endif %}{% endfor %}) {
{% if gcf.can_generate_function %}
    {{gcf.generate_function}}
{% endif %}
}
{% endfor %}

{% endfor %}

// Base class function overrides
{% for bc in base_classes %}
// Base class: {{bc.identifier}}
{% for bcf in bc.functions %}
{{bcf.return_type}} {{struct}}Schema::{{bcf.identifier}}({% for param in bcf.parameters %}{{param.type}} {{param.identifier}}{% if not loop.is_last %}, {% endif %}{% endfor %}) {
{% if bcf.can_generate_function %}
    {{bcf.generate_function}}
{% endif %}
}
{% endfor %}

{% endfor %}
