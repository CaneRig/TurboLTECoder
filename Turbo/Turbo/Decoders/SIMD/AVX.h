// Header contains bindings for avx2
#pragma once
#ifndef AVX_H
#define AVX_H

#include <immintrin.h>
#include <cstdint>


#include <bit>
#if USED_AVX
static_assert(std::endian::native == std::endian::little, "Little endian required");
namespace avx {
	using inds_t = __m256i;
	using simd_t = __m256i;
	using var_t = int8_t;
	using vec_t = int64_t;


	inline inds_t piecemeal_load_inds(const uint8_t v1, const uint8_t v2, const uint8_t v3, const uint8_t v4, const uint8_t v5, const uint8_t v6, const uint8_t v7, const uint8_t v8) {
		return _mm256_setr_epi8(v1 + 0, v2 + 0, v3 + 0, v4 + 0, v5 + 0, v6 + 0, v7 + 0, v8 + 0, v1 + 8, v2 + 8, v3 + 8, v4 + 8, v5 + 8, v6 + 8, v7 + 8, v8 + 8, v1 + 16, v2 + 16, v3 + 16, v4 + 16, v5 + 16, v6 + 16, v7 + 16, v8 + 16, v1 + 24, v2 + 24, v3 + 24, v4 + 24, v5 + 24, v6 + 24, v7 + 24, v8 + 24);
	}
	inline inds_t piecemeal_loadx2(const var_t v0, const var_t v1, const var_t v2, const var_t v3, const var_t v4, const var_t v5, const var_t v6, const var_t v7,
		const var_t v8, const var_t v9, const var_t v10, const var_t v11, const var_t v12, const var_t v13, const var_t v14, const var_t v15) {
		return _mm256_setr_epi8(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15);
	}

	inline simd_t add(simd_t a, simd_t b) {
		return _mm256_adds_epi8(a, b);
	}
	inline simd_t sub(simd_t a, simd_t b) {
		return _mm256_subs_epi8(a, b);
	}
	inline simd_t max(simd_t a, simd_t b) {
		return _mm256_max_epi8(a, b);
	}
	template<int INDEX>
	inline var_t extract(simd_t vec) {
		return _mm256_extract_epi8(vec, INDEX);
	}

	inline simd_t shuffle(simd_t vec, inds_t inds) {
		return _mm256_shuffle_epi8(vec, inds);
	}

	inline simd_t load_vetors(vec_t a, vec_t b, vec_t c, vec_t d) {
		return _mm256_setr_epi64x(a, b, c, d);
	}

	template<int INDEX>
	inline vec_t get_vector(simd_t v) {
		return _mm256_extract_epi64(v, INDEX);
	}

	template<const int s>
	inline simd_t lshift(simd_t v) {
		return _mm256_slli_si256(v, s);
	}

	inline simd_t max_in_vec(simd_t vec) {
		vec = max(vec, lshift<4>(vec));
		vec = max(vec, lshift<2>(vec));
		vec = max(vec, lshift<1>(vec));

		return vec;
	}

	const simd_t _FIT_BRANCH_MASK_1 = piecemeal_loadx2(0xff, 0, 0xff, 0, 0, 0, 0, 0, 0xff, 0, 0xff, 0, 0, 0, 0, 0);

	inline simd_t fit_branch(simd_t v) {
		v = _mm256_srai_epi16(v, 1); // dividing by 2 with +/- 1 for negative numbers

		v = _mm256_and_si256(v, _FIT_BRANCH_MASK_1); // convert to 8 bit

		v = _mm256_subs_epi8(v, _mm256_bslli_epi128(v, 1)); // add negative numbers

		return v;
	}

	template<int ID>
	int extract_int32(simd_t v) {
		return _mm256_extract_epi32(v, ID);
	}
}
#endif
#endif