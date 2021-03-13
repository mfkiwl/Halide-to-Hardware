#include "Halide.h"
#include <algorithm>
#include <stdio.h>
#include <math.h>
#include "complex.h"

namespace {

using namespace std;
using namespace Halide;
using namespace Halide::ConciseCasts;

// void my_fft(cpx* in, cpx* out, bool inverse)
// {		
	
	
	// for (int i = 0; i < n; i++)
	// {
		// int rev = 0;
		// int x = i;
		// for (int j = 0; j < stage; j++)
		// {
			// rev <<= 1;
			// rev |= (x & 1);
			// x >>= 1;
		// }
		// out[i] = in[rev];
	// }
	
	// for (int s = 1; s <= stage; s++)
	// {
		// int m = 1 << s; // 2 power s 
		// int m2 = m >> 1; // m2 = m/2 -1
		
		// cpx w(1, 0);
		// cpx wm(cosf(-PI/m2), sinf(-PI/m2));
		// if (inverse) 
		// {
			// wm.i = -wm.i;
		// }
		
		
		// for (int j = 0; j < m2; ++j) {
			// for (int k = j; k < n; k += m) {
				// cpx t = w * out[k + m2];  
				// cpx u = out[k];
				// out[k] = u + t;
				// out[k + m2] = u - t;
			// } 
			// w = w*wm; 
		// }
	// }
		
	// if (inverse)
	// {
		// for (int i = 0; i < n; i++)
		// {
			// out[i].r /= n;
			// out[i].i /= n;
		// }
	// }			
// }

class Audio : public Halide::Generator<Audio> {
	public:
		Input<Buffer<int16_t>> input1{"in1", 2};
		Input<Buffer<int16_t>> input2{"in2", 2};
		Output<Buffer<int16_t>> output{"output", 2};

    void generate() {
		Var x, y;
		Func non_rev, rev, d9, d8, d7, d6, d5, d4, d3, d2, d1, d0;
		ComplexFunc in1, out1, in2, out2;
		
		
		Func tmp;
		tmp(x, y) = cast<bfloat16_t>(-3.0f);
		
		
		in1(x) = ComplexExpr(input1(x, 0), input1(x, 1));
		
		non_rev(x) = x;
				 
		d9(x) = non_rev(x) >> 9;
		d8(x) = (non_rev(x) >> 8) & 1;
		d7(x) = (non_rev(x) >> 7) & 1;
		d6(x) = (non_rev(x) >> 6) & 1;
		d5(x) = (non_rev(x) >> 5) & 1;
		d4(x) = (non_rev(x) >> 4) & 1;
		d3(x) = (non_rev(x) >> 3) & 1;
		d2(x) = (non_rev(x) >> 2) & 1;
		d1(x) = (non_rev(x) >> 1) & 1;
		d0(x) = non_rev(x) & 1;
		
		rev(x) = (d0(x) << 9)
				 + (d1(x) << 8)
				 + (d2(x) << 7)
				 + (d3(x) << 6)
				 + (d4(x) << 5)
				 + (d5(x) << 4)
				 + (d6(x) << 3)
				 + (d7(x) << 2)
				 + (d8(x) << 1)
				 + d9(x); 
		
		out1(x) = in1(rev(x));
		
		output(x, y) = input1(x, y);
		
		
        // if (get_target().has_feature(Target::CoreIR)) {
          // Var xi,yi, xo,yo;
          
          // hw_input.compute_root();
          // hw_output.compute_root();

          // output.bound(x, 0, 64-1);
          // output.bound(y, 0, 64-1);
          
          // hw_output.tile(x,y, xo,yo, xi,yi, 64-1, 64-1)
            // .hw_accelerate(xi, xo);

          // blur.update()
            // .unroll(r.x, 2)
            // .unroll(r.y, 2);

          // blur.linebuffer();

          // hw_input.compute_at(hw_output, xi).store_at(hw_output, xo);
          // hw_input.stream_to_accelerator();
          
        // } else if (get_target().has_feature(Target::Clockwork)) {
          // Var xi,yi, xo,yo;

          // output.bound(x, 0, 64-1);
          // output.bound(y, 0, 64-1);
          
          // hw_output.compute_root();

          // hw_output.tile(x,y, xo,yo, xi,yi, 64-1, 64-1)
            // .hw_accelerate(xi, xo);
          
          // kernel.compute_at(blur, x);
          // blur.update()
            // .unroll(r.x, 2)
            // .unroll(r.y, 2);
          // brighten.compute_at(hw_output, xo);

          // hw_input.stream_to_accelerator();

        // } else {
          // kernel.compute_root();
          // blur.compute_root();
          // blur.update()
            // .unroll(r.x, 2)
            // .unroll(r.y, 2);
        //}
        
		// cout << "***************************************************************" << endl;
		
		// kernel.print_loop_nest();
		
		// cout << "***************************************************************" << endl;
		
		// blur.print_loop_nest();
		
		// cout << "***************************************************************" << endl;
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Audio, audio)
