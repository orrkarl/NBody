#include <iostream>

#include <stdexcept>

#include "NBody.h"

int main()
{
	NBody sim(1000, 1e-3);

	try
	{
		sim.run();
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Exception occured: " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

