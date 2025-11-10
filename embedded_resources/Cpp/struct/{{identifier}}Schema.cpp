#include {{header_include}}

// Getter implementations
{% for mv in member_variables %}{% if not mv.required %}
std::optional<{{mv.type.estimated}}>&{{identifier}}Schema::get{{mv.identifierCamel}}() {
    return this->{{mv.identifier}};
}
{% else %}
{{mv.type.estimated}} &{{identifier}}Schema::get{{mv.identifierCamel}}() {
    return this->{{mv.identifier}};
}
{% endif %}{% endfor %}

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
void {{identifier}}Schema::addTo{{mv.identifierCamel}}({{mv.type.elem_type.estimated}} value) {
    {% if not mv.required %}
    if (!this->{{mv.identifier}}.has_value()){
        this->{{mv.identifier}} = std::vector<{{mv.type.elem_type.estimated}}>();
    }
    this->{{mv.identifier}}.value().push_back(value);
    {% else %}
    this->{{mv.identifier}}.push_back(value);
    {% endif %}
}
{% endif %}{% endfor %}

{% for mv in member_variables %}{% if mv.type.is_array %}
void {{identifier}}Schema::clear{{mv.identifierCamel}}() {
    {% if not mv.required %}
    if (!this->{{mv.identifier}}.has_value()){
        return; // or throw an error
    }
    this->{{mv.identifier}}.value().clear();
    {% else %}
    this->{{mv.identifier}}.clear();
    {% endif %}
}
{% endif %}{% endfor %}

// Clone method implementations
std::shared_ptr<{{identifier}}Schema> {{identifier}}Schema::clone() const {
    auto cloned = std::make_shared<{{identifier}}Schema>();
    
{% for mv in member_variables %}{% if not mv.required %}
    if (this->{{mv.identifier}}.has_value()) {
        cloned->{{mv.identifier}} = this->{{mv.identifier}}.value();
    }
{% else %}
    cloned->{{mv.identifier}} = this->{{mv.identifier}};
{% endif %}{% endfor %}
    
    return cloned;
}

std::shared_ptr<{{identifier}}Schema> {{identifier}}Schema::deepClone() const {
    auto cloned = std::make_shared<{{identifier}}Schema>();
    
{% for mv in member_variables %}{% if not mv.required %}
    if (this->{{mv.identifier}}.has_value()) {
        // TODO: Add deep cloning logic for {{mv.identifier}} if it contains schema objects
        cloned->{{mv.identifier}} = this->{{mv.identifier}}.value();
    }
{% else %}
    // TODO: Add deep cloning logic for {{mv.identifier}} if it contains schema objects
    cloned->{{mv.identifier}} = this->{{mv.identifier}};
{% endif %}{% endfor %}
    
    return cloned;
}

// Function implementations
{% for f in functions %}
{{f.return_type.estimated}} {{identifier}}Schema::{{f.identifier}}({% for param in f.parameters %}{{param.type.estimated}} {{param.identifier}}{% if not loop.is_last %}, {% endif %}{% endfor %}) {
{% if f.can_generate_function %}
    {{f.generate_function}}
{% endif %}
}
{% endfor %}

{% for add in additions %}
// functions from {{add.gen_name}}
{% for f in add.functions %}
{{f}}
{% endfor %}{% endfor %}

{% for add in additions %}
// private variables from {{add.gen_name}}
{% for pv in add.private_variables %}
{{pv}}
{% endfor %}{% endfor %}

{% for add in additions %}
// members variables from {{add.gen_name}}
{% for mv in add.member_variables %}
{{mv}}
{% endfor %}{% endfor %}