// Header contains bindings for sse4.1
#pragma once
#ifndef SSE_H
#define SSE_H

#include <immintrin.h>
#include <cstdint>
#undef max

#include <bit>
static_assert(std::endian::native == std::endian::little, "Little endian required");

namespace sse {
	using inds_t = __m128i;
	using simd_t = __m128i;
	using var_t = int8_t;

	const __m128i _ONES = _mm_set1_epi32(~0);

	inline inds_t load_inds(const uint8_t v1, const uint8_t v2, const uint8_t v3, const uint8_t v4,
							const uint8_t v5, const uint8_t v6, const uint8_t v7, const uint8_t v8,
							const uint8_t v9, const uint8_t v10, const uint8_t v11, const uint8_t v12,
							const uint8_t v13, const uint8_t v14, const uint8_t v15, const uint8_t v16) {
		return _mm_set_epi8(v16, v15, v14, v13, v12, v11, v10, v9, v8, v7, v6, v5, v4, v3, v2, v1);
	}
	// adds to second half 8
	inline inds_t halfed_load_inds(const uint8_t v1, const uint8_t v2, const uint8_t v3, const uint8_t v4,
								   const uint8_t v5, const uint8_t v6, const uint8_t v7, const uint8_t v8,
								   const uint8_t v9, const uint8_t v10, const uint8_t v11, const uint8_t v12,
								   const uint8_t v13, const uint8_t v14, const uint8_t v15, const uint8_t v16) {
		return _mm_set_epi8(v16 + 8, v15 + 8, v14 + 8, v13 + 8, v12 + 8, v11 + 8, v10 + 8, v9 + 8, v8, v7, v6, v5, v4, v3, v2, v1);
	}
	inline inds_t halfed_load_inds_rev(const uint8_t v1, const uint8_t v2, const uint8_t v3, const uint8_t v4,
									   const uint8_t v5, const uint8_t v6, const uint8_t v7, const uint8_t v8,
									   const uint8_t v9, const uint8_t v10, const uint8_t v11, const uint8_t v12,
									   const uint8_t v13, const uint8_t v14, const uint8_t v15, const uint8_t v16) {
		return _mm_set_epi8(v16, v15, v14, v13, v12, v11, v10, v9, v8 + 8, v7 + 8, v6 + 8, v5 + 8, v4 + 8, v3 + 8, v2 + 8, v1 + 8);
	}

	const inds_t _NORMALIZE_INDS = halfed_load_inds(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	const inds_t _HALF_SHUFFLE = load_inds(0 + 8, 1 + 8, 2 + 8, 3 + 8, 4 + 8, 5 + 8, 6 + 8, 7 + 8,
										   0    , 1    , 2    , 3    , 4    , 5    , 6    , 7    );

	inline simd_t add(simd_t a, simd_t b) {
		return _mm_adds_epi8(a, b);
	}
	inline simd_t sub(simd_t a, simd_t b) {
		return _mm_subs_epi8(a, b);
	}
	inline simd_t max(simd_t a, simd_t b) {
		return _mm_max_epi8(a, b);
	}

	inline simd_t set(var_t val) {
		return _mm_set1_epi8(val);
	}
	inline void store(simd_t val, void* dest) {
		_mm_store_si128((simd_t*)dest, val);
	}

	template<int INDEX>
	inline var_t extract(simd_t vec) {
		return _mm_extract_epi8(vec, INDEX);
	}

	inline simd_t load(const var_t v1, const var_t v2,  const var_t v3, const  var_t v4,
					   const var_t v5, const var_t v6,  const var_t v7, const  var_t v8,
					   const var_t v9, const var_t v10, const var_t v11, const var_t v12,
					   const var_t v13,const var_t v14, const var_t v15, const var_t v16) {
		return _mm_set_epi8(v16, v15, v14, v13, v12, v11, v10, v9, v8, v7, v6, v5, v4, v3, v2, v1);
	}
	inline simd_t load_addr(simd_t* addr) {
		return _mm_load_si128(addr);
	}

	inline simd_t from_aligned_array(simd_t* p) {
		return _mm_load_si128(p);
	}

	inline simd_t fill(var_t value) {
		return _mm_set1_epi8(value);
	}
	template<int Half>
	inline simd_t insert_half(simd_t v, int64_t val) {
		return _mm_insert_epi64(v, val, Half);
	}
	template<int Half>
	inline int64_t get_half(simd_t v) {
		return _mm_extract_epi64(v, Half);
	}

	const simd_t _FIT_BRANCH_MASK_1 = load(0xff, 0, 0xff, 0, 0, 0, 0, 0, 0xff, 0, 0xff, 0, 0, 0, 0, 0);
	inline simd_t fit_branch(var_t a, var_t b,
		var_t e, var_t f) {
		auto v = _mm_setr_epi16(a, b, 0, 0, e, f, 0, 0);

		v = _mm_srai_epi16(v, 1); // dividing by 2 with +/- 1 for negative numbers

		v = _mm_and_si128(v, _FIT_BRANCH_MASK_1); // convert to 8 bit

		v = _mm_subs_epi8(v, _mm_bslli_si128(v, 1)); // add negative numbers

		return v;
	}
	inline simd_t fit_branch(simd_t v) {
		v = _mm_srai_epi16(v, 1); // dividing by 2 with +/- 1 for negative numbers

		v = _mm_and_si128(v, _FIT_BRANCH_MASK_1); // convert to 8 bit

		v = _mm_subs_epi8(v, _mm_bslli_si128(v, 1)); // add negative numbers

		return v;
	}

	inline simd_t shuffle(simd_t vec, inds_t inds) {
		return _mm_shuffle_epi8(vec, inds);
	}
	inline simd_t bit_invert(simd_t vec) {
		return _mm_xor_si128(vec, _ONES);
	}

	template<const int s>
	inline simd_t lshift(simd_t v) {
		return _mm_bslli_si128(v, s);
	}

	inline simd_t max_in_half(simd_t vec) {
		vec = max(vec, lshift<4>(vec));
		vec = max(vec, lshift<2>(vec));
		vec = max(vec, lshift<1>(vec));

		return vec;
	}

	inline simd_t normalize(simd_t v) {
		return sub(v, shuffle(v, _NORMALIZE_INDS));
	}

	inline simd_t swap_halfs(simd_t v) {
		return shuffle(v, _HALF_SHUFFLE);
		//__m128d a = *reinterpret_cast<
		//return _mm_shuffle_pd(v, v, 1);
	}

	inline simd_t set_half(simd_t v, int64_t e) {
		return _mm_insert_epi64(v, e, 0);
	}
	inline simd_t set_halfs(int64_t a, int64_t b) {
		return _mm_set_epi64x(b, a);
	}

	// satrurate8((llr-apriori-in)/4*3)
	const simd_t EXCTRINSIC_CALC_CONST = _mm_set1_epi16(3);
	inline simd_t exctrinsic_calc(simd_t llr, simd_t apriori, simd_t in) {
		// _mm_bsrli_si128(_, 8)
#define _EXCTRINSIC_CALC_HALF(M_LLR, M_APRIORI, M_IN, M_RES) 		M_RES = _mm_srai_epi16(_mm_subs_epi16(_mm_subs_epi16(M_LLR, M_APRIORI), M_IN), 2);		\
																	M_RES = _mm_adds_epi16(M_RES, _mm_adds_epi16(M_RES, M_RES));

		simd_t v_llr, v_apriori, v_in, res_1, res_2;


		v_llr = _mm_cvtepi8_epi16(llr);
		v_apriori = _mm_cvtepi8_epi16(apriori);
		v_in = _mm_cvtepi8_epi16(in);

		_EXCTRINSIC_CALC_HALF(v_llr, v_apriori, v_in, res_1);


		v_llr = _mm_cvtepi8_epi16(_mm_bsrli_si128(llr, 2));
		v_apriori = _mm_cvtepi8_epi16(_mm_bsrli_si128(apriori, 2));
		v_in = _mm_cvtepi8_epi16(_mm_bsrli_si128(in, 2));

		_EXCTRINSIC_CALC_HALF(v_llr, v_apriori, v_in, res_2);

		return _mm_packs_epi16(res_1, res_2);
	}
}
#endif