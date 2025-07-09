#include "{{struct}}Schema.hpp"

// Static member definition
std::vector<{{struct}}Schema*> {{struct}}Schema::all_{{struct}}Schemas;

// Getter implementations
{% for mv in member_variables %}
{% if not mv.required %}
std::optional<{{mv.type}}> {{struct}}Schema::get{{mv.identifier}}() const {
    return this->{{mv.identifier}};
}
{% else %}
{{mv.type}} {{struct}}Schema::get{{mv.identifier}}() const {
    return this->{{mv.identifier}};
}
{% endif %}
{% endfor %}

// Setter implementations
{% for mv in member_variables %}
void {{struct}}Schema::set{{mv.identifier}}({{mv.type}} value) {
{% for sl in before_setter_lines %}
    {{sl.line}}
{% endfor %}
    this->{{mv.identifier}} = value;
}
{% endfor %}

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
