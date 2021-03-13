#include "Halide.h"

namespace {

using namespace Halide;
using namespace Halide::ConciseCasts;

class ResnetKernel : public Halide::Generator<ResnetKernel> {
public:
    Input<Buffer<int16_t>>  input{"input", 3};
    Input<Buffer<int16_t>>  kernel{"kernel", 3};
    Output<Buffer<int16_t>> output{"output", 3};
	
    void generate() {
        Var x, y, z;
		
		output(x, y, z) = input(x, y, z) + kernel(x, y, z);
        
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ResnetKernel, resnet_layer_gen)
