#include "SSE_Windowed.h"
#include "../../Interleaver/Interleaver.hpp"
#include "../../__turbo.h"
#include "sse_utility_win.h"

#pragma once

using namespace turbo;

/// constants for shuffling
const simd::inds_t _ALPHA_EVEN_INDS = simd::halfed_load_inds	(	0, 2, 4, 6, 0, 2, 4, 6, 
																	0, 2, 4, 6, 0, 2, 4, 6);
const simd::inds_t _ALPHA_ODD_INDS = simd::halfed_load_inds		(	1, 3, 5, 7, 1, 3, 5, 7, 
																	1, 3, 5, 7, 1, 3, 5, 7);
const simd::inds_t _BETA_REG1_SHUFFLE = simd::halfed_load_inds	(	0, 4, 5, 1, 2, 6, 7, 3, 
																	0, 4, 5, 1, 2, 6, 7, 3);
const simd::inds_t _BETA_REG2_SHUFFLE = simd::halfed_load_inds	(	1, 0, 3, 2, 5, 4, 7, 6, 
																	1, 0, 3, 2, 5, 4, 7, 6);

const simd::inds_t _BETA_BRANCH_SHUFFLE = simd::halfed_load_inds	(	0, 0, 2, 2, 2, 2, 0, 0, 
																		0, 0, 2, 2, 2, 2, 0, 0);
const simd::inds_t _ALPHA_BRANCH_SHUFFLE = simd::halfed_load_inds(	0, 3, 2, 1, 1, 2, 3, 0, 
																	0, 3, 2, 1, 1, 2, 3, 0);


/// set 0 to last four elemets of ARRAY with size=SIZE
#define _CLEAR_TAIL(ARRAY, SIZE) {ARRAY[(SIZE)] = ARRAY[(SIZE)+1] = ARRAY[(SIZE)+2] = ARRAY[(SIZE)+3] = 0;} 

bool WindowedDecoder::Decode(const dec_arr_t& in, int iters, byte_arr_t& full_out)
{
	this->_block_length = in.size() / 3 - 4;

	const auto seq_length = in.size();
	const auto seq_third = seq_length / 3;

	auto interleave = interlv::Get(this->_block_length);
	if (!interlv::Valid(interleave))
		return false;

	// rsc1 computing (u, p1)
	for (int i = 0; i < seq_third; i++)
	{
		this->_rsc1[i] = _quantize_win(in[i * 3]); // systematic
		this->_rsc1[i + seq_third] = _quantize_win(in[i * 3 + 1]);
	}
	// rsc2 computing (u, p2)
	{
		int i = 0;
		for (; i < seq_length / 3 - 4; i++)
		{
			this->_rsc2[i] = _quantize_win(in[interleave[i] * 3]);
			this->_rsc2[i + seq_third] = _quantize_win(in[i * 3 + 2]);
		}
		for (; i < seq_length / 3; i++)
		{
			this->_rsc2[i] = _quantize_win(in[i * 3]);
			this->_rsc2[i+ seq_third] = _quantize_win(in[i * 3 + 2]);
		}
	}

	// resetting '_extrinsic'
	for (int i = 0; i < this->_block_length + 4; i++)
		this->_extrinsic[i] = this->_apriori[i] = 0;

	// reset saved metrics
	this->_saved_alpha_rsc_1	= this->_saved_alpha_rsc_2	= _ALPHA_INIT_VECTOR;
	this->_saved_beta_rsc_1		= this->_saved_beta_rsc_2	= _BETA_INIT_VECTOR;

	for (int _iter = 0; _iter < iters; _iter++)
	{
		// reshuffling
		for (int j = 0; j < this->_block_length; j++)
			this->_apriori[interleave[j]] = this->_extrinsic[j];
		_CLEAR_TAIL(this->_apriori, this->_block_length);


		// siso - 1 (not interleaved)
		this->_SISO(this->_rsc1, this->_apriori, this->_out, this->_extrinsic, this->_saved_alpha_rsc_1, this->_saved_beta_rsc_1);
		this->_saved_alpha_rsc_1	= this->_temp_alpha;
		this->_saved_beta_rsc_1		= this->_temp_beta;


		// reshuffling
		for (int j = 0; j < this->_block_length; j++)
			this->_apriori[j] = this->_extrinsic[interleave[j]];
		_CLEAR_TAIL(this->_apriori, this->_block_length);

		// siso - 2 (interleaved)
		this->_SISO(this->_rsc2, this->_apriori, this->_out, this->_extrinsic, this->_saved_alpha_rsc_2, this->_saved_beta_rsc_2);
		this->_saved_alpha_rsc_2	= this->_temp_alpha;
		this->_saved_beta_rsc_2		= this->_temp_beta;
	}

	// compute output
	full_out.resize(this->_block_length);
	for (int j = 0; j < this->_block_length; j++)
		full_out[interleave[j]] = (this->_out[j] >= 0) ? 1 : 0;
	return true;
}



void WindowedDecoder::_SISO(const _rsc_t& in, const _arr_t& apriori, _arr_t& llr, _arr_t& extrinsic, sse::simd_t alpha_init, sse::simd_t beta_init)
{
	const auto length = this->_block_length + 4;
	const auto half = length / 2;


	// branch calculation
	{
		_dec_t reg1, reg2;// , br_1, br_2, br_3, br_4;
		int16_t* ptr = (int16_t*)this->_branch.data();

		size_t addr = 0;

		for (size_t i = 0, j = half; i < half; i++, j++, addr += 8)
		{
			reg1 = apriori[i] + in[(i)];
			reg2 = in[(i)+length];
			ptr[addr] = -(reg1 + reg2);
			ptr[addr + 1] = (reg2 - reg1);
			reg1 = apriori[j] + in[(j)];	
			reg2 = in[(j)+length];
			ptr[addr + 4] = -(reg1 + reg2);
			ptr[addr + 5] = (reg2 - reg1);

			assert((void*)(ptr + addr + 5) < (void*)(this->_branch.data() + this->_branch.size())); // prevent from overflow
			//this->_branch[i] = simd::fit_branch(br_1, br_2, br_3, br_4);
		}

		{
			reg1 = apriori[half] + in[(half)];
			reg2 = in[(half)+length];
			ptr[addr] = -(reg1 + reg2);
			ptr[addr + 1] = (reg2 - reg1);

			assert((void*)(ptr + addr + 1) < (void*)(this->_branch.data() + this->_branch.size())); // prevent from overflow
			//this->_branch[half] = simd::fit_branch(br_1, br_2, br_3, br_4);
		}
	}

	// alpha calculation
	{
		simd::simd_t reg, branch;

		reg = alpha_init;
		this->_alpha[0] = reg;
		//_DEB_VEC(reg, 0);

		for (int k = 1; k <= half; k++)
		{
			branch = simd::fit_branch(this->_branch[k - 1]);
			this->_branch[k - 1] = branch;

			branch = simd::shuffle(branch, _ALPHA_BRANCH_SHUFFLE);

			reg = simd::max(
				simd::add(simd::shuffle(reg, _ALPHA_EVEN_INDS), branch),
				simd::sub(simd::shuffle(reg, _ALPHA_ODD_INDS), branch)
			);
			reg = simd::normalize(reg);

			this->_alpha[k] = reg;
		}

		this->_temp_alpha = simd::set_halfs(_LEAST_HALF_VECTOR,
											simd::get_half<0>(this->_alpha[half]));
	}

	// beta + branch calculation
	{
		simd::simd_t
			// [0,4,5,1,2,6,7,3]
			beta_reg1 = beta_init,
			// [4,0,1,5,6,2,3,7]
			beta_reg2;

		beta_reg2 = simd::shuffle(beta_reg1, _BETA_REG2_SHUFFLE); // shuffle reg2 from [0,4,5,1,2,6,7,3] to [4,0,1,5,6,2,3,7]

		// branch vector
		simd::simd_t branch, llr_1, llr_2, alpha;

		branch = simd::shuffle(this->_branch[half], _BETA_BRANCH_SHUFFLE);

		beta_reg1 = simd::shuffle(simd::max(simd::add(beta_reg1, branch), simd::sub(beta_reg2, branch)), _BETA_REG1_SHUFFLE);
		beta_reg1 = simd::normalize(beta_reg1);
		beta_reg2 = simd::shuffle(beta_reg1, _BETA_REG2_SHUFFLE);


		for (int k = half - 1; k > 0; k--)
		{
			branch = simd::shuffle(this->_branch[k], _BETA_BRANCH_SHUFFLE);

			alpha = this->_alpha[k];
			

			llr_1 = simd::sub(simd::add(alpha, beta_reg2), branch);
			llr_2 = simd::add(simd::add(alpha, beta_reg1), branch);


			llr_1 = simd::sub(simd::max_in_half(llr_1), simd::max_in_half(llr_2));

			llr[k] = simd::extract<7>(llr_1);
			llr[k + half] = simd::extract<15>(llr_1);

			beta_reg1 = simd::shuffle(simd::max(simd::add(beta_reg1, branch), simd::sub(beta_reg2, branch)), _BETA_REG1_SHUFFLE);
			beta_reg1 = simd::normalize(beta_reg1);
			beta_reg2 = simd::shuffle(beta_reg1, _BETA_REG2_SHUFFLE);
		} 
		
		{
			branch = simd::shuffle(this->_branch[0], _BETA_BRANCH_SHUFFLE);


			alpha = this->_alpha[0];

			llr_1 = simd::sub(simd::add(alpha, beta_reg2), branch);
			llr_2 = simd::add(simd::add(alpha, beta_reg1), branch);

			llr_1 = simd::sub(simd::max_in_half(llr_1), simd::max_in_half(llr_2));

			llr[0] = simd::extract<7>(llr_1);
			//std::cout << (int) 
			llr[half] = simd::extract<15>(llr_1);

			this->_temp_beta = simd::set_halfs(simd::get_half<1>(beta_reg1),
				_LEAST_HALF_VECTOR);

		}

		for (int i = 0; i < this->_block_length; i++)
			extrinsic[i] = (((llr[i] - apriori[i] - in[i])>>2)*3);// *3 / 4;
	}
}