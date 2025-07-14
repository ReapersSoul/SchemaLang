#pragma once
#include <vector>
#include <optional>

//base classes
{% for bc in base_classes %}
#include {{bc.formatted_include}}
{% endfor %}

{% if schema_includes %}
//schema includes
{% for include in schema_includes %}
#include {{include}}
{% endfor %}
{% endif %}

{% if includes %}
//includes
{% for include in includes %}
#include {{include}}
{% endfor %}
{% endif %}

{% for key,g in generators %}
// Generator: {{key}}
{% for include in g.includes %}
#include {{include}}
{% endfor %}
{% endfor %}

class {{struct}}Schema : {% for bc in base_classes %}public Has{{ bc.identifier }}Schema{% if not loop.is_last %}, {% endif %}{% endfor %}{
public:
	{{struct}}Schema() {
		all_{{struct}}Schemas.push_back(this);
	}

	~{{struct}}Schema() {
		auto it = std::find(all_{{struct}}Schemas.begin(), all_{{struct}}Schemas.end(), this);
		if (it != all_{{struct}}Schemas.end()) {
			all_{{struct}}Schemas.erase(it);
		}
	}

	//getters
{% for mv in member_variables %}{% if not mv.required %}
	std::optional<{{mv.type}}> get{{mv.identifier}}() const;
{% else %}
	{{mv.type}} get{{mv.identifier}}() const;
{% endif %}{% endfor %}

	//setters
{% for mv in member_variables %}
	void set{{mv.identifier}}({{mv.type}} value);
{% endfor %}

{% for f in functions %}
	{% if f.static %}static {% endif %}{{f.return_type}} {{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if not loop.is_last %}, {% endif %}{% endfor %});
{% endfor %}

{% for key,g in generators %}
	// Generator: {{key}}
{% for f in g.functions %}
	{% if f.static %}static {% endif %}{{f.return_type}} {{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if not loop.is_last %}, {% endif %}{% endfor %});
{% endfor %}

{% endfor %}

{% for bc in base_classes %}
    // Base class: {{bc.identifier}}
{% for f in bc.functions %}
    {% if f.static %}static {% endif %}{{f.return_type}} {{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if not loop.is_last %}, {% endif %}{% endfor %}) override;
{% endfor %}

{% endfor %}

private:
	static std::vector<{{struct}}Schema*> all_{{struct}}Schemas;

{% for pv in private_variables %}
	{% if pv.static %}static {% endif %}{% if pv.const %}const {% endif %}{{pv.type}} {{pv.identifier}};
{% endfor %}

{% for mv in member_variables %}
{% if not mv.required %}
	std::optional<{{mv.type}}> {{mv.identifier}}{% if mv.default_value %} = {{mv.default_value}}{% endif %};
{% else %}
	{{mv.type}} {{mv.identifier}}{% if mv.default_value %} = {{mv.default_value}}{% endif %};
{% endif %}
{% endfor %}
};
