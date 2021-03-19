#include "Halide.h"

namespace {

using namespace Halide;

class UnitTestSminmax : public Halide::Generator<UnitTestSminmax> {
public:
    Input<Buffer<int8_t>>  input{"input", 2};
    Output<Buffer<int8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func hw_input("hw_input");
        hw_input(x, y) = cast<int16_t>(input(x, y));

        Func smin, smax, sclamp;
        smin(x,y) = min( hw_input(x,y) , -30 );
        smax(x,y) = max( hw_input(x,y) , 64 );
        sclamp(x,y) = clamp( hw_input(x,y), 1, 150 );

        Func hw_output("hw_output");
        hw_output(x, y) = smin(x,y) + smax(x,y) - sclamp(x,y);
        output(x, y) = cast<int8_t>(hw_output(x,y));

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
        } else if (get_target().has_feature(Target::Clockwork)) {
          Var xi,yi, xo,yo;

          output.bound(x, 0, 64);
          output.bound(y, 0, 64);
          
          hw_output.compute_root();
          
          hw_output.tile(x,y, xo,yo, xi,yi, 64, 64)
            .hw_accelerate(xi, xo);

          hw_input.stream_to_accelerator();
          
        } else {  // schedule to CPU
          output.compute_root();
        }
        
    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(UnitTestSminmax, sminmax)
