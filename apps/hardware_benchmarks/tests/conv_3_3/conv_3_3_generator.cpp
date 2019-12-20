#include "Halide.h"

namespace {

using namespace Halide;

class ConvolutionKernel : public Halide::Generator<ConvolutionKernel> {
public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

  int ksize = 3;
  int unrollsize = 3;
  int tilesize = 16;
  int imgsize = 62;

    void generate() {
        /* THE ALGORITHM */

        Var x("x"), y("y");

        Func kernel("kernel");
        Func conv("conv");
        RDom r(0, ksize, 0, ksize);

        kernel(x,y) = 0;
        kernel(0,0) = 1;      kernel(0,1) = 2;      kernel(0,2) = 1;
        kernel(1,0) = 2;      kernel(1,1) = 4;       kernel(1,2) = 2;
        kernel(2,0) = 1;      kernel(2,1) = 2;      kernel(2,2) = 1;
        // kernel(0,0) = 0;      kernel(0,1) = 0;      kernel(0,2) = 0;
        // kernel(1,0) = 0;      kernel(1,1) = 1;       kernel(1,2) = 0;
        // kernel(2,0) = 0;      kernel(2,1) = 0;      kernel(2,2) = 0;

        conv(x, y) = 0;

        Func hw_input("hw_input");
        hw_input(x, y) = cast<uint16_t>(input(x, y));
        conv(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);
        conv(x,y) = conv(x,y) >> 4;
        //conv(x,y) =
        //  kernel(0,0)*hw_input(x,y) +
        //  kernel(1,0)*hw_input(x+1,y) +
        //  kernel(2,0)*hw_input(x+2,y) +
        //  kernel(0,1)*hw_input(x,y+1) +
        //  kernel(1,1)*hw_input(x+1,y+1) +
        //  kernel(2,1)*hw_input(x+2,y+1) +
        //  kernel(0,2)*hw_input(x,y+2) +
        //  kernel(1,2)*hw_input(x+1,y+2) +
        //  kernel(2,2)*hw_input(x+2,y+2);

        Func hw_output("hw_output");
        hw_output(x, y) = cast<uint8_t>(conv(x, y));
        output(x, y) = hw_output(x,y);

        output.bound(x, 0, imgsize);
        output.bound(y, 0, imgsize);

        /* THE SCHEDULE */
        if (get_target().has_feature(Target::CoreIR)) {
          Var xi,yi, xo,yo;

//          Var x_host,y_host, x_gb,y_gb, x_cgra,y_cgra;
//          // Produce loop levels: host, global buffer, cgra
//          output.tile(x, y, x_host,y_host, xi,yi, 256,256);
//          output.tile(xi, yi, x_gb,y_gb, x_cgra,y_cgra, 64-2,64-2);
//
//          hw_input.store_root().compute_root();
//          hw_input.in().store_at(output, x_host).compute_at(output,x_gb);
//          hw_input.in().in().store_at(output, x_gb).compute_at(output,x_cgra);

          hw_input.compute_root();
          hw_output.compute_root();

          hw_output.tile(x,y, xo,yo, xi,yi, tilesize, tilesize)
            .hw_accelerate(xi, xo);

          conv.update()
            .unroll(r.x, unrollsize)
            .unroll(r.y, unrollsize);

          conv.linebuffer();
          //hw_input.linebuffer();

          hw_input.stream_to_accelerator();
          kernel.compute_at(hw_output, xo);

        } else {  // schedule to CPU
        //   Pipeline pipeline = get_pipeline();

        //   Var x_vi("x_vi");
        //   Var x_vo("x_vo");

        //   Func conv = pipeline.get_func(3);
        //   Func kernel = pipeline.get_func(2);
        //   Func output = pipeline.get_func(5);

        //   {
        //       Var x = conv.args()[0];
        //       Var y = conv.args()[1];
        //       RVar r$x(conv.update(0).get_schedule().rvars()[0].var);
        //       RVar r$y(conv.update(0).get_schedule().rvars()[1].var);
        //       conv
        //           .compute_root()
        //           .split(x, x_vo, x_vi, 4)
        //           .vectorize(x_vi)
        //           .parallel(y);
        //       conv.update(0)
        //           .reorder(r$x, x, r$y, y)
        //           .split(x, x_vo, x_vi, 4)
        //           .vectorize(x_vi)
        //           .parallel(y);
        //   }
        //   {
        //       Var x = kernel.args()[0];
        //       Var y = kernel.args()[1];
        //       kernel
        //           .compute_root()
        //           .parallel(y)
        //           .parallel(x);
        //   }
        //   {
        //       Var x = output.args()[0];
        //       Var y = output.args()[1];
        //       output
        //           .compute_root()
        //           .split(x, x_vo, x_vi, 16)
        //           .vectorize(x_vi)
        //           .parallel(y);
        //   }
          kernel.compute_root();
          conv.compute_root();
        }

    }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ConvolutionKernel, conv_3_3)
