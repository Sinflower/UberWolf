#pragma once

#include <functional>
#include <map>
#include <string>

#include "Types.h"

#define LOCALIZE(key) uberWolfLib::Localizer::GetInstance().GetValueT(key)

namespace uberWolfLib
{

class Localizer
{
	using LocMapT = std::map<std::string, tString>;
	static LocMapT s_localizationT;
	static tString s_errorStringT;

public:
	static Localizer& GetInstance()
	{
		static Localizer instance;
		return instance;
	}

	Localizer(Localizer const&)      = delete;
	void operator=(Localizer const&) = delete;

	const tString& GetValueT(const std::string& key) const;
	// Method to register an external function to query localizations from

	void RegisterLocQuery(const LocalizerQuery& func)
	{
		m_localizationFunction = func;
	}

private:
	Localizer()  = default;
	~Localizer() = default;

private:
	LocalizerQuery m_localizationFunction;
};

}; // namespace uberWolfLib