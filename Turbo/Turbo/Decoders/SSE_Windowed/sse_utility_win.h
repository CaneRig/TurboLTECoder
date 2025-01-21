#pragma once
#include "../SIMD/SSE.h"
#include "SSE_Windowed.h"

namespace simd = sse;


constexpr int8_t _MAX_INT8 = 127;
constexpr int8_t _LEAST_VAL = -64;

const simd::simd_t _LEAST_VECTOR = simd::load(	0, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL,
												0, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL);

const simd::simd_t _ALPHA_INIT_VECTOR = simd::load(	0, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL,
													0,	0,	0,	0,	0,	0,	0,	0);

const simd::simd_t _BETA_INIT_VECTOR = simd::load(	0, 0, 0, 0, 0, 0, 0, 0,
													0, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL, _LEAST_VAL);

const int64_t _LEAST_HALF_VECTOR = simd::get_half<0>(_LEAST_VECTOR);

// clamping "val" between -_MAX_INT16 and _MAX_INT16
inline constexpr int8_t _saturate8(int val) {
	val = (val < -_MAX_INT8) ? -_MAX_INT8 : val;
	return  (val > _MAX_INT8) ? _MAX_INT8 : val;
}

// convert float to int8 representation
inline constexpr turbo::_dec_t _quantize_win(float val) {
	return val; //  _saturate8(int(val * 4));
}

constexpr turbo::_dec_t operator""_dec(long double d) {
	return _quantize_win((float)d);
}