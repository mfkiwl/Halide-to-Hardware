#include <iostream>
#include <math.h>
#include <cstdio>
#include "hardware_process_helper.h"
#include "halide_image_io.h"

#if defined(WITH_CPU)
   #include "resnet_layer_gen.h"
#endif

#if defined(WITH_COREIR)
    #include "coreir_interpret.h"
#endif

#if defined(WITH_CLOCKWORK)
    #include "rdai_api.h"
    #include "clockwork_sim_platform.h"
    #include "resnet_layer_gen_clockwork.h"
#endif

using namespace std;
using namespace Halide::Tools;
using namespace Halide::Runtime;

int main( int argc, char **argv ) {
  std::map<std::string, std::function<void()>> functions;
  ManyInOneOut_ProcessController<int16_t> processor("resnet_layer_gen", {"input.png", "kernel.png"});

  #if defined(WITH_CPU)
      auto cpu_process = [&]( auto &proc ) {
        resnet_layer_gen(proc.inputs["input.png"], proc.inputs["kernel.png"], proc.output);
      };
      functions["cpu"] = [&](){ cpu_process( processor ); } ;
  #endif
  
  #if defined(WITH_COREIR)
      auto coreir_process = [&]( auto &proc ) {
          run_coreir_on_interpreter<>( "bin/design_top.json",
                                       proc.inputs["input.png"], proc.output,
                                       "self.in_arg_0_0_0", "self.out_0_0" );
      };
      functions["coreir"] = [&](){ coreir_process( processor ); };
  #endif
  
  #if defined(WITH_CLOCKWORK)
      auto clockwork_process = [&]( auto &proc ) {
        RDAI_Platform *rdai_platform = RDAI_register_platform( &rdai_clockwork_sim_ops );
        if ( rdai_platform ) {
          printf( "[RUN_INFO] found an RDAI platform\n" );
          resnet_layer_gen_clockwork(proc.inputs["input.png"], proc.inputs["kernel.png"], proc.output);
          RDAI_unregister_platform( rdai_platform );
        } else {
          printf("[RUN_INFO] failed to register RDAI platform!\n");
        }
      };
      functions["clockwork"] = [&](){ clockwork_process( processor ); };
  #endif


    processor.run_calls = functions;

    ///// INPUT IMAGE /////
    processor.inputs["input.png"] = Buffer<int16_t>(240, 30, 3);
    auto input_copy_stencil = processor.inputs["input.png"];

    for (int y = 0; y < input_copy_stencil.dim(2).extent(); y++) {
      for (int x = 0; x < input_copy_stencil.dim(1).extent(); x++) {
        for (int z = 0; z < input_copy_stencil.dim(0).extent(); z++) {
            input_copy_stencil(z, x, y) = 99999999;
    } } }
	

    ///// KERNEL WEIGHTS /////  
    processor.inputs["kernel.png"] = Buffer<int16_t>(240, 30, 3);
    auto kernel_copy_stencil = processor.inputs["kernel.png"];
    for (int y = 0; y < kernel_copy_stencil.dim(2).extent(); y++) {
      for (int x = 0; x < kernel_copy_stencil.dim(1).extent(); x++) {
        for (int w = 0; w < kernel_copy_stencil.dim(0).extent(); w++) {
              kernel_copy_stencil(w, x, y) = 3;
    } } }
	
	cout << kernel_copy_stencil.dim(2).extent() << endl;
	
    processor.inputs_preset = true;
    processor.output = Buffer<int16_t>(240, 30, 3);

    auto output = processor.process_command(argc, argv);
    
    return output;
}  