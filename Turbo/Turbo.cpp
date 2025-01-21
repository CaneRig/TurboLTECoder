#define NDEBUG
#include "Tests/DecoderValidation.h"
#include "Tests/SpeedTest.hpp"
#include <iostream>
#include "Turbo/Decoder.hpp"
using namespace std;

#define TEST_SPEED 1
#define TEST_VALID 2

#define TEST_MODE TEST_SPEED

float snr = 2;

int main() {
#if TEST_MODE == TEST_SPEED
	int thread_count;
	cout << "Thread count: ";
	cin >> thread_count;
	double count_multiplier = 1;
	cout << "Block number per thread multiplier: ";
	cin >> count_multiplier;
#endif
	int block_size;
	cout << "Block size: ";
	cin >> block_size;

	if (block_size > 6144 || block_size < 0) {
		throw std::domain_error("Block size out of range");
	}

	// array of sizes that are going to be tested
	int block_sizes[] = { block_size };
	
	for (auto sz : block_sizes) {
		cout << "\n\nBlock size: " << sz << endl;

#if TEST_MODE == TEST_SPEED
		TestSpeed<turbo::Decoder>(sz, 100000000/sz*count_multiplier, thread_count, 0);
#elif TEST_MODE == TEST_VALID
		TestValidation<turbo::Decoder>(sz, true, 1);
#else
		#error Invalid TEST_MODE
#endif
	}
}
