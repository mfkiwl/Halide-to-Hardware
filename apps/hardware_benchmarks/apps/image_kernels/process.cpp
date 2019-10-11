#include <cstdio>

#include "image_kernels.h"

#include "hardware_process_helper.h"
#include "coreir_interpret.h"
#include "halide_image_io.h"

using namespace Halide::Tools;
using namespace Halide::Runtime;

int main(int argc, char **argv) {

  OneInOneOut_ProcessController<uint8_t> processor("image_kernels",
                                            {
                                              {"cpu",
                                                  [&]() { image_kernels(processor.input, processor.output); }
                                              },
                                              {"coreir",
                                                  [&]() { run_coreir_on_interpreter<>("bin/design_top.json", processor.input, processor.output,
                                                                                      "self.in_arg_0_0_0", "self.out_0_0"); }
                                              }

                                            });

  processor.input = Buffer<uint8_t>(768, 512);
  processor.output = Buffer<uint8_t>(768-10, 512-10);
  
  processor.process_command(argc, argv);
  
}
