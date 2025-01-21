#pragma once
#include "../SIMD/AVX.h"
#if USED_AVX
#include "../../__turbo.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <array>

namespace turbo {
	using _dec_t = int8_t;

	// AVX2 realisation of turbo decoder with Windowing optimization
	class WindowedDecoderAVX {
	public:
		WindowedDecoderAVX() {

		}

		~WindowedDecoderAVX() {}

		 /**
		 *	@in		 - input array where negative values are probability of being zero, and positive of being one
		 *	@iters	 - number of decoding iterations
		 *	@full_out- decoder destination
		 *	@return  - true if input values are correct, false overwize
		 **/
		bool Decode(const dec_arr_t& in, int iters, byte_arr_t& full_out);
	private:
		static const int _MAX_IN_BLOCK_SIZE = 6144 + 4;

		using _avarr_t = std::array<avx::simd_t, _MAX_IN_BLOCK_SIZE / 2 + 1>;
		using _arr_t = std::array<_dec_t, _MAX_IN_BLOCK_SIZE * 2>;
		using _rsc_t = std::array<_dec_t, _MAX_IN_BLOCK_SIZE * 2>;

		size_t _block_length = 0;

		alignas(avx::simd_t) _avarr_t _branch, _alpha;

		alignas(avx::simd_t) _rsc_t _rsc_sys, _llr, _rsc_par;

		alignas(avx::simd_t) _arr_t _apriori;

		avx::simd_t _init_alpha, _init_beta;

		void _SISO();
	};
}
#endif