#pragma once
#include <vector>
#include <optional>
#include <memory>

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
	}

	virtual ~{{struct}}Schema() {
	}

	//getters
{% for mv in member_variables %}{% if not mv.required %}{% if mv.isArray %}
	// Optional getter for {{mv.identifier}}
	// Returns an optional containing the value of {{mv.identifier}} if it exists, or std::nullopt otherwise.
	// {{mv.identifier}}: {{mv.description}}
	virtual std::optional<{{mv.type}}> &get{{mv.identifierCamel}}() const;
{% else %}
	// Optional getter for {{mv.identifier}}
	// Returns an optional containing the value of {{mv.identifier}} if it exists, or std::nullopt otherwise.
	// {{mv.identifier}}: {{mv.description}}
	virtual std::optional<{{mv.type}}> get{{mv.identifierCamel}}() const;
{% endif %}{% else %}{% if mv.isArray %}
	// Getter for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual {{mv.type}} &get{{mv.identifierCamel}}() const;
{% else %}
	// Getter for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual {{mv.type}} get{{mv.identifierCamel}}() const;
{% endif %}{% endif %}{% endfor %}

	//setters
{% for mv in member_variables %}
	// Setter for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual void set{{mv.identifierCamel}}({{mv.type}} value);
{% endfor %}

{% for mv in member_variables %}{% if mv.isArray %}
	// Adder for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual void addTo{{mv.identifierCamel}}({{mv.elementType}} value);
{% endif %}{% endfor %}

{% for mv in member_variables %}{% if mv.isArray %}
	// Adder for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	virtual void clear{{mv.identifierCamel}}();
{% endif %}{% endfor %}

{% for f in functions %}
	{% if f.static %}static {% else %}virtual {% endif %}{{f.return_type}} {{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if param.defaultArg %}={{param.defaultArg}}{% endif %}{% if not loop.is_last %}, {% endif %}{% endfor %});
{% endfor %}

{% for key,g in generators %}
	// Generator: {{key}}
{% for f in g.functions %}
	{% if f.static %}static {% else %}virtual {% endif %}{{f.return_type}} {{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if param.defaultArg %}={{param.defaultArg}}{% endif %}{% if not loop.is_last %}, {% endif %}{% endfor %});
{% endfor %}

{% endfor %}

{% for bc in base_classes %}
    // Base class: {{bc.identifier}}
{% for f in bc.functions %}
    {% if f.static %}static {% else %}virtual {% endif %}{{f.return_type}} {{f.identifier}}({% for param in f.parameters %}{{param.type}} {{param.identifier}}{% if param.defaultArg %}={{param.defaultArg}}{% endif %}{% if not loop.is_last %}, {% endif %}{% endfor %}) override;
{% endfor %}

{% endfor %}

private:

{% for pv in private_variables %}
	{% if pv.static %}static {% endif %}{% if pv.const %}const {% endif %}{{pv.type}} {{pv.identifier}};
{% endfor %}

{% for mv in member_variables %}
{% if not mv.required %}
	// Optional member variable for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	std::optional<{{mv.type}}> {{mv.identifier}}{% if mv.default_value %} = {{mv.default_value}}{% endif %};
{% else %}
	// Member variable for {{mv.identifier}}
	// {{mv.identifier}}: {{mv.description}}
	{{mv.type}} {{mv.identifier}}{% if mv.default_value %} = {{mv.default_value}}{% endif %};
{% endif %}
{% endfor %}
};
