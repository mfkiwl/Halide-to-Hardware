#include "Halide.h"

namespace {

using namespace Halide;

class Covariance : public Halide::Generator<Covariance> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    void generate() {
        /* THE ALGORITHM */
        // M = 1200
        // N = 1400 
        // input (data) - N x M
        // output (covar) - M x M 

        Var x("x"), y("y");

        Func hw_input("hw_input");
        hw_input(x, y) = cast<uint16_t>(input(x, y));

        Func mean("mean");
        RDom r(0, 32);
        mean(x) = 0;
        mean(x) += hw_input(x,r);
        mean(x) /= 32;

        Func normalize("normalize");
        normalize(x, y) = hw_input(x, y) - mean(x);

        Func cov("cov");
        cov(x,y) = 0;
        cov(x,y) += normalize(r,x) * normalize(r,y);
        cov(x,y) /= (32-1);

        Func hw_output("hw_output");
        hw_output(x,y) = cast<uint8_t>(cov(x,y));
        output(x,y) = hw_output(x,y);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
          Var xi,yi, xo,yo;
          
          hw_input.compute_root();
          hw_output.compute_root();

          // output.bound(x, 0, 32);
          // output.bound(y, 0, 32);
          
          hw_output.tile(x,y, xo,yo, xi,yi, 32, 32)
            .hw_accelerate(xi, xo);

          hw_input.stream_to_accelerator();
          
        } else {  // schedule to CPU
          output.compute_root();
        }

    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Covariance, covar)
