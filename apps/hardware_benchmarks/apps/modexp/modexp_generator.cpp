#include "Halide.h"
#include "complex.h"
#include <math.h>

namespace {

using namespace std;
using namespace Halide;
using namespace Halide::ConciseCasts;

class modexp : public Halide::Generator<modexp> {
	public:
		Input<Buffer<uint16_t>> a{"a", 1};
		Input<Buffer<uint16_t>> c{"c", 1};
		Output<Buffer<uint16_t>> output{"output", 1};

    void generate() {
		Var x;
		Func hw_a, hw_c, hw_output;
		
		hw_a(x) = a(x);
		hw_c(x) = c(x);
		
		hw_output(x) = cast<uint16_t>(1);
		
		
		for (int i = 0; i < 30; i++)
		{
			hw_output(0) = (hw_output(0) * hw_a(0)) % hw_c(0);
		}
		
		output(x) = hw_output(0);
		
		
		if (get_target().has_feature(Target::Clockwork)) 
		{			  
			  Var xi, xo;
					  
			  output.bound(x, 0, 1);

			  hw_output.compute_root();
			  
			  hw_output.split(x, xo, xi, 1).reorder(xi, xo).hw_accelerate(xi, xo);			
			  
			  hw_a.stream_to_accelerator();
			  hw_c.stream_to_accelerator();
        } else 
		{
			output.compute_root();
        }
		
		
		
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(modexp, modexp)