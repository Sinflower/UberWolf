#include <iostream>
#include <windows.h>
#include <vector>

#include <UberWolfLib.h>

int main(int argc, char* argv[])
{
	// TODO: This needs to be passed somehow -- ignore for sub process?
	const tString gameExe = TEXT("");

	UberWolfLib uwl(argc, argv, gameExe);

	UWLExitCode uec = uwl.UnpackData();

	std::string key;

	if(uwl.FindProtectionKey(key) == UWLExitCode::SUCCESS)
		std::cout << "Protection key: " << key << std::endl;
	else
		std::cout << "Protection key not found!" << std::endl;

	return 0;
}
