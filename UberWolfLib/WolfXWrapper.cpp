/*
 *  File: WolfXWrapper.cpp
 *  Copyright (c) 2025 Sinflower
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

#include "WolfXWrapper.h"

#include "WolfRPG/WolfRPG.h"
#include "UberLog.h"

#include <filesystem>

bool WolfXWrapper::DecryptAll()
{
	if (m_dataFolder.empty())
	{
		ERROR_LOG << "[WolfXWrapper] Data folder is empty" << std::endl;
		return false;
	}

	if (!std::filesystem::exists(m_dataFolder))
	{
		ERROR_LOG << "[WolfXWrapper] Data folder does not exist" << std::endl;
		return false;
	}

	wolfx::WolfXFiles wolfXFiles = wolfx::utils::collectWolfXFiles(m_dataFolder);

	if (wolfXFiles.empty())
		return true;

	INFO_LOG << "Found " << wolfXFiles.size() << " WolfX files" << std::endl;

	collectWolfXDecryptionInfo();

	INFO_LOG << "Decrypting WolfX files ... " << std::flush;
	wolfx::crackWolfXFiles(wolfXFiles, m_wolfxDecryptCollection);
	INFO_LOG << "Done" << std::endl;
	return true;
}

void WolfXWrapper::collectWolfXDecryptionInfo()
{
	WolfRPG wolfRpg(m_dataFolder, true);

	m_wolfxDecryptCollection.clear();
	m_wolfxDecryptCollection.decryptKeys.push_back({ "/", "" }); // Add an empty entry to also test the default case

	auto parseCommand = [&wolfxDecryptCollection = m_wolfxDecryptCollection](const Command::CommandShPtr::Command &command) {
		switch (command->GetType())
		{
			case Command::CommandType::SetString:
			{
				Command::CommandShPtr::SetString setString = std::dynamic_pointer_cast<Command::CommandSpecialClasses::SetString>(command);

				const uint32_t idx = setString->GetID();
				wolfxDecryptCollection.stringValues[idx].insert(setString->GetString());
				break;
			}
			case Command::CommandType::SetVariable:
			{
				Command::CommandShPtr::SetVariable setVariable = std::dynamic_pointer_cast<Command::CommandSpecialClasses::SetVariable>(command);

				const uint32_t idx = setVariable->GetID();
				wolfxDecryptCollection.intValues[idx].insert(setVariable->GetValue());
				break;
			}
			case Command::CommandType::ProFeature:
			{
				Command::CommandShPtr::ProFeature proFeature = std::dynamic_pointer_cast<Command::CommandSpecialClasses::ProFeature>(command);
				if (proFeature->GetProFeatureType() == Command::CommandSpecialClasses::ProFeature::Type::SetWolfxKey)
					wolfxDecryptCollection.decryptKeys.push_back({ fileAccessUtils::ws2s(proFeature->GetWolfxFolder()), fileAccessUtils::ws2s(proFeature->GetWolfxKey()) });
				break;
			}
			default:
				break;
		}
	};

	INFO_LOG << "Collecting WolfX decryption information ... " << std::flush;

	for (const Map &map : wolfRpg.GetMaps())
	{
		for (const Event &event : map.GetEvents())
		{
			for (const Page &page : event.GetPages())
			{
				for (const Command::CommandShPtr::Command &command : page.GetCommands())
					parseCommand(command);
			}
		}
	}

	for (const CommonEvent &comEv : wolfRpg.GetCommonEvents().GetEvents())
	{
		for (const Command::CommandShPtr::Command &command : comEv.GetCommands())
			parseCommand(command);
	}

	// Unique the wolfxDecryptInfos
	std::sort(m_wolfxDecryptCollection.decryptKeys.begin(), m_wolfxDecryptCollection.decryptKeys.end(), [](const wolfx::WolfXDecryptKey &a, const wolfx::WolfXDecryptKey &b) {
		return a.folder < b.folder || (a.folder == b.folder && a.key < b.key);
	});

	m_wolfxDecryptCollection.decryptKeys.erase(std::unique(m_wolfxDecryptCollection.decryptKeys.begin(), m_wolfxDecryptCollection.decryptKeys.end()), m_wolfxDecryptCollection.decryptKeys.end());

	auto countEntires = []<typename T>(const T &m) {
		std::size_t count = 0;
		for (const auto [k, v] : m)
			count += v.size();
		return count;
	};

	INFO_LOG << "Done" << std::endl;
#if 0 // Old debug prints, keep for now
	INFO_LOG << "Found: " << std::endl
			  << m_wolfxDecryptCollection.decryptKeys.size() << " decryption keys" << std::endl
			  << countEntires(m_wolfxDecryptCollection.stringValues) << " string values" << std::endl
			  << countEntires(m_wolfxDecryptCollection.intValues) << " int values" << std::endl;

	for (const auto &info : m_wolfxDecryptCollection.decryptKeys)
		INFO_LOG << "Folder: \"" << info.folder << "\", Key: \"" << info.key << "\"" << std::endl;
#endif
}
