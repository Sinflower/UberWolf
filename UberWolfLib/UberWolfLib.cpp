#include "UberWolfLib.h"
#include "WolfDec.h"
#include "WolfKeyFinder.h"
#include "WolfPro.h"
#include "Utils.h"

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

static const tString EXTENSION = TEXT(".wolf");

// TODO: Maybe implement error callbacks or something that notifies the application about errors

// For parent process calls the first argument is expected to be the game executable path
UberWolfLib::UberWolfLib(const tStrings& argv) :
	m_gameExePath(TEXT("")),
	m_dataFolder(TEXT("")),
	m_valid(false)
{
	if (argv.size() < 1)
		throw std::runtime_error("UberWolfLib: Invalid arguments count");

	uint32_t mode = -1;
	tString path = TEXT("");
	bool isSubProcess = IsSubProcess();

	if (isSubProcess && argv.size() >= 3)
	{
		for (std::size_t i = 0; i < argv.size(); i++)
		{
			if (argv[i] == TEXT("-m"))
			{
				if (i + 2 >= argv.size())
					throw std::runtime_error("UberWolfLib: -m argument requires two values");

				mode = std::stoi(WStringToString(argv[i + 1]));
				path = argv[i + 2];
				break;
			}
		}
	}

	m_wolfDec = WolfDec(argv[0], mode, isSubProcess);

	if (isSubProcess)
	{
		// For subprocess calls the mode was passed as an argument
		// directly call unpack, which in this case will also terminate the process
		m_wolfDec.UnpackArchive(path.c_str());
		ExitProcess(0);
	}
	else
		InitGame(argv[0]);
}

UberWolfLib::UberWolfLib(int argc, char* argv[]) :
	UberWolfLib(argvToList(argc, argv))
{
}

bool UberWolfLib::InitGame(const tString& gameExePath)
{
	m_valid = false;
	m_gameExePath = gameExePath;

	if (!fs::exists(m_gameExePath))
	{
		tcerr << TEXT("UberWolfLib: Invalid game executable path: ") << gameExePath << std::endl;
		return false;
	}

	if(!findDataFolder()) return false;

	m_wolfPro = WolfPro(m_dataFolder);

	m_valid = true;
	return m_valid;
}

UWLExitCode UberWolfLib::UnpackData()
{
	for (const auto& dirEntry : fs::directory_iterator(m_dataFolder))
	{
		if (dirEntry.path().extension() == ".wolf")
		{
			tString target = FS_PATH_TO_TSTRING(dirEntry.path());
			UWLExitCode uec = unpackArchive(target);
			if (uec != UWLExitCode::SUCCESS) return uec;
		}
	}

	return UWLExitCode::SUCCESS;
}

UWLExitCode UberWolfLib::UnpackArchive(const tString& archivePath)
{
	return unpackArchive(archivePath);
}

UWLExitCode UberWolfLib::FindDxArcKey()
{
	if (findDxArcKeyFile() == UWLExitCode::SUCCESS)
		return UWLExitCode::SUCCESS;

	return findDxArcKeyInject();
}

UWLExitCode UberWolfLib::FindProtectionKey(std::string& key)
{
	if (!m_wolfPro.IsWolfPro())
		return UWLExitCode::NOT_WOLF_PRO;

	// Check if the protection file exists
	m_wolfPro.RecheckProtFileState();

	if (m_wolfPro.NeedsUnpacking())
	{
		const tString target = m_dataFolder + TEXT("/") + m_wolfPro.GetProtKeyArchiveName() + EXTENSION;
		if (unpackArchive(target) != UWLExitCode::SUCCESS) return UWLExitCode::UNPACK_FAILED;
	}


	Key keyVec = m_wolfPro.GetProtectionKey();
	if (keyVec.empty())
		return UWLExitCode::PROT_KEY_DETECT_FAILED;

	key = "";
	for (const uint8_t& byte : keyVec)
		key += static_cast<char>(byte);

	return UWLExitCode::SUCCESS;
}

UWLExitCode UberWolfLib::unpackArchive(const tString& archivePath)
{
	// Make sure the file exists
	if (!fs::exists(archivePath))
		return UWLExitCode::FILE_NOT_FOUND;

	if (archivePath.empty())
		return UWLExitCode::INVALID_PATH;

	tcout << TEXT("Unpacking: ") << fs::path(archivePath).filename() << TEXT("... ");
	bool result = m_wolfDec.UnpackArchive(archivePath.c_str());

	if (!result)
	{
		UWLExitCode uec = FindDxArcKey();

		if (uec == UWLExitCode::SUCCESS)
			result = m_wolfDec.UnpackArchive(archivePath.c_str());
	}

	tcout << (result ? TEXT("Done") : TEXT("Failed")) << std::endl;
	return result ? UWLExitCode::SUCCESS : UWLExitCode::KEY_MISSING;
}

bool UberWolfLib::findDataFolder()
{
	// Get the folder where the game executable is located
	tString gameFolder = FS_PATH_TO_TSTRING(fs::path(m_gameExePath).parent_path());

	// Check if the data folder exists
	if (fs::exists(gameFolder + TEXT("/Data")))
	{
		m_dataFolder = gameFolder + TEXT("/Data");
		return true;
	}

	// Check if data.wolf exists in the game folder
	if (fs::exists(gameFolder + TEXT("/data.wolf")))
	{
		m_dataFolder = gameFolder;
		return true;
	}

	tcerr << TEXT("UberWolfLib: Could not find data folder") << std::endl;
	return false;
}

UWLExitCode UberWolfLib::findDxArcKeyFile()
{
	if (!m_wolfPro.IsWolfPro())
		return UWLExitCode::NOT_WOLF_PRO;

	const Key key = m_wolfPro.GetDxArcKey();

	if (key.empty())
		return UWLExitCode::KEY_DETECT_FAILED;

	m_wolfDec.AddKey("UNKNOWN_PRO", false, key);
	updateConfig(false, key);

	return UWLExitCode::SUCCESS;
}

UWLExitCode UberWolfLib::findDxArcKeyInject()
{
	WolfKeyFinder wkf(m_gameExePath);

	std::vector<BYTE> key;
	if (!wkf.Inject())
		return UWLExitCode::KEY_DETECT_FAILED;

	m_wolfDec.AddKey("UNKNOWN", wkf.UseOldDxArc(), wkf.GetKey());
	updateConfig(wkf.UseOldDxArc(), wkf.GetKey());

	return UWLExitCode::SUCCESS;
}

void UberWolfLib::updateConfig(const bool& useOldDxArc, const Key& key)
{
	nlohmann::json data;

	// Load the config file if it exists and is not empty
	if (fs::exists(WolfDec::CONFIG_FILE_NAME) && fs::file_size(WolfDec::CONFIG_FILE_NAME) > 0)
	{
		std::ifstream f(WolfDec::CONFIG_FILE_NAME);
		data = nlohmann::json::parse(f);
	}
	// Create a new config file
	else
	{
		data["keys"] = nlohmann::json::object();
	}

	uint32_t cnt = 0;
	const std::string NAME_BASE = "UNKNOWN_";
	std::string name = NAME_BASE + std::to_string(cnt);

	// Check if a key with the name already exists
	while (data["keys"].contains(name))
	{
		cnt++;
		name = NAME_BASE + std::to_string(cnt);
	}

	// Add the key to the config
	data["keys"][name] = nlohmann::json::object();
	data["keys"][name]["mode"] = (useOldDxArc ? "VER6" : "VER8");
	// Write the key as an array of 0x prefixed hex strings
	data["keys"][name]["key"] = nlohmann::json::array();
	for (const auto& byte : key)
		data["keys"][name]["key"].push_back("0x" + ByteToHexString(byte));

	// Write the config file
	std::ofstream f(WolfDec::CONFIG_FILE_NAME);
	f << data.dump(4);
}
