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

#include <CLI11/CLI11.hpp>
#include <filesystem>
#include <format>
#include <iostream>
#include <vector>
#include <windows.h>

#include <UberWolfLib.h>
#include <Utils.h>

namespace fs = std::filesystem;

static const std::string UWCLI_VERSION = "0.3.1";
static const std::string UWCLI_NAME    = "UberWolfCli";

std::string buildPackInfo()
{
	Strings crypts   = UberWolfLib::GetEncryptions();
	std::string info = "";
	for (std::size_t i = 0; i < crypts.size(); i++)
	{
		if (!info.empty())
			info += "\n";
		info += std::format("{} - {}", i, crypts[i]);
	}

	return info;
}

int main(int argc, char* argv[])
{
	if (IsSubProcess())
	{
		UberWolfLib uwl;
		return 0;
	}

	CLI::App app{ UWCLI_NAME + " v" + UWCLI_VERSION };
	argv = app.ensure_utf8(argv);

	tStrings files;
	app.add_option("FILE[s]", files, "<Game[Pro].exe>\n<data_folder>\n<.wolf-files>")->required();

	bool override = false;
	app.add_flag("-o,--override", override, "Override existing files");

	bool unprotect = false;
	app.add_flag("-u,--unprotect", unprotect, "Unprotect Pro files");

	std::string packVersion = "";
	app.add_option("-p,--pack", packVersion, buildPackInfo())->type_name("VER_IDX");

	CLI11_PARSE(app, argc, argv);

	const tStrings zeroArg = { StringToWString(argv[0]) };
	UberWolfLib uwl(zeroArg);

	if (files.empty())
	{
		std::cout << "No files specified." << std::endl;
		return -1;
	}

	uwl.Configure(override, unprotect);

	// Check if the first argument is an executable
	if (fs::exists(files.front()) && fs::is_regular_file(files.front()) && fs::path(files.front()).extension() == ".exe")
	{
		uwl.InitGame(files.front());

		if (packVersion.empty())
		{
			uwl.UnpackData();

			std::string key;

			if (uwl.FindProtectionKey(key) == UWLExitCode::SUCCESS)
				std::cout << "Protection key: " << key << std::endl;
		}
		else
		{
			int32_t encIdx = -1;

			try
			{
				encIdx = std::stoi(packVersion);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Invalid package version index - " << packVersion << std::endl;
				std::cerr << "Error: " << e.what() << std::endl;
				return -1;
			}

			if (encIdx < 0 || static_cast<std::size_t>(encIdx) >= UberWolfLib::GetEncryptions().size())
			{
				std::cout << "Invalid package version index" << std::endl;
				return -1;
			}

			UWLExitCode result = uwl.PackData(encIdx);
			if (result != UWLExitCode::SUCCESS)
				std::cout << "PackData failed with exit code: " << static_cast<int>(result) << std::endl;
		}

		return 0;
	}

	if (!packVersion.empty())
	{
		std::cerr << "[ERROR] Currently, packing can only be used with an executable" << std::endl;
		return -1;
	}

	tStrings paths;

	// Check if the first argument is a folder
	if (fs::exists(files.front()) && fs::is_directory(files.front()))
	{
		for (const auto& entry : fs::directory_iterator(files.front()))
		{
			if (entry.is_regular_file())
				paths.push_back({ FS_PATH_TO_TSTRING(entry.path()) });
		}
	}
	else
	{
		for (size_t i = 0; i < files.size(); i++)
		{
			if (fs::exists(files[i]) && fs::is_regular_file(files[i]))
				paths.push_back({ files[i] });
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
