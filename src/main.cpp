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
			std::cerr << "Error while running simulation:\n\t" << ex.what() << ": " << stringFromCLError(ex.err()) << " (" << ex.err() << ")\n";
			return EXIT_FAILURE;
		}
		catch (const std::exception& ex)
		{
			std::cerr << "Error while running simulation:\n\t" << ex.what() << '\n';
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

