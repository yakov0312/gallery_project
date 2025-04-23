#include <iostream>
#include <string>
#include <chrono>
#include "DatabaseAccess.h"
#include "AlbumManager.h"

void printProgramDetails();

int getCommandNumberFromUser()
{
	std::string message("\nPlease enter any command(use number): ");
	std::string numericStr("0123456789");
	
	std::cout << message << std::endl;
	std::string input;
	std::getline(std::cin, input);
	
	while (std::cin.fail() || std::cin.eof() || input.find_first_not_of(numericStr) != std::string::npos) 
	{

		std::cout << "Please enter a number only!" << std::endl;

		if (input.find_first_not_of(numericStr) == std::string::npos) 
		{
			std::cin.clear();
		}

		std::cout << std::endl << message << std::endl;
		std::getline(std::cin, input);
	}
	
	return std::atoi(input.c_str());
}

int main(void)
{
	// initialization data access
	DatabaseAccess dataAccess;
	SetConsoleCtrlHandler(AlbumManager::processClose, TRUE);

	// initialize album manager
	AlbumManager albumManager(dataAccess);


	std::string albumName;
	printProgramDetails();
	std::cout << "Welcome to Gallery!" << std::endl;
	std::cout << "===================" << std::endl;
	std::cout << "Type " << HELP << " to a list of all supported commands" << std::endl;
	
	while (true)
	{
		int commandNumber = getCommandNumberFromUser();

		try
		{
			albumManager.executeCommand(static_cast<CommandType>(commandNumber));
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
}

void printProgramDetails()
{
	auto now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);
	std::cout << "Created by yakov" << std::endl << "Date: " << std::put_time(std::localtime(&now_c), "%Y/%m/%d - %T") << std::endl;
}
