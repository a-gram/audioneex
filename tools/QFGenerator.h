/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef QFGENERATOR_H
#define QFGENERATOR_H

#include <cstdint>
#include <vector>

#include "Fingerprint.h"
#include "Utils.h"

/// A synthetic fingerprint generator

class QFGenerator
{
    // 1 second t-f spectrum interval
    static const int DF = 558;   // Frequency range (Kmax-Kmin+1)
    static const int DT = 73;    // Time units (~1s)
    static const int spec_size= sizeof(int) * 558 * 73;

    int spec[DT][DF];

    Audioneex::Utils::rng::natural<int>          m_rand_number;
    std::vector<Audioneex::QLocalFingerprint_t>  m_QF;

public:

    std::vector<Audioneex::QLocalFingerprint_t>& Generate()
    {
        // Generate synthetic fingerprint

        int toffset = 0;

        // Generate a duration for the fingerprint (in sec)
        // Note: random durations are generated from a uniform pdf
        //       probably a normal distribution is more realistic ?
        int dur = m_rand_number(240, 420);

        // convert duration in time units
        int tdur = dur / Audioneex::Pms::dt;

        m_QF.clear();

        // generate LFs
        while(toffset<=tdur)
        {
            // reset t-f interval
            memset(spec, 0, spec_size);

            // number of LFs in the t-f interval
            // NOTE: With the current parameters the algorithm
            //       generates about 15-25 LFs per second.
            int nlf = m_rand_number(15, 25);

            // Generate random LFs points in the t-f spectrum interval
            // starting at current time toffset.
            for(int i=1; i<=nlf; i++)
			{
                bool exists = false;
				
                while(!exists)
				{
                    int T = m_rand_number(0, DT-1);
                    int F = m_rand_number(0, DF-1);
					
                    if(spec[T][F]==0){
                       spec[T][F]=1;
                       exists=true;
                    }
                }
            }

            // create QLFs
            for(int t=0; t<DT; t++)
			{
                for(int k=0; k<DF; k++)
				{
                    if(spec[t][k]!=0)
					{
                       Audioneex::QLocalFingerprint_t QLF;
                       QLF.T = toffset + t;
                       QLF.F = Audioneex::Pms::Kmin + k;
                       QLF.W = m_rand_number(0, Audioneex::Pms::Kmed-1);
                       QLF.E = m_rand_number(50, 255);
                       m_QF.push_back(QLF);
                    }
                }
            }

            // next t-f interval
            toffset += DT;
        }

        return m_QF;
    }
};

#endif
