#include <vector>
#include <stdint.h>
#include <cassert>
#pragma once
namespace turbo {
	template<class T>
	using vector_t = std::vector<T>;

	using dec_prob_t = float;
	using dec_arr_t = vector_t<dec_prob_t>;

	using byte_t = uint8_t;
	using byte_arr_t = vector_t<byte_t>;
}