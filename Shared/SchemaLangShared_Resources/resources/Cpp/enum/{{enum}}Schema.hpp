#pragma once
#include <string>

enum class {{enum}}Schema {
{% for value in values %}
    {{value.identifier}} = {{value.value}},
{% endfor %}
};

std::string {{enum}}SchemaToString({{enum}}Schema e);

{{enum}}Schema {{enum}}SchemaFromString(std::string str);