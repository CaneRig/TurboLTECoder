#include "AVX_Windowed.h"
#if USED_AVX
#include "../../Interleaver/Interleaver.hpp"
#include "../../__turbo.h"
#include "avx_utility_win.h"

using namespace turbo;

/// constants for shuffling
const simd::inds_t _ALPHA_EVEN_INDS = simd::piecemeal_load_inds(0, 2, 4, 6, 0, 2, 4, 6);
const simd::inds_t _ALPHA_ODD_INDS = simd::piecemeal_load_inds(1, 3, 5, 7, 1, 3, 5, 7);
const simd::inds_t _BETA_REG1_INDS = simd::piecemeal_load_inds(0, 4, 5, 1, 2, 6, 7, 3);
const simd::inds_t _BETA_REG2_INDS = simd::piecemeal_load_inds(1, 0, 3, 2, 5, 4, 7, 6);

const simd::inds_t _BETA_BRANCH_INDS = simd::piecemeal_load_inds(0, 0, 2, 2, 2, 2, 0, 0);
const simd::inds_t _ALPHA_BRANCH_INDS= simd::piecemeal_load_inds(0, 3, 2, 1, 1, 2, 3, 0);

const simd::inds_t _NORMALIZE_INDS = simd::piecemeal_load_inds(0, 0, 0, 0, 0, 0, 0, 0);

//

/// set 0 to last four elemets of ARRAY with size=SIZE
#define _CLEAR_TAIL(ARRAY, SIZE) {ARRAY[(SIZE)] = ARRAY[(SIZE)+1] = ARRAY[(SIZE)+2] = ARRAY[(SIZE)+3] = 0;} 

bool WindowedDecoderAVX::Decode(const dec_arr_t& in, int iters, byte_arr_t& full_out)
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
		this->_rsc_sys[i] = (in[i * 3]); // systematic
		this->_rsc_par[i] = (in[i * 3 + 1]);
	}
	// rsc2 computing (u, p2)
	{
		int i = 0;
		for (; i < seq_length / 3 - 4; i++)
		{
			this->_rsc_sys[i + seq_third] = (in[interleave[i] * 3]);
			this->_rsc_par[i + seq_third] = (in[i * 3 + 2]);
		}
		for (; i < seq_length / 3; i++)
		{
			this->_rsc_sys[i + seq_third] = (in[i * 3]);
			this->_rsc_par[i + seq_third] = (in[i * 3 + 2]);
		}
	}

	// zero apriori
	for (int i = 0; i < this->_block_length + 4; i++)
		this->_apriori[i] = 0;

	this->_init_alpha = _ALPHA_INIT_VECTOR;
	this->_init_beta = _BETA_INIT_VECTOR;

	for (int i = 0; i < iters; i++)
	{
		_SISO();
		
		for (size_t i = 0; i < this->_block_length; i++) {

			std::swap(this->_apriori[i], this->_apriori[interleave[i] + seq_third]);
		}
	}

	// compute output
	full_out.resize(this->_block_length);
	for (int j = 0; j < this->_block_length; j++)
		full_out[j] = (this->_llr[j] >= 0) ? 1 : 0;
	
	return true;
}

void WindowedDecoderAVX::_SISO()
{
	const auto length = this->_block_length + 4;
	const auto half = length / 2;
	const auto length_hc = length + half;

	// branch calculation
	{
		int16_t reg1, reg2;

		int16_t* addr;

		// rsc1
		addr = (int16_t*)this->_branch.data();
		for (int k = 0, j = half; k < half; k++, j++, addr += 256 / 16)
		{
			reg1 = this->_apriori[k] + this->_rsc_sys[k];
			reg2 = this->_rsc_par[k];

			addr[0] = -(reg1 + reg2);
			addr[1] = reg2 - reg1;

			reg1 = this->_apriori[j] + this->_rsc_sys[j];
			reg2 = this->_rsc_par[j];

			addr[4] = -(reg1 + reg2);
			addr[5] = reg2 - reg1;
		}
		{
			reg1 = this->_apriori[half] + this->_rsc_sys[half];
			reg2 = this->_rsc_par[half];

			addr[0] = -(reg1 + reg2);
			addr[1] = reg2 - reg1;
		}

		// rsc2
		addr = (int16_t*)this->_branch.data() + 128 / 16;
		for (int k = length, j = half + length; k < length_hc; k++, j++, addr += 256 / 16)
		{
			reg1 = this->_apriori[k] + this->_rsc_sys[k];
			reg2 = this->_rsc_par[k];

			addr[0] = -(reg1 + reg2);
			addr[1] = reg2 - reg1;

			reg1 = this->_apriori[j] + this->_rsc_sys[j];
			reg2 = this->_rsc_par[j];

			addr[4] = -(reg1 + reg2);
			addr[5] = reg2 - reg1;
		}
		{
			reg1 = this->_apriori[length_hc] + this->_rsc_sys[length_hc];
			reg2 = this->_rsc_par[length_hc];

			addr[0] = -(reg1 + reg2);
			addr[1] = reg2 - reg1;
		}
	}

	// alpha calculation
	{
		simd::simd_t branch, reg; // reg stores previous iteration alpha

		reg = this->_init_alpha;
		this->_alpha[0] = reg;

		for (int k = 1; k <= half; k++)
		{
			branch = simd::fit_branch(this->_branch[k - 1]); // compute branch
			this->_branch[k - 1] = branch; // saving

			branch = simd::shuffle(branch, _ALPHA_BRANCH_INDS); // preparing for alpha calculation

			reg = simd::max( // current alpha calculation
				simd::add(simd::shuffle(reg, _ALPHA_EVEN_INDS), branch),
				simd::sub(simd::shuffle(reg, _ALPHA_ODD_INDS), branch)
			);
			reg = simd::sub(reg, simd::shuffle(reg, _NORMALIZE_INDS)); // normalization

			this->_alpha[k] = reg; // saving
		}

		this->_init_alpha = simd::load_vetors(_LEAST_VEC, simd::get_vector<0>(reg), _LEAST_VEC, simd::get_vector<2>(reg)); // preparing initial vector for next SISO decoder
	}

	// beta + branch calculation
	{
		simd::simd_t
			// [0,4,5,1,2,6,7,3]
			beta_reg1 = this->_init_beta,
			// [4,0,1,5,6,2,3,7]
			beta_reg2;

		beta_reg2 = simd::shuffle(beta_reg1, _BETA_REG2_INDS); // shuffle reg2 from [0,4,5,1,2,6,7,3] to [4,0,1,5,6,2,3,7]

		// branch vector
		simd::simd_t branch, llr_1, llr_2, alpha;


		for (int k = half - 1; k > 0; k--)
		{
			branch = simd::shuffle(this->_branch[k], _BETA_BRANCH_INDS);

			alpha = this->_alpha[k];


			llr_1 = simd::sub(simd::add(alpha, beta_reg2), branch);
			llr_2 = simd::add(simd::add(alpha, beta_reg1), branch);


			llr_1 = simd::sub(simd::max_in_vec(llr_1), simd::max_in_vec(llr_2));

			this->_llr[k] = simd::extract<7>(llr_1); // win1 rsc1
			this->_llr[k + half] = simd::extract<15>(llr_1); // win2 rsc1
			this->_llr[k + length] = simd::extract<7 + 16>(llr_1); // win1 rsc2 
			this->_llr[k + length_hc] = simd::extract<15 + 16>(llr_1); // win2 rsc2

			beta_reg1 = simd::shuffle(simd::max(simd::add(beta_reg1, branch), simd::sub(beta_reg2, branch)), _BETA_REG1_INDS);
			beta_reg1 = simd::sub(beta_reg1, simd::shuffle(beta_reg1, _NORMALIZE_INDS));
			beta_reg2 = simd::shuffle(beta_reg1, _BETA_REG2_INDS);
		}
		{
			branch = simd::shuffle(this->_branch[0], _BETA_BRANCH_INDS);

			alpha = this->_alpha[0];


			llr_1 = simd::sub(simd::add(alpha, beta_reg2), branch);
			llr_2 = simd::add(simd::add(alpha, beta_reg1), branch);


			llr_1 = simd::sub(simd::max_in_vec(llr_1), simd::max_in_vec(llr_2));

			this->_llr[0] = simd::extract<7>(llr_1); // win1 rsc1
			this->_llr[half] = simd::extract<15>(llr_1); // win2 rsc1
			this->_llr[length] = simd::extract<7 + 16>(llr_1); // win1 rsc2 
			this->_llr[length_hc] = simd::extract<15 + 16>(llr_1); // win2 rsc2

			this->_init_beta = simd::load_vetors(simd::get_vector<1>(beta_reg1), _LEAST_VEC, simd::get_vector<3>(beta_reg1), _LEAST_VEC); // preparing initial vector for next SISO 
		}
	}

	for (int i = 0; i < this->_block_length*2; i++) 
		this->_apriori[i] = (((this->_llr[i] - this->_apriori[i] - this->_rsc_sys[i]) >> 2) * 3);// *3 / 4;
}
#endif