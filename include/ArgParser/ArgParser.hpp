#pragma once
#include <string>
#include <functional>
#include <vector>
#include <iostream>
#include <algorithm>

class Flag
{
	std::string name = "";
	bool value = false;
	bool required = false;
	int priority = 0;
	std::function<void()> callback = nullptr;
	bool HasCallback = false;

public:
	Flag(std::string name, bool required, std::function<void()> callback = nullptr, int priority = 0)
	{
		this->name = name;
		this->required = required;
		this->callback = callback;
		this->priority = priority;
		if (callback)
		{
			HasCallback = true;
		}
		else
		{
			HasCallback = false;
		}
	}

	void setValue(bool value)
	{
		this->value = value;
	}

	bool getValue()
	{
		return value;
	}

	std::string getName()
	{
		return name;
	}

	bool isRequired()
	{
		return required;
	}

	bool hasCallback()
	{
		return HasCallback;
	}

	int getPriority() const
	{
		return priority;
	}

	void execute()
	{
		callback();
	}
};

class Parameter
{
	std::string name = "";
	std::string value = "";
	bool required = false;
	int priority = 0;
	std::function<void(std::string)> callback = nullptr;
	bool HasCallback = false;

public:
	Parameter(std::string name, bool required, std::function<void(std::string)> callback = nullptr, int priority = 0)
	{
		this->name = name;
		this->required = required;
		this->callback = callback;
		this->priority = priority;
		if (callback)
		{
			HasCallback = true;
		}
		else
		{
			HasCallback = false;
		}
	}

	void setValue(std::string value)
	{
		this->value = value;
	}

	std::string getValue()
	{
		return value;
	}

	std::string getName()
	{
		return name;
	}

	bool isRequired()
	{
		return required;
	}

	bool hasCallback()
	{
		return HasCallback;
	}

	int getPriority() const
	{
		return priority;
	}

	void execute()
	{
		callback(value);
	}
};

class argumentParser
{
	std::string programName;
	std::vector<Flag *> flags;
	std::vector<Parameter *> parameters;

	void split(std::string &name, std::string &value, std::string arg)
	{
		size_t pos = arg.find("=");
		if (pos != std::string::npos)
		{
			name = arg.substr(0, pos);
			value = arg.substr(pos + 1);
		}
		else
		{
			name = arg;
			value = "1";
		}
	}

public:
	bool parse(int argc, char *argv[])
	{
		std::vector<std::string> args;
		std::vector<Flag*> flagsToExecute;
		std::vector<Parameter*> parametersToExecute;
		programName = argv[0];
		for (int i = 1; i < argc; i++)
		{
			args.push_back(argv[i]);
		}
		for (int i = 0; i < args.size(); i++)
		{
			if (args[i].substr(0, 2) == "--" || args[i].substr(0, 1) == "-")
			{
				// strip off the -- or -
				if (args[i].substr(0, 2) == "--")
				{
					args[i] = args[i].substr(2);
				}
				else
				{
					args[i] = args[i].substr(1);
				}
				bool found = false;
				std::string flagName;
				std::string flagValue;
				// split the flag into name and value
				split(flagName, flagValue, args[i]);
				for (int j = 0; j < flags.size(); j++)
				{
					if (flags[j]->getName() == flagName)
					{
						flags[j]->setValue(true);
						if (flags[j]->hasCallback())
						{
							flagsToExecute.push_back(flags[j]);
						}
						found = true;
						break;
					}
				}
				for (int j = 0; j < parameters.size(); j++)
				{
					if (parameters[j]->getName() == flagName)
					{
						parameters[j]->setValue(flagValue);
						if (parameters[j]->hasCallback())
						{
							parametersToExecute.push_back(parameters[j]);
						}
						found = true;
						break;
					}
				}
				if (!found)
				{
					printf("Unknown flag or parameter: %s\n", flagName.c_str());
					printUsage();
					return false;
				}
			}
		}
		
		// Create a unified collection of callbacks with their priorities
		std::vector<std::pair<int, std::function<void()>>> callbacksWithPriority;
		
		// Add flag callbacks to the unified collection
		for (Flag* flag : flagsToExecute)
		{
			callbacksWithPriority.push_back(std::make_pair(flag->getPriority(), [flag]() { flag->execute(); }));
		}
		
		// Add parameter callbacks to the unified collection
		for (Parameter* parameter : parametersToExecute)
		{
			callbacksWithPriority.push_back(std::make_pair(parameter->getPriority(), [parameter]() { parameter->execute(); }));
		}
		
		// Sort all callbacks by priority (higher priority first)
		std::sort(callbacksWithPriority.begin(), callbacksWithPriority.end(), 
			[](const std::pair<int, std::function<void()>>& a, const std::pair<int, std::function<void()>>& b) {
				return a.first > b.first;
			});
		
		// Execute all callbacks in priority order
		for (const auto& callback : callbacksWithPriority)
		{
			callback.second();
		}
		
		for (int i = 0; i < flags.size(); i++)
		{
			if (flags[i]->isRequired() && !flags[i]->getValue())
			{
				printf("Flag %s is required\n", flags[i]->getName().c_str());
				printUsage();
				return false;
			}
		}
		for (int i = 0; i < parameters.size(); i++)
		{
			if (parameters[i]->isRequired() && parameters[i]->getValue() == "")
			{
				printf("Parameter %s is required\n", parameters[i]->getName().c_str());
				printUsage();
				return false;
			}
		}
		return true;
	}

	void printUsage()
	{
		printf("Usage: %s [flags] [parameters]\n", programName.c_str());
		printf("Flags:\n");
		for (int i = 0; i < flags.size(); i++)
		{
			printf("\t%s: %s\n", flags[i]->getName().c_str(), flags[i]->isRequired() ? "required" : "optional");
		}
		printf("Parameters:\n");
		for (int i = 0; i < parameters.size(); i++)
		{
			printf("\t%s: %s\n", parameters[i]->getName().c_str(), parameters[i]->isRequired() ? "required" : "optional");
		}
	}

	void addFlag(Flag *flag)
	{
		flags.push_back(flag);
	}

	void addParameter(Parameter *parameter)
	{
		parameters.push_back(parameter);
	}
};