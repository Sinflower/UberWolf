#include <iostream>
#include <windows.h>
#include <vector>

#include <UberWolfLib.h>

int main(int argc, char* argv[])
{
	UberWolfLib uwl(argc, argv);

	UWLExitCode uec = uwl.UnpackData();

	std::string key;

	if(uwl.FindProtectionKey(key) == UWLExitCode::SUCCESS)
		std::cout << "Protection key: " << key << std::endl;
	else
		std::cout << "Protection key not found!" << std::endl;

	return 0;
}
