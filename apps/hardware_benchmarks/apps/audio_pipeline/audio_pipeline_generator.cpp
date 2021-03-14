/*
 * An application that performs a basic guitar pedal pipeline.
 * Stages of the pipeline are compression, distortion, noise
 * reduction, modulation, and reverberation.
 */

#include "Halide.h"

const float PI = 3.1415926536f;


namespace {
  using namespace Halide;
  using namespace Halide::ConciseCasts;
  using namespace std;

  class AudioPipeline : public Halide::Generator<AudioPipeline> {
    
  public:
    Input<Buffer<int16_t>>  input{"input", 2}; // second dimension is each audio channel
    Output<Buffer<int16_t>> output{"output", 2};

    void generate() {
	  float sample_freq = 44100;
	  int num_samples = 31466;



	  Var t, c;

      Func hw_input, hw_output;
	  Func normalize, scale_up;
	  
      hw_input(t, c) = input(t, c);
	  
	  normalize(t, c) = cast<float>(hw_input(t, c)) / 32767.0f;
	  scale_up(t, c) = cast<int16_t>(clamp(10.0f * normalize(t, c), -1.0f, 1.0f) * 32767.0f);

	  hw_output(t, c) = scale_up(t, c);
	  output(t, c) = hw_output(t, c);


	  if (get_target().has_feature(Target::Clockwork)) {
		  
		Var ti, to;  
		  
        output.bound(t, 0, num_samples);
        output.bound(c, 0, 2);

        hw_output.compute_root();

        hw_output.split(t, to, ti, num_samples)
          .reorder(ti, c, to)
          .hw_accelerate(ti, to);


        scale_up.compute_at(hw_output, to);
        normalize.compute_at(hw_output, to);


        hw_input.stream_to_accelerator();
          
      } else 
	  {    
			scale_up.compute_root();
			normalize.compute_root();
      }
    }
	
	
  };

}

HALIDE_REGISTER_GENERATOR(AudioPipeline, audio_pipeline)