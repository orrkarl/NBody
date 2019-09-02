#include "NBodyProgram.h"

const std::string NBODY_CODE = R"__CODE__(

typedef struct _particle
{
    float2 position;
    float2 velocity;
    float mass;
} particle;

kernel void process_particles(
    global const particle* source,
    const uint particle_count,
    const float step_size,
    global particle* destination)
{

}

)__CODE__";

NBodyProgram::NBodyProgram(const cl::Context& ctx)
    : cl::Program(ctx, NBODY_CODE, true)
{
}
