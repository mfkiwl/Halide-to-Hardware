#include <cstdio>
#include "hardware_process_helper.h"
#include "hardware_image_helpers.h"
#include "halide_image_io.h"

#if defined(WITH_CPU)
   #include "audio_pipeline.h"
#endif

#if defined(WITH_CLOCKWORK)
    #include "rdai_api.h"
    #include "clockwork_sim_platform.h"
    #include "audio_pipeline_clockwork.h"
#endif

using namespace Halide::Tools;
using namespace Halide::Runtime;
using namespace std;

int main( int argc, char **argv ) {
  std::map<std::string, std::function<void()>> functions;
  OneInOneOut_ProcessController<int16_t> processor("audio_pipeline");
  //OneInOneOut_ProcessController<int16_t> processor("audio_pipeline");

  #if defined(WITH_CPU)
      auto cpu_process = [&]( auto &proc ) {
        audio_pipeline( proc.input, proc.output );
      };
      functions["cpu"] = [&](){ cpu_process( processor ); } ;
  #endif
  
  #if defined(WITH_CLOCKWORK)
      auto clockwork_process = [&]( auto &proc ) {
        RDAI_Platform *rdai_platform = RDAI_register_platform( &rdai_clockwork_sim_ops );
        if ( rdai_platform ) {
          printf( "[RUN_INFO] found an RDAI platform\n" );
          audio_pipeline_clockwork( proc.input, proc.output );
          RDAI_unregister_platform( rdai_platform );
        } else {
          printf("[RUN_INFO] failed to register RDAI platform!\n");
        }
      };
      functions["clockwork"] = [&](){ clockwork_process( processor ); };
  #endif

  // Add all defined functions
  processor.run_calls = functions;
  
  
  AudioFile<int16_t> f1;
  readAudioData<int16_t>("third.wav", &f1);
  
  cout << "*********************************************************" << endl;
  cout << f1.header << endl;
  
  

  processor.input   = f1.data;
  auto in = processor.input;
  processor.output  = Buffer<int16_t>(in.dim(0).extent(), in.dim(1).extent());
  processor.inputs_preset = true;
  
  
  cout << in.dim(0).extent() << "   " << in.dim(1).extent() << endl;
  cout << "*********************************************************" << endl;
  
  

  auto ret_value = processor.process_command(argc, argv);


  AudioFile<int16_t> outAudio;
  outAudio.header = f1.header;
  outAudio.data = processor.output;
  writeAudioData<int16_t>("bin/output.wav", outAudio);
  
  auto out = processor.output;
  
  
  
  // for (int i = 0; i < 40; i++)
  // {
	  // cout << in(i, 0) << "  " << in(i, 1) << endl;
  // }
  
  // cout << endl << endl << endl;
  
  // for (int i = 0; i < 40; i++)
  // {
	  // cout << out(i, 0) << "  " << out(i, 1) << endl;
  // }
  
  return ret_value;
}