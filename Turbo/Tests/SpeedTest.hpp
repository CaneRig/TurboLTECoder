#include "../Turbo/Coder.hpp"

#include "../Utility/SysThreadUtility.h"

#include "../Utility/Timer.hpp"
#include <iostream>
#include <random>
#include <thread>

#include <string>
#include <atomic>
#pragma once

using namespace std;
atomic_int _thidss;


// lightweught randomizer
inline void _turbo_fast_randomize(turbo::byte_arr_t& arr, std::mt19937& rnd) {
	unsigned int a = rnd();
	for (auto& _ : arr)
		_ = !((a += 123239871) & 0b100);
}

// loop that decodes 
template<class Decoder_>
void _decoding_loop(size_t blocks_number, int block_size, const int iterations) {
	std::unique_ptr<Decoder_> decoder = std::make_unique<Decoder_>();

	std::mt19937 rd;
	turbo::byte_arr_t inp;
	turbo::byte_arr_t einp;
	inp.resize(block_size);
	einp.resize(block_size * 3 + 12);

	turbo::dec_arr_t out(block_size * 3 + 12);

	for (size_t i = 0; i < blocks_number; i++)
	{
		_turbo_fast_randomize(inp, rd);

		turbo::Encode(inp, einp);

		for (int j = 0; j < einp.size(); j++)
			out[j] = (!einp[j]) ? -1 : 1;
		
		decoder->Decode(out, iterations, inp);
	}	
}

// runs decoder speed test initialization
template<class Decoder_>
void _decoder_speed_body(int thread_count, size_t number, size_t bllen, const int its, bool use_priority) {
	auto worker = _decoding_loop<Decoder_>;

#if __linux__
	std::cout << "Warning!: To use thread priority boost, program has to be executed as root" << std::endl;
#endif

	if (!interlv::Valid(interlv::Get(bllen)))
	{
		std::cerr << "Error!: Invalid block size: " << bllen << std::endl;
		return;
	}

	_thidss.store(0);

	std::vector<std::thread> threads;

	if (thread_count < 1)
		thread_count = std::thread::hardware_concurrency();

	size_t number1 = number;

		std::cout << "TestSpeed: "
			<< "\n \tBlock-size:   " << bllen
			<< "\n \tBlocks/thread:" << number
			<< "\n \tDecoder iter.:" << its
			<< "\n \tThread amount:" << thread_count
			<< "\n \tPrioritize   :" << use_priority << std::endl;

	number *= thread_count;

	for (size_t i = 0; i < thread_count; i++) {
		threads.push_back(std::thread(worker, number1, bllen, its));
		if (use_priority)
			SysSetHighestPriority(threads[i].native_handle());
	}
	for (size_t i = 0; i < thread_count; i++)
		threads[i].join();
}

// test decoder's speed
/**
*	Decoder_		- decoder type
*
*	@block_size					- size of a block
*	@block_number_per_thread	- number of blocks that are decoded per thread
*	@thread_count				- number of running threads
*	@use_priority				- if true, thread is prioritized to avoid thread yielding
*	@iterations					- decoding iterations
**/
template<class Decoder_>
void TestSpeed(int block_size, size_t block_number_per_thread, int thread_count, bool use_priority, int iterations = 6) {
	FunctionTimer<void, int, size_t, size_t, int> timer(std::string("Worker"), _decoder_speed_body<Decoder_>);

	timer.Invoke(thread_count, block_number_per_thread, block_size, iterations, use_priority);

	cout << "Total number of blocks: " << block_number_per_thread * thread_count << endl;
	cout << timer << endl;

	size_t total_size = block_number_per_thread * block_size * thread_count;

	string v_ = to_string(total_size / timer.elapsed() / 1e6);
	if (v_.find('.') != std::string::npos)
		v_[v_.find('.')] = ',';
	cout << v_ << "Mbit/s for all\n";
	cout << total_size / timer.elapsed() / 1e6 / thread_count << "Mbit/s per thread\n";
	cout << block_number_per_thread * thread_count / timer.elapsed() << "its/s" << endl;
	cout << timer.elapsed() / block_number_per_thread / thread_count << "s/it" << endl;
	cout << block_number_per_thread * thread_count << "its" << endl;

}