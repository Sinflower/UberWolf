#include "UberWolfLib.h"
#include "WolfDec.h"
#include "WolfKeyFinder.h"
#include "WolfPro.h"
#include "Utils.h"
#include "UberLog.h"
#include "resource.h"

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

static const tString EXTENSION = TEXT(".wolf");

static const tStrings GAME_EXE_NAMES = {
	TEXT("Game.exe"),
	TEXT("GamePro.exe")
};

static const tString DATA_FOLDER_NAME = TEXT("Data");

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
	else if (argv.size() >= 2)
		InitGame(argv[1]);
}

UberWolfLib::UberWolfLib(int argc, char* argv[]) :
	UberWolfLib(argvToList(argc, argv))
{
}

bool UberWolfLib::InitGame(const tString& gameExePath)
{
	m_valid = false;

	if (!fs::exists(gameExePath))
	{
		ERROR_LOG << TEXT("UberWolfLib: Invalid game executable path: ") << gameExePath << std::endl;
		return false;
	}

	m_gameExePath = gameExePath;

	if (!findDataFolder()) return false;

	m_wolfPro = WolfPro(m_dataFolder);

	m_valid = true;
	return m_valid;
}

UWLExitCode UberWolfLib::UnpackData()
{
	if (!m_valid)
		return UWLExitCode::NOT_INITIALIZED;

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
	if (!m_valid)
		return UWLExitCode::NOT_INITIALIZED;

	if (findDxArcKeyFile() == UWLExitCode::SUCCESS)
		return UWLExitCode::SUCCESS;

	return findDxArcKeyInject();
}

UWLExitCode UberWolfLib::FindProtectionKey(std::string& key)
{
	if (!m_valid)
		return UWLExitCode::NOT_INITIALIZED;

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

UWLExitCode UberWolfLib::FindProtectionKey(std::wstring& key)
{
	std::string sKey;
	UWLExitCode uec = FindProtectionKey(sKey);
	if (uec != UWLExitCode::SUCCESS) return uec;

	key = StringToWString(sKey);
	return UWLExitCode::SUCCESS;
}

std::size_t UberWolfLib::RegisterLogCallback(const LogCallback& callback)
{
	return uberLog::AddLogCallback(callback);
}

void UberWolfLib::UnregisterLogCallback(const std::size_t& idx)
{
	uberLog::RemoveLogCallback(idx);
}

UWLExitCode UberWolfLib::unpackArchive(const tString& archivePath)
{
	// Make sure the file exists
	if (!fs::exists(archivePath))
		return UWLExitCode::FILE_NOT_FOUND;

	if (archivePath.empty())
		return UWLExitCode::INVALID_PATH;

	if (!m_wolfDec)
		return UWLExitCode::WOLF_DEC_NOT_INITIALIZED;

	INFO_LOG << TEXT("Unpacking: ") << fs::path(archivePath).filename() << TEXT("... ");
	bool result = m_wolfDec.UnpackArchive(archivePath.c_str());

	if (!result)
	{
		if (!m_valid)
		{
			if (!findGameFromArchive(archivePath))
				return UWLExitCode::NOT_INITIALIZED;
		}

		UWLExitCode uec = FindDxArcKey();

		if (uec == UWLExitCode::SUCCESS)
			result = m_wolfDec.UnpackArchive(archivePath.c_str());
	}

	INFO_LOG << (result ? TEXT("Done") : TEXT("Failed")) << std::endl;
	return result ? UWLExitCode::SUCCESS : UWLExitCode::KEY_MISSING;
}

bool UberWolfLib::findDataFolder()
{
	// Get the folder where the game executable is located
	tString gameFolder = FS_PATH_TO_TSTRING(fs::path(m_gameExePath).parent_path());

	// Check if the data folder exists
	if (fs::exists(gameFolder + TEXT("/") + DATA_FOLDER_NAME))
	{
		m_dataFolder = gameFolder + TEXT("/") + DATA_FOLDER_NAME;
		return true;
	}

	// Check if data.wolf exists in the game folder
	if (fs::exists(gameFolder + TEXT("/data.wolf")))
	{
		m_dataFolder = gameFolder;
		return true;
	}

	ERROR_LOG << TEXT("UberWolfLib: Could not find data folder") << std::endl;
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

bool UberWolfLib::findGameFromArchive(const tString& archivePath)
{
	// Get the folder containing the archive
	tString searchFolder = FS_PATH_TO_TSTRING(fs::path(archivePath).parent_path());

	// Check if the name of the search folder is the data folder, if search for the game executable in the parent folder
	if (fs::path(searchFolder).filename() == DATA_FOLDER_NAME)
		searchFolder = FS_PATH_TO_TSTRING(fs::path(searchFolder).parent_path());

	INFO_LOG << TEXT("Searching for game executable in: ") << searchFolder << std::endl;

	// Check if the game executable exists in the search folder
	for (const auto& gameExeName : GAME_EXE_NAMES)
	{
		const tString gameExePath = searchFolder + TEXT("/") + gameExeName;
		if (fs::exists(gameExePath))
		{
			INFO_LOG << TEXT("Found game executable: ") << gameExeName << std::endl;
			return InitGame(gameExePath);
		}
	}

	INFO_LOG << TEXT("Could not find the game executable") << std::endl;

	return false;
}

bool UberWolfLib::copyDllFromResource(const tString& outDir) const
{
	INFO_LOG << TEXT("Copying resource...") << std::endl;

	const HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_KEY_HOOK), RT_RCDATA);
	if (hResource == NULL)
	{
		ERROR_LOG << TEXT("UberWolfLib: Failed to find resource") << std::endl;
		return false;
	}

	const HGLOBAL hResData = LoadResource(NULL, hResource);
	if (hResData == NULL)
	{
		ERROR_LOG << TEXT("UberWolfLib: Failed to load resource") << std::endl;
		return false;
	}

	const LPVOID lpResourceData = LockResource(hResData);
	const DWORD dwResourceSize = SizeofResource(NULL, hResource);

	const tString dllPath = outDir + TEXT("/") + WolfKeyFinder::DLL_NAME;

	HANDLE hFile = CreateFile(dllPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		ERROR_LOG << TEXT("UberWolfLib: Failed to create file") << std::endl;
		return false;
	}

	DWORD bytesWritten = 0;
	WriteFile(hFile, lpResourceData, dwResourceSize, &bytesWritten, NULL);

	CloseHandle(hFile);
	INFO_LOG << TEXT("UberWolfLib: Resource written to file") << std::endl;

	return true;
}
