/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aamås                                    *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "makesinckernel.h"
#include "array.h"

void makeSincKernel(short *const kernel, const unsigned phases,
		const unsigned phaseLen, SysDDec fc, SysDDec (*win)(long m, long M), SysDDec const maxAllowedGain) {
	static const SysDDec PI = 3.14159265358979323846;
	fc /= phases;
	
	/*{
		const Array<SysDDec> dkernel(phaseLen * phases);
		const long M = static_cast<long>(phaseLen) * phases - 1;
		
		for (long i = 0; i < M + 1; ++i) {
			const SysDDec sinc = i * 2 == M ?
					PI * fc :
					std::sin(PI * fc * (i * 2 - M)) / (i * 2 - M);
			
			dkernel[((phases - (i % phases)) % phases) * phaseLen + i / phases] = win(i, M) * sinc;
		}
		
		SysDDec maxabsgain = 0;
		
		for (unsigned ph = 0; ph < phases; ++ph) {
			SysDDec gain = 0;
			SysDDec absgain = 0;
			
			for (unsigned i = 0; i < phaseLen; ++i) {
				gain += dkernel[ph * phaseLen + i];
				absgain += std::abs(dkernel[ph * phaseLen + i]);
			}
			
			gain = 1.0 / gain;
			
			// Per phase normalization to avoid DC fluctuations.
			for (unsigned i = 0; i < phaseLen; ++i)
				dkernel[ph * phaseLen + i] *= gain;
			
			absgain *= gain;
			
			if (absgain > maxabsgain)
				maxabsgain = absgain;
		}
		
		const SysDDec gain = (0x10000 - 0.5 * phaseLen) * maxAllowedGain / maxabsgain;
		
		for (long i = 0; i < M + 1; ++i)
			kernel[i] = std::floor(dkernel[i] * gain + 0.5);
	}*/
	
	// The following is equivalent to the more readable version above
	
	const long M = static_cast<long>(phaseLen) * phases - 1;
	const Array<SysDDec> dkernel(M / 2 + 1);
	
	{
		SysDDec *dk = dkernel;
		
		for (unsigned ph = 0; ph < phases; ++ph) {
			for (long i = ph; i < M / 2 + 1; i += phases) {
				const SysDDec sinc = i * 2 == M ?
						PI * fc :
						std::sin(PI * fc * (i * 2 - M)) / (i * 2 - M);
				
				*dk++ = win(i, M) * sinc;
			}
		}
	}
	
	SysDDec maxabsgain = 0.0;
	
	{
		SysDDec *dkp1 = dkernel;
		SysDDec *dkp2 = dkernel + M / 2;
		
		for (unsigned ph = 0; ph < (phases + 1) / 2; ++ph) {
			SysDDec gain = 0.0;
			SysDDec absgain = 0.0;
			
			{
				const SysDDec *kp1 = dkp1;
				const SysDDec *kp2 = dkp2;
				long i = ph;
				
				for (; i < M / 2 + 1; i += phases) {
					gain += *kp1;
					absgain += std::abs(*kp1++);
				}
				
				for (; i < M + 1; i += phases) {
					gain += *kp2;
					absgain += std::abs(*kp2--);
				}
			}
			
			gain = 1.0 / gain;
			
			long i = ph;
			
			for (; i < M / 2 + 1; i += phases)
				*dkp1++ *= gain;
			
			if (dkp1 < dkp2) {
				for (; i < M + 1; i += phases)
					*dkp2-- *= gain;
			}
			
			absgain *= gain;
			
			if (absgain > maxabsgain)
				maxabsgain = absgain;
		}
	}
	
	const SysDDec gain = (0x10000 - 0.5 * phaseLen) * maxAllowedGain / maxabsgain;
	const SysDDec *dk = dkernel;
	
	for (unsigned ph = 0; ph < phases; ++ph) {
		short *k = kernel + ((phases - ph) % phases) * phaseLen;
		short *km = kernel + phaseLen - 1 + ((ph + 1) % phases) * phaseLen;
		
		for (long i = ph; i < M / 2 + 1; i += phases)
			*km-- = *k++ = static_cast<short>(std::floor(*dk++ * gain + 0.5));
	}
}
