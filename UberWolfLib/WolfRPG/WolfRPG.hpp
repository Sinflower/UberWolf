/*
 *  File: WolfRPG.hpp
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

#include "CommonEvents.hpp"
#include "Database.hpp"
#include "GameDat.hpp"
#include "Map.hpp"
#include "Types.hpp"

#include <filesystem>

class WolfRPG
{
public:
	explicit WolfRPG(const std::filesystem::path& dataPath, const bool& skipGD = false, const bool& saveUncompressed = false) :
		m_dataPath(dataPath),
		m_skipGD(skipGD),
		m_saveUncompressed(saveUncompressed)
	{
		try
		{
			WolfDataBase::SetUncompressedPath("uncompressed");
			loadGameDat();
			loadCommonEvents();
			loadDatabases();
			loadMaps();

			m_valid = true;
		}
		catch (WolfRPGException& e)
		{
			std::wcerr << std::endl
					   << "Error while processing: " << g_activeFile << std::endl
					   << e.what() << std::endl;
		}
	}

	~WolfRPG()
	{
	}

	const bool& Valid() const
	{
		return m_valid;
	}

	void Save2File(const std::filesystem::path& outputPath) const
	{
		checkValid();

		std::filesystem::path basicDataDir = outputPath / "BasicData";

		CheckAndCreateDir(basicDataDir);

		if (!m_skipGD)
		{
			std::cout << "Writing Game.dat to file ... ";
			m_gameDat.Dump(outputPath, m_dataPath);
			std::cout << "Done" << std::endl;
		}

		std::cout << "Writing CommonEvents to file ... ";
		m_commonEvents.Dump(outputPath, m_dataPath);
		std::cout << "Done" << std::endl;

		std::cout << "Writing Databases to file ... ";
		for (const Database& db : m_databases)
			db.Dump(basicDataDir);
		std::cout << "Done" << std::endl;

		std::cout << "Writing Maps to file ... ";
		for (const Map& map : m_maps)
			map.Dump(outputPath, m_dataPath);
		std::cout << "Done" << std::endl;
	}

	GameDat& GetGameDat()
	{
		checkValid();
		return m_gameDat;
	}

	const GameDat& GetGameDat() const
	{
		checkValid();
		return m_gameDat;
	}

	Maps& GetMaps()
	{
		checkValid();
		return m_maps;
	}

	const Maps& GetMaps() const
	{
		checkValid();
		return m_maps;
	}

	CommonEvents& GetCommonEvents()
	{
		checkValid();
		return m_commonEvents;
	}

	const CommonEvents& GetCommonEvents() const
	{
		checkValid();
		return m_commonEvents;
	}

	Databases& GetDatabases()
	{
		checkValid();
		return m_databases;
	}

	const Databases& GetDatabases() const
	{
		checkValid();
		return m_databases;
	}

private:
	void checkValid() const
	{
		if (!m_valid)
			throw WolfRPGException(ERROR_TAGW + L"Invalid WolfRPG object");
	}

	void loadGameDat()
	{
		if (m_skipGD) return;

		std::cout << "Loading Game.dat ... " << std::flush;

		m_gameDat = GameDat(m_dataPath / "BasicData/Game.dat", m_saveUncompressed);

		std::cout << "Done" << std::endl;
	}

	void loadMaps()
	{
		std::cout << "Loading Maps ... " << std::flush;

		size_t prevLength = 0;
		for (std::filesystem::directory_entry p : std::filesystem::recursive_directory_iterator(m_dataPath))
		{
			std::filesystem::path pp = p.path();
			if (pp.extension() == ".mps")
			{
				std::wcout << "\rLoading Map: " << pp.filename() << std::setfill(TCHAR(' ')) << std::setw(prevLength) << "" << std::flush;
				prevLength = pp.filename().wstring().length();
				m_maps.push_back(Map(pp, m_saveUncompressed));
			}
		}

		std::cout << "\rLoading Maps ... Done" << std::setfill(' ') << std::setw(prevLength) << "" << std::endl;
	}

	void loadCommonEvents()
	{
		std::cout << "Loading CommonEvents ... " << std::flush;

		m_commonEvents = CommonEvents(m_dataPath / "BasicData/CommonEvent.dat", m_saveUncompressed);

		std::cout << "Done" << std::endl;
	}

	void loadDatabases()
	{
		std::cout << "Loading Databases ... " << std::flush;

		for (std::filesystem::directory_entry p : std::filesystem::directory_iterator(m_dataPath / "BasicData"))
		{
			std::filesystem::path pp = p.path();
			if (pp.extension() == ".project" && pp.filename() != "SysDataBaseBasic.project")
			{
				std::filesystem::path projectFile = pp;
				std::filesystem::path datFile     = pp;
				datFile.replace_extension(".dat");
				m_databases.push_back(Database(projectFile, datFile));
			}
		}

		std::cout << "Done" << std::endl;
	}

private:
	std::filesystem::path m_dataPath;
	bool m_skipGD;
	bool m_saveUncompressed;

	GameDat m_gameDat;
	Maps m_maps;
	CommonEvents m_commonEvents;
	Databases m_databases;

	bool m_valid = false;
};
