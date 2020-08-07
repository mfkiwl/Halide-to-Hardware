#include "Halide.h"

namespace {

using namespace Halide;

class ConvolutionKernel : public Halide::Generator<ConvolutionKernel> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};
    int ksize = 3;
    int imgsize = 62;
    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func kernel("kernel"), fp_kernel("fp_kernel");
        Func conv("conv");
        RDom r(0, 3,
               0, 3);

        kernel(x,y) = bfloat16_t(0);
        kernel(0,0) = bfloat16_t(1.1);      kernel(0,1) = bfloat16_t(1.2);     kernel(0,2) = bfloat16_t(1.3);
        kernel(1,0) = bfloat16_t(2.4);      kernel(1,1) = bfloat16_t(0);       kernel(1,2) = bfloat16_t(2.6);
        kernel(2,0) = bfloat16_t(3.7);      kernel(2,1) = bfloat16_t(3.8);     kernel(2,2) = bfloat16_t(3.9);
        fp_kernel(x) = cast<bfloat16_t>(3*x);

        conv(x, y) = cast<bfloat16_t>(0);

        Func hw_input("hw_input"), hw_input_bfloat;
        hw_input(x, y) = cast<uint16_t>(input(x, y));
        hw_input_bfloat(x, y) = cast<bfloat16_t>(hw_input(x, y));
        conv(x, y)  += fp_kernel(r.x + 3* r.y) * hw_input_bfloat(x + r.x, y + r.y);

        Func hw_output("hw_output");
        hw_output(x, y) = cast<float>(conv(x, y));
        output(x, y) = cast<uint8_t>(ceil(hw_output(x,y)));

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {

        } else if (get_target().has_feature(Target::Clockwork)) {
          Var xi,yi, xo,yo;

          output.bound(x, 0, imgsize);
          output.bound(y, 0, imgsize);

          hw_output.compute_root();

          hw_output
              .tile(x,y, xo,yo, xi,yi, imgsize, imgsize)
              .hw_accelerate(xi, xo);

          conv.compute_at(hw_output, xo);

          hw_input_bfloat.compute_at(hw_output, xo);
          hw_input.compute_root();
          hw_input.stream_to_accelerator();
            
        } else {  // schedule to CPU
          kernel.compute_root();
          conv.compute_root();
          conv.update()
            .unroll(r.x, 3)
            .unroll(r.y, 3);
        }

    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ConvolutionKernel, fp_conv_3_3)
