/*
 * An application that performs a basic camera pipeline.
 * Stages of the pipeline are hot pixel suppression, demosaicking,
 * color corrections, and applying a camera curve.
 */

#include "Halide.h"

namespace {

  using namespace Halide;
  Var x("x"), y("y"), c("c"), xo("xo"), yo("yo"), xi("xi"), yi("yi");


  class ImageKernels : public Halide::Generator<ImageKernels> {
    
  public:
    Input<Buffer<uint8_t>>  input{"input", 2};
    Output<Buffer<uint8_t>> output{"output", 2};

    Func sharpen(Func input) {
      Func kernel("kernel");
      Func conv("conv");
      RDom r(0, 3,
             0, 3);

      kernel(x,y) = 0;
      kernel(0,0) = 0;       kernel(0,1) = -1;      kernel(0,2) = 0;
      kernel(1,0) = -1;      kernel(1,1) = 5;       kernel(1,2) = -1;
      kernel(2,0) = 0;       kernel(2,1) = -1;      kernel(2,2) = 0;

      conv(x, y) = 0;

      Func hw_input("hw_input");
      hw_input(x, y) = cast<uint16_t>(input(x, y));
      conv(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);

      Func sharpened("sharpened");
      sharpened(x, y) = cast<uint8_t>(conv(x,y) % 256);
      return sharpened;
    }

    Func gaussian(Func input) {
      Func kernel("kernel");
      Func conv("conv");
      RDom r(0, 3,
             0, 3);

      kernel(x,y) = 0;
      kernel(0,0) = 1;      kernel(0,1) = 2;     kernel(0,2) = 1;
      kernel(1,0) = 2;      kernel(1,1) = 4;     kernel(1,2) = 2;
      kernel(2,0) = 1;      kernel(2,1) = 2;     kernel(2,2) = 1;

      conv(x, y) = 0;

      Func hw_input("hw_input"), averaged("averaged");
      hw_input(x, y) = cast<uint16_t>(input(x, y));
      conv(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);
      averaged(x, y) = conv(x, y) >> 4;

      Func gaussianed("blurred");
      gaussianed(x, y) = cast<uint8_t>(averaged(x,y) % 256);
      return gaussianed;
    }

    Func motion_blur(Func input) {
      Func kernel("kernel");
      Func conv("conv");
      RDom r(0, 3,
             0, 3);

      kernel(x,y) = 0;
      kernel(0,0) = 1;      kernel(0,1) = 1;     kernel(0,2) = 1;
      kernel(1,0) = 1;      kernel(1,1) = 1;     kernel(1,2) = 1;
      kernel(2,0) = 1;      kernel(2,1) = 1;     kernel(2,2) = 1;

      conv(x, y) = 0;

      Func hw_input("hw_input"), averaged("averaged");
      hw_input(x, y) = cast<uint16_t>(input(x, y));
      conv(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);
      averaged(x, y) = conv(x, y) / 9;

      Func motion_blurred("blurred");
      motion_blurred(x, y) = cast<uint8_t>(averaged(x,y) % 256);
      return motion_blurred;
    }

    Func emboss(Func input) {
      Func kernel("kernel");
      Func conv("conv");
      RDom r(0, 3,
             0, 3);

      kernel(x,y) = 0;
      kernel(0,0) = -2;      kernel(0,1) = -1;     kernel(0,2) = 0;
      kernel(1,0) = -1;      kernel(1,1) = 1;      kernel(1,2) = 1;
      kernel(2,0) = 0;       kernel(2,1) = 1;      kernel(2,2) = 2;

      conv(x, y) = 0;

      Func hw_input("hw_input");
      hw_input(x, y) = cast<uint16_t>(input(x, y));
      conv(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);

      Func embossed("embossed");
      embossed(x, y) = cast<uint8_t>(conv(x,y) % 256);
      return embossed;
    }

    Func outline(Func input) {
      Func kernel("kernel");
      Func conv("conv");
      RDom r(0, 3,
             0, 3);

      kernel(x,y) = 0;
      kernel(0,0) = -1;      kernel(0,1) = -1;     kernel(0,2) = -1;
      kernel(1,0) = -1;      kernel(1,1) = 8;      kernel(1,2) = -1;
      kernel(2,0) = -1;      kernel(2,1) = -1;     kernel(2,2) = -1;

      conv(x, y) = 0;

      Func hw_input("hw_input");
      hw_input(x, y) = cast<uint16_t>(input(x, y));
      conv(x, y)  += kernel(r.x, r.y) * hw_input(x + r.x, y + r.y);

      Func outlined("outlined");
      outlined(x, y) = cast<uint8_t>(conv(x,y) % 256);
      return outlined;
    }

    
    void generate() {

      Func hw_input;
      hw_input(x,y) = input(x,y);

      Func stage_1;
      stage_1 = gaussian(hw_input);

      Func stage_2;
      stage_2 = sharpen(stage_1);

      Func stage_3;
      stage_3 = motion_blur(stage_2);

      Func stage_4;
      stage_4 = emboss(stage_3);

      Func stage_5;
      stage_5 = outline(stage_4);
        
      output = stage_5;
      //output = outline(stage_3);
        
      /* THE SCHEDULE */
      if (get_target().has_feature(Target::CoreIR)) {
        /*hw_input.compute_root();
        hw_output.compute_root();
          
        hw_output.tile(x, y, xo, yo, xi, yi, 64-6,64-6);
          
        stage_1.linebuffer()
          .unroll(x).unroll(y);
        stage_2.linebuffer()
          .unroll(c).unroll(x).unroll(y);

        stage_3.compute_at(hw_output, xo).unroll(x);  // synthesize curve to a ROM

        hw_output.accelerate({hw_input}, xi, xo, {});
        //hw_output.unroll(c).unroll(xi, 2);
        hw_output.unroll(c);*/
          
      } else {    // schedule to CPU
        output.tile(x, y, xo, yo, xi, yi, 64-6, 64-6)
          .compute_root();

        output.fuse(xo, yo, xo).parallel(xo).vectorize(xi, 4);
      }
    }
  };

}  // namespace

HALIDE_REGISTER_GENERATOR(ImageKernels, image_kernels)

