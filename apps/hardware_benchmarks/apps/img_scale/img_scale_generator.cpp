#include "Halide.h"

namespace {

using namespace Halide;

class ImageScale : public Halide::Generator<ImageScale> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func hw_input("hw_input");
        hw_input(x, y) = cast<int16_t>(input(x, y));

        Func smin, smax, scale;
        smax(x,y) = max( hw_input(x,y) , 0 );
        scale(x,y) = 2*smax(x,y);
        smin(x,y) = min( scale(x,y) , 255 );

        Func hw_output("hw_output");
        hw_output(x, y) = cast<uint8_t>(smin(x,y));
        output(x, y) = hw_output(x,y);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
          Var xi,yi, xo,yo;

          output.bound(x, 0, 10);
          output.bound(y, 0, 10);
          
          hw_input.compute_root();
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

HALIDE_REGISTER_GENERATOR(ImageScale, img_scale)
