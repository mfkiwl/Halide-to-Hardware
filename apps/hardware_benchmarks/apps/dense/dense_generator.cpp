#include "Halide.h"

namespace {

using namespace Halide;

class DenseLayer : public Halide::Generator<DenseLayer> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 1};

    void generate() {
        /* THE ALGORITHM */
        // relu(W*x + b)
        // W - Weights are input
        // x - input vector is static
        // b - bias vector is static

        Var x("x"), y("y");
        RDom r(0, 3);

        Func hw_input("hw_input");
        hw_input(x, y) = cast<int16_t>(input(x, y));

        // Func weights;
        // weights(x,y) = 0;
        // for (int i=0; i < 64; i++)
        // {
        //   for (int j=0; j < 64; j++)
        //   {
        //     weights(i,j) = rand();
        //   }
        // }

        Func xinput;
        xinput(x) = 0;
        xinput(0) = 0;
        xinput(1) = 1;
        xinput(2) = 2;
        // xinput(r) = r;
        // for (int i=0; i < 64; i++)
        // {
        //   xinput(i) = i;
        // }

        Func bias;
        bias(x) = 0;
        bias(0) = 0;
        bias(1) = 2;
        bias(2) = 4;
        // bias(r) = 2*r;
        // for (int i=0; i < 64; i++)
        // {
        //   bias(i) = 2*i;
        // }

        Func mult;
        mult(x) = hw_input(x,r) * xinput(r) + bias(r);

        Func relu;
        relu(x) = max( mult(x) , 0 );

        Func hw_output("hw_output");
        hw_output(x) = cast<uint8_t>(relu(x));
        output(x) = hw_output(x);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
          Var xi,yi, xo,yo;

          output.bound(x, 0, 3);
          
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

HALIDE_REGISTER_GENERATOR(DenseLayer, dense)
