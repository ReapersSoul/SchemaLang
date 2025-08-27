#pragma once
#include <string>

enum class {{enum}}Schema {
{% for value in values %}
    {{enum}}_{{value.identifier}} = {{value.value}},
{% endfor %}
};

static std::string {{enum}}SchemaToString({{enum}}Schema e);

static {{enum}}Schema {{enum}}SchemaFromString(std::string str);