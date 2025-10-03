#include "Application/Application.h"

#include <iostream>
#include <exception>
#include <vector>
int main()
{
	ev::Application* app = new ev::Application();
	app->init();

	try
	{
		app->run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		app->clear();
		delete app;
		return EXIT_FAILURE;
	}

	app->clear();
	delete app;
	return EXIT_SUCCESS;
}