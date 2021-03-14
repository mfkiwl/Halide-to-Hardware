#include "Halide.h"
#include "complex.h"
#include <math.h>

namespace {

using namespace std;
using namespace Halide;
using namespace Halide::ConciseCasts;

class FFT512 : public Halide::Generator<FFT512> {
	public:
		Input<Buffer<float>> input{"in1", 2};
		Output<Buffer<float>> output{"output", 2};

    void generate() {
		Var x, y, z;
		RDom t = RDom(0, 512);
		
		
		// bit reverse
		Func hw_input, non_rev, rev, d8, d7, d6, d5, d4, d3, d2, d1, d0;
		
		non_rev(x) = x;
				 
		d8(x) = (non_rev(x) >> 8);
		d7(x) = (non_rev(x) >> 7) & 1;
		d6(x) = (non_rev(x) >> 6) & 1;
		d5(x) = (non_rev(x) >> 5) & 1;
		d4(x) = (non_rev(x) >> 4) & 1;
		d3(x) = (non_rev(x) >> 3) & 1;
		d2(x) = (non_rev(x) >> 2) & 1;
		d1(x) = (non_rev(x) >> 1) & 1;
		d0(x) = non_rev(x) & 1;
		
		rev(x) = (d0(x) << 8)
				 + (d1(x) << 7)
				 + (d2(x) << 6)
				 + (d3(x) << 5)
				 + (d4(x) << 4)
				 + (d5(x) << 3)
				 + (d6(x) << 2)
				 + (d7(x) << 1)
				 + d8(x);
				 
		hw_input(x, y) = input(rev(x), y); // stream this into accelerator
		//////////////////////////////////////////
		


		// twiddling factor
		ComplexFunc twi;
		Func hw_twi;
		
		float PI = atan(1)*4;
		twi(x) = expj(-2 * PI * x / 512);
		
		hw_twi(x, y, z) = 0.0f;
		for (int s = 1; s <= 9; s++)
		{
			int tmp = 1 << s;
			
			hw_twi(t.x, 0, s - 1) = twi(((t.x % tmp) / (tmp / 2)) * (t.x % (tmp / 2)) * (512 / tmp)).x;
			hw_twi(t.x, 1, s - 1) = twi(((t.x % tmp) / (tmp / 2)) * (t.x % (tmp / 2)) * (512 / tmp)).y;
		} // stream this into accelerator
		///////////////////////////////////////////
		

		
		// stages
		Func hw_output;
		ComplexFunc stages, twi_stages;
		
		stages(x, y) = ComplexExpr(0.0f, 0.0f);
		stages(x, 0) = ComplexExpr(hw_input(x, 0), hw_input(x, 1));
		
		twi_stages(x, z) = ComplexExpr(hw_twi(x, 0, z), hw_twi(x, 1, z));
		
		for (int s = 1; s <= 9; s++)
		{
			int tmp = 1 << s;
			
			stages(t.x, s - 1) = stages(t.x, s - 1) * twi_stages(t.x, s - 1);
			
			stages(t.x, s) = (1 - 2 * (t.x / ((t.x / tmp) * tmp + tmp / 2))) *
						     stages(t.x, s - 1) +
						     stages((t.x + tmp / 2) % tmp + (t.x / tmp) * tmp, s - 1);
		}
		
		hw_output(x, y) = select(y == 0, stages(x, 9).x, stages(x, 9).y);
		output(x, y) = hw_output(x, y);
		////////////////////////////////////////////////////////////////////////
		
		
		
		if (get_target().has_feature(Target::Clockwork)) 
		{			  
			  Var xi, yi, xo, yo;
					  
			  output.bound(x, 0, 512);
			  output.bound(y, 0, 2);

			  hw_output.compute_root();
			  
			  hw_output
				.tile(x, y, xo, yo, xi, yi, 512, 2)
				.hw_accelerate(xi, xo);
				
				
			  // stages.unroll(x, 2);
		      
			  
			  hw_twi.stream_to_accelerator();
			  hw_input.stream_to_accelerator();
        } else 
		{
			stages.compute_root();
        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(FFT512, fft512)