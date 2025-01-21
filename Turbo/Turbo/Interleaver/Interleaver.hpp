#pragma once
#include <array>
#include "../__turbo.h"


namespace interlv {

	// interleaver internal static data
	namespace data {
		// array of block sizes and its beginning
		static const std::array<int, 6145> _ks = {
			#include "_KS.data"
		};
		// an array of shuffling indicies
		static const std::array<short, 355248> _interleave_table = {
			#include "_indexes.data"
		};
	}

	// interleaver object used in getting interleaver data
	struct interleaver_t
	{
		const short* const _ptr;

		// creates interleaver object with indicies that stored in given address
		interleaver_t(const short* const address) : _ptr(address) {}
#ifndef NDEBUD
		const int _size = 0;

		interleaver_t(const short* const address, const int size) : _ptr(address), _size(size) {}
#else
		interleaver_t(const short* const address, const int) : _ptr(address) {}
#endif // !NDEBUD

		[[nodiscard]] inline auto operator[](const size_t id) const {
			assert((id < _size));

			return (_ptr)[id];
		}
	};

	// get interleaver by block size, null if size is invalid
	/**
	*	@size	- size of information block
	*	@return - interleaver object. If size if invalid, returns invalid interleaver object
	**/
	static inline interleaver_t Get(int size) {
		if (size > 6144 || size < 0) return interleaver_t(nullptr, 0);
		return (data::_ks[size] == -1) ? interleaver_t(nullptr, 0) : interleaver_t(&(data::_interleave_table[data::_ks[size]]), size);
	}

	// validate interleaver:
	/**
	*	@x		- interliaver that is being validated
	*	@return	-  false if interleaver empty, true if interleaver exists
	**/
	static inline bool Valid(const interleaver_t& x) { return x._ptr != nullptr; }
}
