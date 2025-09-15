#pragma once
#include <vector>
#include <optional>
#include <memory>
#include <concepts>
{% for include in includes %}
#include {{include}}
{% endfor %}

{% for add in additions %}
// includes from {{add.gen_name}}
{% for include in add.includes %}
#include {{include}}
{% endfor %}{% endfor %}

class {{identifier}}Schema : public std::enable_shared_from_this<{{identifier}}Schema> {
public:
	{{identifier}}Schema() {
	}

	virtual ~{{identifier}}Schema() {
	}

	//getters
{% for mv in member_variables %}{% if not mv.required %}
	// Optional getter for {{mv.identifier}}
	// Returns an optional containing the value of {{mv.identifier}} if it exists, or std::nullopt otherwise.
	// {{mv.identifier}}: {{mv.description}}
	virtual std::optional<{{mv.type.estimated}}> &get{{mv.identifierCamel}}();
{% else %}
	// Getter for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual {{mv.type.estimated}} &get{{mv.identifierCamel}}();
{% endif %}{% endfor %}

	//setters
{% for mv in member_variables %}
	// Setter for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual void set{{mv.identifierCamel}}({{mv.type.estimated}} value);
{% endfor %}

{% for mv in member_variables %}{% if mv.type.is_array %}
	// Adder for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual void addTo{{mv.identifierCamel}}({{mv.type.elem_type.estimated}} value);
{% endif %}{% endfor %}

{% for mv in member_variables %}{% if mv.type.is_array %}
	// Adder for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual void clear{{mv.identifierCamel}}();
{% endif %}{% endfor %}

{% for f in functions %}
	{% if f.static %}static {% else %}virtual {% endif %}{{f.return_type.estimated}} {{f.identifier}}({% for param in f.parameters %}{{param.type.estimated}} {{param.identifier}}{% if param.defaultArg %}={{param.defaultArg}}{% endif %}{% if not loop.is_last %}, {% endif %}{% endfor %});
{% endfor %}

{% for add in additions %}
// functions from {{add.gen_name}}
{% for f in add.functions %}
	{{f}}
{% endfor %}{% endfor %}
	//"as" must be child class to use 
	template<typename T>
		requires std::derived_from<T, {{identifier}}Schema>
	T* as() {
		return dynamic_cast<T*>(this);
	}

protected:

{% for pv in private_variables %}
	{% if pv.static %}static {% endif %}{% if pv.const %}const {% endif %}{{pv.type.estimated}} {{pv.identifier}};
{% endfor %}

{% for mv in member_variables %}
{% if not mv.required %}
	// Optional member variable for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	std::optional<{{mv.type.estimated}}> {{mv.identifier}}{% if mv.type.defaulted %} = {{mv.default_value}}{% endif %};
{% else %}
	// Member variable for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	{{mv.type.estimated}} {{mv.identifier}}{% if mv.type.defaulted %} = {{mv.default_value}}{% endif %};
{% endif %}
{% endfor %}

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

};
