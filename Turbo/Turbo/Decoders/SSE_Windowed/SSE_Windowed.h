#pragma once
#include "../SIMD/SSE.h"
#include "../../__turbo.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <array>

namespace turbo {
	using _dec_t = int8_t;

	// SSE realisation of turbo decoder with Windowing optimization
	class WindowedDecoder {
	public:
		WindowedDecoder() {}
		~WindowedDecoder() {}

		/**
		*	@in		 - input array where negative values are probability of being zero, and positive of being one
		*	@iters	 - number of decoding iterations
		*	@full_out- decoder destination
		*	@return  - true if input values are correct, false overwize
		**/
		bool Decode(const dec_arr_t& in, int iters, byte_arr_t& full_out);
	private:
		static const int _MAX_IN_BLOCK_SIZE = 6144 + 4;
		static const int _MAX_RSC_SIZE = _MAX_IN_BLOCK_SIZE * 2;

		using _rsc_t = std::array<_dec_t, _MAX_RSC_SIZE>;
		using _arr_t = std::array<_dec_t, _MAX_IN_BLOCK_SIZE>;

		size_t _block_length = 0;

		alignas(sse::simd_t) _rsc_t _rsc1;
		alignas(sse::simd_t) _rsc_t _rsc2;

		alignas(sse::simd_t) _arr_t _apriori;
		alignas(sse::simd_t) _arr_t _extrinsic;
		alignas(sse::simd_t) _arr_t _out;

		alignas(sse::simd_t) std::array<sse::simd_t, _MAX_IN_BLOCK_SIZE / 2 + 1> _alpha, _branch;

		sse::simd_t _saved_alpha_rsc_1, _saved_alpha_rsc_2;
		sse::simd_t _saved_beta_rsc_1, _saved_beta_rsc_2;
		sse::simd_t _temp_alpha, _temp_beta;


		void _SISO(const _rsc_t& in, const _arr_t& apriori, _arr_t& llr, _arr_t& extrinsic, sse::simd_t alpha_init, sse::simd_t beta_init);

	};
}