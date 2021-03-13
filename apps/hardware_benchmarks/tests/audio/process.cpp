#include <cstdio>
#include "hardware_process_helper.h"
#include "halide_image_io.h"

#if defined(WITH_CPU)
   #include "audio.h"
#endif

// #if defined(WITH_COREIR)
    // #include "coreir_interpret.h"
// #endif

// #if defined(WITH_CLOCKWORK)
    // #include "rdai_api.h"
    // #include "clockwork_sim_platform.h"
    // #include "brighten_and_blur_clockwork.h"
// #endif

using namespace std;
using namespace Halide::Tools;
using namespace Halide::Runtime;

int main( int argc, char **argv ) {
  std::map<std::string, std::function<void()>> functions;
  ManyInOneOut_ProcessController<int16_t> processor("audio", {"in1.png", "in2.png"});

  #if defined(WITH_CPU)
      auto cpu_process = [&]( auto &proc ) {
        audio(proc.inputs["in1.png"], proc.inputs["in2.png"], proc.output);
      };
      functions["cpu"] = [&](){ cpu_process( processor ); } ;
  #endif
  
  // #if defined(WITH_COREIR)
      // auto coreir_process = [&]( auto &proc ) {
          // run_coreir_on_interpreter<>( "bin/design_top.json",
                                       // proc.input, proc.output,
                                       // "self.in_arg_0_0_0", "self.out_0_0" );
      // };
      // functions["coreir"] = [&](){ coreir_process( processor ); };
  // #endif
  
  // #if defined(WITH_CLOCKWORK)
      // auto clockwork_process = [&]( auto &proc ) {
        // RDAI_Platform *rdai_platform = RDAI_register_platform( &rdai_clockwork_sim_ops );
        // if ( rdai_platform ) {
          // printf( "[RUN_INFO] found an RDAI platform\n" );
          // brighten_and_blur_clockwork( proc.input, proc.output );
          // RDAI_unregister_platform( rdai_platform );
        // } else {
          // printf("[RUN_INFO] failed to register RDAI platform!\n");
        // }
      // };
      // functions["clockwork"] = [&](){ clockwork_process( processor ); };
  // #endif

  // Add all defined functions
  processor.run_calls = functions;
  processor.inputs["in1.png"] = Buffer<int16_t>(1024, 2);
  processor.inputs["in2.png"] = Buffer<int16_t>(1024, 2);
  
  auto in1 = processor.inputs["in1.png"];
  auto in2 = processor.inputs["in2.png"];

  for (int i = 0; i < 1024; i++)
  {
	  in1(i, 0) = -22;
	  in1(i, 1) = -10;
	  
	  in2(i, 0) = -10;
	  in2(i, 1) = -7;
  }
  
  processor.inputs_preset = true;
  processor.output = Buffer<int16_t>(1024, 2);
  auto out = processor.process_command(argc, argv);
  
  return out;
}