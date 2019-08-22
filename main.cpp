#include <iostream>
#include <stdexcept>

#include "common.h"

#include "NBody.h"

int main()
{
	NBodyInitializeGuard guard;

	{

		try
		{
			NBody sim(1000, 1e-3);

			sim.run();
		}
		catch (const cl::Error& ex)
		{
			std::cerr << "Error while running simulation: " << ex.what() << " (" << ex.err() << ")\n";
		}
		catch (const std::exception& ex)
		{
			std::cerr << "Error while running simulation: " << ex.what() << '\n';
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

