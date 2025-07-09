#pragma once
//base classes
{% for bc in base_classes %}
#include "Has{{ bc }}Schema.hpp"
{% endfor %}

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

class {{struct}}Schema : {% for bc in base_classes %}public Has{{ bc }}Schema{% if not loop.is_last %}, {% endif %}{% endfor %}{
public:
	{{struct}}Schema() {
		all_nodes.push_back(this);
	}

	~{{struct}}Schema() {
		auto it = std::find(all_nodes.begin(), all_nodes.end(), this);
		if (it != all_nodes.end()) {
			all_nodes.erase(it);
		}
	}

	//getters
{% for mv in member_variables %}{% if not mv.required %}
	std::optional<{{mv.type}}> get{{mv.identifier}}() const {
		return this->{{mv.identifier}};
	}
{% else %}
	{{mv.type}} get{{mv.identifier}}() const {
		return this->{{mv.identifier}};
	}
{% endif %}{% endfor %}

	//setters
{% for mv in member_variables %}
	void set{{mv.identifier}}({{mv.type}} value) {
		this->{{mv.identifier}} = value;
		return this->{{mv.identifier}};
	}
{% endfor %}

{% for f in functions %}
	{% if f.static %}static {% endif %}{{f.return_type}} {{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if not loop.is_last %},{% endif %}{% endfor %}) {
{% if f.can_generate_function %}
		{{f.generate_function}}
{% else %}
		// Default implementation
		return {{f.return_type}}();
{% endif %}
	}
{% endfor %}

{% for key,g in generators %}
	// Generator: {{key}}
{% for f in g.functions %}
	{% if f.static %}static {% endif %}{{f.return_type}} {{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if not loop.is_last %},{% endif %}{% endfor %}) {
{% if f.can_generate_function %}
		{{f.generate_function}}
{% else %}
		// Default implementation
		return {{f.return_type}}();
{% endif %}
	}
{% endfor %}

{% endfor %}

private:
	static std::vector<{{struct}}Schema*> all_{{struct}}Schemas;

{% for pv in private_variables %}
	{{pv.type}} {{pv.identifier}};
{% endfor %}

{% for mv in member_variables %}
{% if not mv.required %}
	std::optional<{{mv.type}}> {{mv.identifier}}{% if mv.default_value %} = {{mv.default_value}}{% endif %};
{% else %}
	{{mv.type}} {{mv.identifier}}{% if mv.default_value %} = {{mv.default_value}}{% endif %};
{% endif %}
{% endfor %}
};
