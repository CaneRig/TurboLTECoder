#include <iostream>
#include "../Turbo/Coder.hpp"
#include <random>
#include <iostream>
#include <memory>
#pragma once

// test decoder correctiveness
/**
*	Decoder_	- decoder type
* 
*	@blsize		- size of a block
*	@verbose	- if true, outputs to console errors
*	@snr		- signal-noise ratio
*	@number		- number of tests
*	@seed		- seed of randomnizator
*	@return		- average BER (bit error rate)
**/
template<class Decoder_>
double TestValidation(int blsize, bool verbose = true, float snr = 0, int number = 100, int seed = 0) {
	std::mt19937 random(seed);
	std::mt19937 gauss(random());

	int badBlocks = 0;

	double avg_ber = 0;


	if (verbose) {
		std::cout << "TestValidation: "
			<< "\n \tBlock-size: " << blsize
			<< "\n \tSNR:        " << snr
			<< "\n \tTest amount:" << number
			<< "\n \tSeed:       " << seed << std::endl;
	}


	std::uniform_int_distribution<int> dist(0, 1);
	std::normal_distribution<float> noise(0, sqrt(pow(10, -snr / 10)));

	auto decoder = std::make_unique<Decoder_>();

	for (int i = 0; i < number; i++)
	{
		double ber = 0;
		turbo::byte_arr_t input, coded, decout;
		input.resize(blsize);

		for (auto& _ : input)
			_ = dist(random);

		if (!turbo::Encode(input, coded)) {
			std::cout << "Code error" << std::endl;
			continue;
		}

		turbo::dec_arr_t decin;

		std::transform(coded.begin(), coded.end(), std::back_inserter(decin), [](int x) {
			return ((!x) ? -1 : 1) * 1;
			});
		std::transform(decin.begin(), decin.end(), decin.begin(), [&](float x) {
			return (x + noise(gauss));
			});

		auto  _saturate8 = [](int val) -> int8_t {
			val = (val < -127) ? -127 : val;
			return  (val > 127) ? 127 : val;
			};

		std::transform(decin.begin(), decin.end(), decin.begin(), [&](float x) {
			return _saturate8(int(x * 4));
			});

		if (!decoder->Decode(decin, 6, decout)) {
			if (verbose)
				std::cout << "Code error" << std::endl;
			continue;
		}


		if (input != decout)
		{
			if (verbose) {
				for (auto _ : input) std::cout << (int)_;
				std::cout << '\n';
				for (int k = 0; k < blsize; k++) std::cout << ((input[k] != decout[k]) ? '#' : ' ');
				std::cout << '\n';
				for (auto _ : decout) std::cout << (int)_;
				std::cout << '\n';

				std::cout << "\a[ERRR] (press any key and enter)" << std::endl;

				char __;
				std::cin >> __;
			}
			for (int k = 0; k < blsize; k++)
				if (input[k] != decout[k])
					ber++;
			ber /= input.size();
			if (verbose) std::cout << "BER: " << ber << std::endl;


			avg_ber += ber;
			badBlocks++;
		}
		else {
			if (verbose) std::cout << "[PASS]" << std::endl;
		}
		if (verbose) std::cout << std::endl;
	}

	if (verbose)
		std::cout << "Test done errors " << badBlocks << " of " << number << "; avg ber " << avg_ber / number << std::endl;

	return avg_ber / number;
}