#include {{enum_include}}

std::string {{enum}}SchemaToString({{enum}}Schema e) {
    switch(e) {
{% for value in values %}
        case {{enum}}Schema::{{value.identifier}}:
            return "{{value.identifier}}";
{% endfor %}
        default:
            return "Unknown";
    }
}

{{enum}}Schema {{enum}}SchemaFromString(std::string str) {
{% for value in values %}
{% if not loop.is_last %}
    if(str == "{{value.identifier}}") {
        return {{enum}}Schema::{{value.identifier}};
    }
{% endif %}
{% endfor %}
    return {{enum}}Schema::Unknown;
}