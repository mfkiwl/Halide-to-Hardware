#include <cmath>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <cstdlib>
#include <chrono>

#include "HalideBuffer.h"

#include "fft_forward_r2c.h"
#include "fft_inverse_c2r.h"
#include "fft_forward_c2c.h"
#include "fft_inverse_c2c.h"

namespace {
const float kPi = 3.14159265358979310000f;

const int32_t kSize = 16;
}

using namespace std;
using Halide::Runtime::Buffer;

Buffer<float, 3> real_buffer(int32_t y_size = kSize) {
    return Buffer<float, 3>::make_interleaved(kSize, y_size, 1);
}

Buffer<float, 3> complex_buffer(int32_t y_size = kSize) {
    return Buffer<float, 3>::make_interleaved(kSize, y_size, 2);
}

float &re(Buffer<float, 3> &b, int x, int y) {
    return b(x, y, 0);
}

float &im(Buffer<float, 3> &b, int x, int y) {
    return b(x, y, 1);
}

float re(const Buffer<float, 3> &b, int x, int y) {
    return b(x, y, 0);
}

float im(const Buffer<float, 3> &b, int x, int y) {
    return b(x, y, 1);
}

int main(int argc, char **argv) {
    std::cout << std::fixed << std::setprecision(2);

	std::cout << "Forward complex to complex test." << std::endl;

	auto in = complex_buffer();

	float signal_1d_real[kSize];
	float signal_1d_complex[kSize];
	for (size_t i = 0; i < kSize; i++) {
		signal_1d_real[i] = 0;
		signal_1d_complex[i] = 0;
		for (size_t k = 1; k < 5; k++) {
			signal_1d_real[i] += cos(2 * kPi * (k * (i / (float)kSize) + (k / 16.0f)));
			signal_1d_complex[i] += sin(2 * kPi * (k * (i / (float)kSize) + (k / 16.0f)));
		}
	}

	for (int j = 0; j < kSize; j++) {
		for (int i = 0; i < kSize; i++) {
			re(in, i, j) = signal_1d_real[i] + signal_1d_real[j];
			im(in, i, j) = signal_1d_complex[i] + signal_1d_complex[j];
		}
	}

	auto out = complex_buffer();

	int halide_result;
	
	auto start = chrono::steady_clock::now();
	halide_result = fft_forward_c2c(in, out);
	auto end = chrono::steady_clock::now();
	
	
	if (halide_result != 0) {
		std::cerr << "fft_forward_c2c failed returning " << halide_result << std::endl;
		exit(1);
	}

	for (size_t i = 1; i < 5; i++) {
		// Check horizontal bins
		float real = re(out, i, 0);
		float imaginary = im(out, i, 0);
		float magnitude = sqrt(real * real + imaginary * imaginary);
		if (fabs(magnitude - 1.0f) > .001) {
			std::cerr << "fft_forward_c2c bad magnitude for horizontal bin " << i << ":" << magnitude << std::endl;
			exit(1);
		}
		float phase_angle = atan2(imaginary, real);
		if (fabs(phase_angle - (i / 16.0f) * 2 * kPi) > .001) {
			std::cerr << "fft_forward_c2c bad phase angle for horizontal bin " << i << ": " << phase_angle << std::endl;
			exit(1);
		}
		// Check vertical bins
		real = re(out, 0, i);
		imaginary = im(out, 0, i);
		magnitude = sqrt(real * real + imaginary * imaginary);
		if (fabs(magnitude - 1.0f) > .001) {
			std::cerr << "fft_forward_c2c bad magnitude for vertical bin " << i << ":" << magnitude << std::endl;
			exit(1);
		}
		phase_angle = atan2(imaginary, real);
		if (fabs(phase_angle - (i / 16.0f) * 2 * kPi) > .001) {
			std::cerr << "fft_forward_c2c bad phase angle for vertical bin " << i << ": " << phase_angle << std::endl;
			exit(1);
		}
	}

	// Check all other components are close to zero.
	for (size_t j = 0; j < kSize; j++) {
		for (size_t i = 0; i < kSize; i++) {
			// The first four non-DC bins in x and y have non-zero
			// values. The input is chose so the mirrored negative
			// frequency components are all zero due to
			// interference of the real and complex parts.
		  if (!((j == 0 && (i > 0 && i < 5)) ||
				(i == 0 && j > 0 && j < 5))) {
				float real = re(out, i, j);
				float imaginary = im(out, i, j);
				if (fabs(real) > .001) {
					std::cerr << "fft_forward_c2c real component at (" << i << ", " << j << ") is non-zero: " << real << std::endl;
					exit(1);
				}
				if (fabs(imaginary) > .001) {
					std::cerr << "fft_forward_c2c imaginary component at (" << i << ", " << j << ") is non-zero: " << imaginary << std::endl;
					exit(1);
				}
			}
		}
	}
	cout << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms" << endl;
	
	return 0;
}
