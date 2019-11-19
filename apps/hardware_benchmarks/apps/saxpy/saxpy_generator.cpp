#include "Halide.h"

namespace {

using namespace Halide;

class SinglePrecisionAXPlusY : public Halide::Generator<SinglePrecisionAXPlusY> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func hw_input("hw_input");
        hw_input(x, y) = cast<int16_t>(input(x, y));

        Func mult, div, add, sub, mod;
        mult(x,y) = hw_input(x,y) * 13;
        add(x,y) = mult(x,y) + hw_input(x,y);

        Func hw_output("hw_output");
        hw_output(x, y) = cast<uint8_t>(add(x, y));
        output(x, y) = hw_output(x,y);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
          Var xi,yi, xo,yo;
          
          hw_input.compute_root();
          hw_output.compute_root();

          output.bound(x, 0, 22);
          output.bound(y, 0, 22);
          
          hw_output.tile(x,y, xo,yo, xi,yi, 64, 64)
            .hw_accelerate(xi, xo);

          hw_input.stream_to_accelerator();
          
        } else {  // schedule to CPU
          output.compute_root();
        }
        
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(SinglePrecisionAXPlusY, saxpy)
