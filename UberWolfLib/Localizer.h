/*
 *  File: Localizer.h
 *  Copyright (c) 2024 Sinflower
 *
 *  MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

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