/*
 *  File: UberWolfCli.cpp
 *  Copyright (c) 2023 Sinflower
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

#include <filesystem>
#include <iostream>
#include <vector>
#include <windows.h>

#include <UberWolfLib.h>
#include <Utils.h>

namespace fs = std::filesystem;

static const std::string UWCLI_VERSION = "0.1.0";
static const std::string UWCLI_NAME  = "UberWolfCli";

void printHelp()
{
	std::cout << "UberWolfCli v" << UWCLI_VERSION << std::endl;
	std::cout << "Usage: " << std::endl;
	std::cout << "  " << UWCLI_NAME << " <Game[Pro].exe>" << std::endl;
	std::cout << "  " << UWCLI_NAME << " <data_folder>" << std::endl;
	std::cout << "  " << UWCLI_NAME << " <.wolf-files>" << std::endl;
}

int main(int argc, char* argv[])
{
	if (IsSubProcess())
	{
		UberWolfLib uwl;
		return 0;
	}

	tStrings args          = argvToList(argc, argv);
	const tStrings zeroArg = { args[0] };
	UberWolfLib uwl(zeroArg);

	// If no arguments are passed, print the help message
	if (args.size() == 1)
	{
		printHelp();
		return 0;
	}

	// Check if the first argument is an executable
	if (fs::exists(args[1]) && fs::is_regular_file(args[1]) && fs::path(args[1]).extension() == ".exe")
	{
		uwl.InitGame(args[1]);
		uwl.UnpackData();

		std::string key;

		if (uwl.FindProtectionKey(key) == UWLExitCode::SUCCESS)
			std::cout << "Protection key: " << key << std::endl;

		return 0;
	}

	tStrings paths;

	// Check if the first argument is a folder
	if (fs::exists(args[1]) && fs::is_directory(args[1]))
	{
		for (const auto& entry : fs::directory_iterator(args[1]))
		{
			if (entry.is_regular_file())
				paths.push_back({ FS_PATH_TO_TSTRING(entry.path()) });
		}
	}
	else
	{
		for (size_t i = 1; i < args.size(); i++)
		{
			if (fs::exists(args[i]) && fs::is_regular_file(args[i]))
				paths.push_back({ args[i] });
		}
	}

	if (paths.empty())
	{
		std::cout << "No valid files found." << std::endl;
		return -1;
	}

	uwl.UnpackDataVec(paths);

	return 0;
}
