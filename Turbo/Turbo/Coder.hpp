#include "__turbo.h"
#include "Interleaver/Interleaver.hpp"
#pragma once

#define __TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER					\
		a = input[i] ^ d ^ c;										\
		dest[i * 3] = input[i];										\
		dest[i * 3 + 1] = a ^ b ^ d;								\
		d = c;														\
		c = b;														\
		b = a;														\
		i++;

#define __TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER					\
		a = input[interleave[i]] ^ d ^ c;							\
		dest[i * 3 + 2] = a ^ b ^ d;								\
		d = c;														\
		c = b;														\
		b = a;														\
		i++;																

#define __TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER_TAIL				\
		a = d ^ c;													\
		dest[i * 3] = 0;											\
		dest[i * 3 + 1] = a ^ b ^ d;								\
		d = c;														\
		c = b;														\
		b = a;

#define __TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER_TAIL				\
		a = d ^ c;													\
		dest[i * 3 + 2] = a ^ b ^ d;								\
		d = c;														\
		c = b;														\
		b = a;	

namespace turbo {
	// Turbo encoder
	/**
	*	@input	- array containing input data
	*	@dest	- array with encoded data, it resizes to: input.size() * 3 + 12
	*	@return - false if @input has invalid size according to standart, otherwise false
	**/
	bool Encode(const byte_arr_t& input, byte_arr_t& dest) {
		dest.resize(input.size() * 3 + 4 * 3);

		auto interleave = interlv::Get(input.size());
		if (!interlv::Valid(interleave))
			return false;

		const int size = input.size();

		// u, p1
		{
			byte_t a, b = 0, c = 0, d = 0;

			int i = 0;
			for (; i < size;)
			{
				__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER;

				__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER;
			}

			// tail bits
			__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER_TAIL;
			__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER_TAIL;
			__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER_TAIL;
			__TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER_TAIL;
		}

		// p2
		{
			byte_t a, b = 0, c = 0, d = 0;

			int i = 0;
			for (; i < size;)
			{
				__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER;

				__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER;
				__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER;
			}

			// p2 tail
			__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER_TAIL;
			__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER_TAIL;
			__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER_TAIL;
			__TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER_TAIL;
		}

		return true;
	}
}
#undef __TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER
#undef __TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER
#undef __TURBO_ENCODER_LOOP_BODY_NO_INTERLEAVER_TAIL
#undef __TURBO_ENCODER_LOOP_BODY_WITH_INTERLEAVER_TAIL