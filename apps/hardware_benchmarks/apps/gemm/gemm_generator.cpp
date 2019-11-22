#include "Halide.h"

namespace {

using namespace Halide;

class Gemm : public Halide::Generator<Gemm> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
//     Input<Buffer<uint8_t>>  input_B{"input_B", 2};
//     Input<Buffer<uint8_t>>  input_C{"input_C", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */
        // C = 1000 x 1100
        // A = 1000 x 1200
        // B = 1200 x 1100
        // C = alpha*A*B + beta*C
      
        Var x("x"), y("y");

        Func f_gemm("gemm");
        RDom r(0, 4);

        Func hw_input("hw_input");
        hw_input(x, y) = cast<uint16_t>(input(x, y));
       //  hw_input_B(x, y) = cast<uint16_t>(input_B(x, y));
       //  hw_input_C(x, y) = cast<uint16_t>(input_C(x, y));

        // Expr alpha = Expr(1.5);
        // Expr beta = Expr(1.2);
        
        f_gemm(x, y) = 0;
        f_gemm(x, y) += hw_input(r, x) * hw_input(r, y);
        //f_gemm(x, y) += f_gemm(x, y) + input(x, y);

        Func hw_output("hw_output");
        hw_output(x, y) = cast<uint8_t>(f_gemm(x, y));
        output(x, y) = hw_output(x,y);

        // input.dim(0).set_bounds(0, 64);
       //  input_B.dim(0).set_bounds(0, 64);
       //  input_C.dim(0).set_bounds(0, 64);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
           Var xi,yi, xo,yo;
	   output.bound(x, 0, 4);
	   output.bound(y, 0, 4);
           hw_input.compute_root();
           //hw_input_B.compute_root();
	   //    hw_input_C.compute_root();
           hw_output.compute_root();

           hw_output.tile(x,y, xo,yo, xi,yi, 64, 64)
             .hw_accelerate(xi, xo);

	   f_gemm.compute_at(hw_output, xo).store_at(hw_output, xo);

           hw_input.stream_to_accelerator();
       //    hw_input_B.stream_to_accelerator();
       //    hw_input_C.stream_to_accelerator();

        } else {  // schedule to CPU


        }
   }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Gemm, gemm)

