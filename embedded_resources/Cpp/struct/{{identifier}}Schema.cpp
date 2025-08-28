#include {{header_include}}

// Getter implementations
{% for mv in member_variables %}{% if not mv.required %}{% if mv.type.is_array %}
std::optional<{{mv.type}}>&{{identifier}}Schema::get{{mv.identifierCamel}}() const {
    return this->{{mv.identifier}};
}
{% else %}
std::optional<{{mv.type.estimated}}> {{identifier}}Schema::get{{mv.identifierCamel}}() const {
    return this->{{mv.identifier}};
}
{% endif %}{% else %}{% if mv.type.is_array %}
{{mv.type.estimated}} &{{identifier}}Schema::get{{mv.identifierCamel}}() const {
    return this->{{mv.identifier}};
}
{% else %}
{{mv.type.estimated}} {{identifier}}Schema::get{{mv.identifierCamel}}() const {
    return this->{{mv.identifier}};
}
{% endif %}{% endif %}{% endfor %}

// Setter implementations
{% for mv in member_variables %}
void {{identifier}}Schema::set{{mv.identifierCamel}}({{mv.type.estimated}} value) {
{% for sl in before_setter_lines %}
    {{sl.line}}
{% endfor %}
    this->{{mv.identifier}} = value;
}
{% endfor %}

{% for mv in member_variables %}{% if mv.type.is_array %}
void {{identifier}}Schema::addTo{{mv.identifierCamel}}({{mv.elementType.estimated}} value) {
    this->{{mv.identifier}}.push_back(value);
}
{% endif %}{% endfor %}

{% for mv in member_variables %}{% if mv.type.is_array %}
void {{identifier}}Schema::clear{{mv.identifierCamel}}() {
    this->{{mv.identifier}}.clear();
}
{% endif %}{% endfor %}

// Function implementations
{% for f in functions %}
{{f.return_type.estimated}} {{identifier}}Schema::{{f.identifier}}({% for param in f.parameters %}{{param.type.estimated}} {{param.identifier}}{% if not loop.is_last %}, {% endif %}{% endfor %}) {
{% if f.can_generate_function %}
    {{f.generate_function}}
{% endif %}
}
{% endfor %}
