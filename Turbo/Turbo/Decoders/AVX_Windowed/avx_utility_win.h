#pragma once

#include "../SIMD/AVX.h"

namespace simd = avx;

constexpr int8_t _MAX_INT8 = 127;
constexpr int8_t _LEAST_VAL = -64;

const simd::simd_t _LEAST_AVXVEC = _mm256_set1_epi8(_LEAST_VAL);

const simd::vec_t _LEAST_VEC = simd::get_vector<0>(_LEAST_AVXVEC);


const simd::simd_t _ALPHA_INIT_VECTOR = simd::piecemeal_loadx2(	0, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL,
																0,	0,	0,	0,	0,	0,	0,	0);

const simd::simd_t _BETA_INIT_VECTOR = simd::piecemeal_loadx2(	0, 0, 0, 0, 0, 0, 0, 0,
																0, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL);