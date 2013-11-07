/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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
#include "rateest.h"
#include <cstdlib>

void RateEst::SumQueue::push(std::ptrdiff_t const samples, usec_t const usecs) {
	q_.push_back(std::make_pair(samples, usecs));
	samples_ += samples;
	usecs_ += usecs;
}

void RateEst::SumQueue::pop() {
	std::pair<std::ptrdiff_t, usec_t> const &f = q_.front();
	samples_ -= f.first;
	usecs_ -= f.second;
	q_.pop_front();
}

static usec_t sampleUsecs(std::ptrdiff_t samples, long rate) {
	return usec_t((samples * 1000000.0f) / (rate ? rate : 1) + 0.5f);
}

static long limit(long est, long const reference) {
	if (est > reference + (reference >> 6))
		est = reference + (reference >> 6);
	else if (est < reference - (reference >> 6))
		est = reference - (reference >> 6);

	return est;
}

enum { mean_scale = 1 << 5 };

RateEst::RateEst(long const nominalSampleRate, std::size_t const maxValidFeedPeriodSamples)
: srate_(nominalSampleRate * est_scale)
, reference_(srate_)
, maxPeriod_(sampleUsecs(maxValidFeedPeriodSamples, nominalSampleRate))
, last_(0)
, usecs_(12000000 * mean_scale)
, samples_(nominalSampleRate * 12 * mean_scale)
{
}

void RateEst::feed(std::ptrdiff_t samplesIn, usec_t const now) {
	usec_t usecsIn = now - last_;

	if (last_ && usecsIn < maxPeriod_) {
		sumq_.push(samplesIn, usecsIn);

		while ((usecsIn = sumq_.usecs()) > 100000) {
			samplesIn = sumq_.samples();
			sumq_.pop();

			long const srateIn = long(samplesIn * (1000000.0f * est_scale) / usecsIn);
			if (std::abs(srateIn - reference_) < reference_ >> 1) {
				samples_ += (samplesIn - sumq_.samples()) * mean_scale;
				usecs_   += (  usecsIn - sumq_.usecs()  ) * mean_scale;

				long est = long(samples_ * (1000000.0f * est_scale) / usecs_ + 0.5f);
				est = limit((srate_ * 31 + est + 16) >> 5, reference_);
				srate_ = est;

				if (usecs_ > 16000000 * mean_scale) {
					samples_ = (samples_ * 3 + 2) >> 2;
					usecs_   = (usecs_   * 3 + 2) >> 2;
				}
			}
		}
	}

	last_ = now;
}
