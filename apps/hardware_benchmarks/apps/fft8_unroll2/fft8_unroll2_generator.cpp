#include "Halide.h"
#include "complex.h"
#include <math.h>

namespace {

using namespace std;
using namespace Halide;
using namespace Halide::ConciseCasts;

class FFT8_unroll2 : public Halide::Generator<FFT8_unroll2> {
	public:
		Input<Buffer<float>> input{"input", 2};
		Output<Buffer<float>> output{"output", 2};

    void generate() {
		Var x, y, z;
		RDom t = RDom(0, 8);
		
		
		// bit reverse
		Func hw_input, non_rev, rev, d2, d1, d0;
		
		non_rev(x) = x;
				 
		d2(x) = (non_rev(x) >> 2);
		d1(x) = (non_rev(x) >> 1) & 1;
		d0(x) = non_rev(x) & 1;
		
		rev(x) = (d0(x) << 2)
				 + (d1(x) << 1)
				 + d2(x);
				 
		hw_input(x, y) = input(rev(x), y); // stream this into accelerator
		//////////////////////////////////////////
		


		// twiddling factor
		ComplexFunc twi;
		Func hw_twi;
		
		float PI = atan(1)*4;
		twi(x) = expj(-2 * PI * x / 8);
		
		hw_twi(x, y, z) = 0.0f;
		for (int s = 1; s <= 3; s++)
		{
			int tmp = 1 << s;
			
			hw_twi(t.x, 0, s - 1) = twi(((t.x % tmp) / (tmp / 2)) * (t.x % (tmp / 2)) * (8 / tmp)).x;
			hw_twi(t.x, 1, s - 1) = twi(((t.x % tmp) / (tmp / 2)) * (t.x % (tmp / 2)) * (8 / tmp)).y;
		} // stream this into accelerator
		///////////////////////////////////////////
		

		
		// stages
		Func hw_output;
		ComplexFunc twi_stages, stage0, stage1, stage2, stage3;
		
		twi_stages(x, z) = ComplexExpr(hw_twi(x, 0, z), hw_twi(x, 1, z)); // 8 x 3
		
		
		stage0(x) = ComplexExpr(hw_input(x, 0), hw_input(x, 1));
		stage1(x) = ComplexExpr(0.0f, 0.0f);
		stage2(x) = ComplexExpr(0.0f, 0.0f);
		stage3(x) = ComplexExpr(0.0f, 0.0f);
		
		
		
		
		
		stage1(t.x) = (1 - 2 * (t.x / ((t.x / 2) * 2 + 2 / 2))) *
						     stage0(t.x) * twi_stages(t.x, 0) +
						     stage0((t.x + 2 / 2) % 2 + (t.x / 2) * 2) * twi_stages((t.x + 2 / 2) % 2 + (t.x / 2) * 2, 0);
							 
		stage2(t.x) = (1 - 2 * (t.x / ((t.x / 4) * 4 + 4 / 2))) *
						     stage1(t.x) * twi_stages(t.x, 1) +
						     stage1((t.x + 4 / 2) % 4 + (t.x / 4) * 4) * twi_stages((t.x + 4 / 2) % 4 + (t.x / 4) * 4, 1);
							 
		stage3(t.x) = (1 - 2 * (t.x / ((t.x / 8) * 8 + 8 / 2))) *
						     stage2(t.x) * twi_stages(t.x, 2) +
						     stage2((t.x + 8 / 2) % 8 + (t.x / 8) * 8) * twi_stages((t.x + 8 / 2) % 8 + (t.x / 8) * 8, 2);
		
		
		
		
		
		hw_output(x, y) = select(y == 0, stage3(x).x, stage3(x).y);
		output(x, y) = hw_output(x, y);
		////////////////////////////////////////////////////////////////////////
		
		
		
		if (get_target().has_feature(Target::Clockwork)) 
		{			  
			  Var xi, yi, xo, yo;
					  
			  output.bound(x, 0, 8);
			  output.bound(y, 0, 2);

			  hw_output.compute_root();
			  
			  hw_output
				.tile(x, y, xo, yo, xi, yi, 8, 2)
				.hw_accelerate(xi, xo);
				
				
			  stage3.unroll(x, 2);
			  stage2.unroll(x, 2);
			  stage1.unroll(x, 2);
			  
			  
			  hw_twi.stream_to_accelerator();
			  hw_input.stream_to_accelerator();
        } else 
		{
			stage3.compute_root();
			stage2.compute_root();
			stage1.compute_root();
        }
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(FFT8_unroll2, fft8_unroll2)