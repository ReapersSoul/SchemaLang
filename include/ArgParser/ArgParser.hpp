#pragma once
#include <string>
#include <functional>
#include <vector>
#include <iostream>

class Flag
{
	std::string name = "";
	bool value = false;
	bool required = false;
	std::function<void()> callback = nullptr;
	bool HasCallback = false;

public:
	Flag(std::string name, bool required, std::function<void()> callback = nullptr)
	{
		this->name = name;
		this->required = required;
		this->callback = callback;
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
	std::function<void(std::string)> callback = nullptr;
	bool HasCallback = false;

public:
	Parameter(std::string name, bool required, std::function<void(std::string)> callback = nullptr)
	{
		this->name = name;
		this->required = required;
		this->callback = callback;
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
							flags[j]->execute();
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
							parameters[j]->execute();
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