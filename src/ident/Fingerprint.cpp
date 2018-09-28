/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <vector>

#include "common.h"
#include "Fingerprint.h"
#include "Utils.h"

#ifdef TESTING
 #include "Tester.h"
#endif


TEST_HERE( namespace { Audioneex::Tester TEST; } )


Audioneex::Fingerprint::Fingerprint(size_t bufferSize)
:
    m_OSBuffer (bufferSize, Pms::Fs, Pms::Ca, 0),
    m_OSWindow (Pms::OrigWindowSize, Pms::Fs, Pms::Ca, 0),
    m_fftFrame (Pms::OrigWindowSize + 1),
    m_LID      (0),
    m_DeltaT   (0)
{
    // Set up FFT processor
    m_AudioProcessor.SetFFT(new FFT(Pms::OrigWindowSize, Pms::zeroPadFactor));
}

// ----------------------------------------------------------------------------

void Audioneex::Fingerprint::Process(AudioBlock<float> &audio, bool flush)
{
    assert(audio.SampleRate() == Pms::Fs);
    assert(audio.Channels() == Pms::Ca);

    // Reset structures from previous processing
    m_Spectrum.clear();
    m_LF.clear();
    m_Peak.clear();

#ifdef PLOTTING_ENABLED
    m_POI.clear();
#endif

    // Check whether we have enough audio data to extract the
    // fingerprints. The minimum value depends on the maximum
    // size of the windows used in the fingerprinting (Wp, Np)
    if(audio.Duration() >= 0.5)
    {
       // This reallocation should never happen, but just in case...
       if( m_OSBuffer.Capacity() < audio.Size() + Pms::OrigWindowSize ){
          WARNING_MSG("O&S buffer reallocation.");
          m_OSBuffer = AudioBlock<float>(audio.Size() + Pms::OrigWindowSize,
                                         Pms::Fs,
                                         Pms::Ca,
                                         0);
       }

       ComputeSpectrum(audio, flush);
       FindPeaks();
       ExtractPOI();
       ComputeDescriptors();

       // time-traslate the current snippet
       m_DeltaT += m_Spectrum.size();
    }
    else{
        // Ignore data ?
    }
}

// ----------------------------------------------------------------------------

void Audioneex::Fingerprint::Reset()
{
    m_OSBuffer.Resize(0);
    m_OSWindow.Resize(0);
    m_LID = 0;
    m_DeltaT = 0;
}

// ----------------------------------------------------------------------------

void Audioneex::Fingerprint::SetBufferSize(size_t size)
{
    m_OSBuffer = AudioBlock<float>(size + Pms::OrigWindowSize, Pms::Fs, Pms::Ca, 0);
}

// ----------------------------------------------------------------------------

void Audioneex::Fingerprint::ComputeSpectrum(AudioBlock<float> &audio, bool flush)
{
    // Prepend the last O&S window to current audio block
    m_OSBuffer.Append(m_OSWindow).Append(audio);

    m_OSWindow.Resize(Pms::OrigWindowSize);

    // Read the input block in an overlap windowed fashion
    for(size_t wstart=0; m_OSWindow.Size()==Pms::OrigWindowSize; wstart+=Pms::hopSize)
    {
        m_OSBuffer.GetSubBlock(wstart, Pms::OrigWindowSize, m_OSWindow);

        // if we have a complete FFT window, process it
        if(m_OSWindow.Size() == Pms::OrigWindowSize){
           m_AudioProcessor.FFT_Transform(m_OSWindow, m_fftFrame, FFT::EnergySpectrum);
           m_Spectrum.push_back(m_fftFrame);
        }
    }

    // Reset the O&S buffer
    m_OSBuffer.Resize(0);

    // If the flush flag is set, then any residual data in the O&S window
    // must also be processed after the audio block.
    if(flush && m_OSWindow.Size() > 0)
    {
       m_OSBuffer = m_OSWindow;

       for(size_t wstart=0; m_OSWindow.Size()>0; wstart+=Pms::hopSize)
       {
           m_OSBuffer.GetSubBlock(wstart, Pms::OrigWindowSize, m_OSWindow);

           if(m_OSWindow.Size()>0){
              m_AudioProcessor.FFT_Transform(m_OSWindow, m_fftFrame, FFT::EnergySpectrum);
              m_Spectrum.push_back(m_fftFrame);
           }
       }
    }

}

// ----------------------------------------------------------------------------

void Audioneex::Fingerprint::FindPeaks()
{
    assert(m_Spectrum.size() >= 3);

    int nbins = Pms::Kmax - Pms::Kmin + 1;

    std::vector< std::vector<float> > &X = m_Spectrum;

    m_Peak.resize(X.size());
    for(size_t i=0; i<X.size(); i++)
        m_Peak[i].resize(nbins);

    // NOTE: Good values for the boosting factor (central element) are in the
    //       range [5,7]

    int H[3][3] = {{-1,-1,-1},
                   {-1, 6,-1},
                   {-1,-1,-1}};

    int a = 3;      // LBL aperture
    int rH = a/2;   // LBL radius

    float Wp = 3;   // Peak width
    int rWp = Wp/2; // Peak radius

    float y, Ep;

    // Convolve spectrum with LBL kernel.
    // Skip points too close to spectrum boundaries
    // to prevent incomplete descriptors.

    for(size_t m=Pms::rNpT; m<X.size()-Pms::rNpT; m++)
       for(size_t k=Pms::Kmin+Pms::rNpF; k<Pms::Kmax-Pms::rNpF; k++)
       {
           y=0; Ep=0;

           // compute filter output at current point
           for(size_t i=0; i<a; i++)
               for(size_t j=0; j<a; j++){
                   y += X[m-rH+i][k-rH+j] * H[i][j];
                   Ep += X[m-rH+i][k-rH+j];
               }

           // If output is positive then there is a maximum at p (possible peak).
           // Store it for post-processing.
           if(y>0)
              m_Peak[m][k-Pms::Kmin] = Ep;
       }
}

// ----------------------------------------------------------------------------

void Audioneex::Fingerprint::ExtractPOI()
{

    std::vector< std::vector<float> > &X = m_Spectrum;
    std::vector< std::vector<float> > &P = m_Peak;

#ifdef PLOTTING_ENABLED
    m_POI.resize(X.size());
    for(size_t i=0; i<X.size(); i++)
        m_POI[i].resize(P[0].size());
#endif

    bool ismax;

    // for each peak in the map perform a non-maximum suppression
    for(size_t m=0; m<P.size(); m++)
        for(size_t k=0; k<P[0].size(); k++)

            if(P[m][k] > 0) // if peak
            {
                int is,ie,js,je;

                // check for out of bound windows
                (m<Pms::rWp) ? is=0 : is=m-Pms::rWp;
                (k<Pms::rHp) ? js=0 : js=k-Pms::rHp;
                (m>=P.size()-Pms::rWp) ? ie=P.size()-1 : ie=m+Pms::rWp;
                (k>=P[0].size()-Pms::rHp) ? je=P[0].size()-1 : je=k+Pms::rHp;

                ismax=true;

                // perform non-maximum suppression
                for(size_t i=is; i<=ie && ismax; i++)
                   for(size_t j=js; j<=je && ismax; j++)
                       if(P[i][j] > P[m][k])
                          ismax=false;

                // if current peak is a local maximum, mark it as a POI
                // NOTE: The POI is marked in the spectrum by changing
                //       the sign of the bin value. It is then restored
                //       when the spectrum is scanned in ComputeDescriptors().
                //       Weird, but will save us from using another map.
                if(ismax){
                   X[m][k] *= POI_LOCATION;
#ifdef PLOTTING_ENABLED
                   m_POI[m][k] = 1;
#endif
                }
            }
}

// ----------------------------------------------------------------------------

void Audioneex::Fingerprint::ComputeDescriptors()
{

    std::vector< std::vector<float> > &X = m_Spectrum;
    std::vector< std::vector<float> > &P = m_Peak;// m_POI;

    size_t Tmax = X.size();
    size_t Fmax = X[0].size();

    for(size_t m=0; m<Tmax; m++) {
        for(size_t k=0; k<Fmax; k++) {

            if(X[m][k] < 0)
            {
                X[m][k] *= POI_LOCATION;

                // center neighborhood N(p) around the POI
                // compute origin of N(p) (in X(t,f))
                int NpoT = m - Pms::rNpT;
                int NpoF = Pms::Kmin + k - Pms::rNpF;

                // compute starting Wc center (in X(t,f))
                int WcoT = NpoT + Pms::rWcT;
                int WcoF = NpoF + Pms::rWcF;

                // the descriptor for current N(p)
                std::vector<unsigned char> D;

                unsigned char Vc=0, SH=1/*SD shift*/, csd=1;

                // Scan N(p) with overlapped windows Wc and build the descriptor

                for(int i=0; i<Pms::nWcT; i++) {
                    for(int j=0; j<Pms::nWcF; j++) {
                        // compute current scanning window's center (in X(t,f))
                        int Wc0F = WcoF + j*Pms::nsf;
                        int Wc0T = WcoT + i*Pms::nst;

                        // compute energy of current scanning window
                        float EWc = ComputeWindowEnergy(Wc0T, Wc0F, Pms::rWcT, Pms::rWcF, X);

                        // compute energy of current scanning window's k-neighbours
                        float EWcN[4] = {0};

                        // neighbour EAST
                        EWcN[0] = ComputeWindowEnergy(Wc0T + Pms::nbt + Pms::rWcT, Wc0F,
                                                      Pms::rWcT, Pms::rWcF,  X);
                        // neighbour WEST
                        EWcN[1] = ComputeWindowEnergy(Wc0T - Pms::nbt - Pms::rWcT, Wc0F,
                                                      Pms::rWcT, Pms::rWcF,  X);
                        // neighbour NORTH
                        EWcN[2] = ComputeWindowEnergy(Wc0T, Wc0F + Pms::nbf + Pms::rWcF,
                                                      Pms::rWcT, Pms::rWcF,  X);
                        // neighbour SOUTH
                        EWcN[3] = ComputeWindowEnergy(Wc0T, Wc0F - Pms::nbf - Pms::rWcF,
                                                      Pms::rWcT, Pms::rWcF,  X);

                        // Apply hysteresis
                        float TL=0.25;//0.3
                        float TLmin=2;//3

                        float LRatioMax = std::max<float>(EWc,EWcN[0]) /
                                          std::min<float>(EWc,EWcN[0]);
                        float Lmax = fabs(EWc - EWcN[0]);

                        for(int wi=1; wi<=3; wi++){
                            LRatioMax = std::max<float>(LRatioMax, std::max<float>(EWc,EWcN[wi]) /
                                                                   std::min<float>(EWc,EWcN[wi]) );
                            Lmax = std::max<float>(Lmax, fabs(EWc - EWcN[wi]));
                        }

                        if(LRatioMax > TLmin)
                        {
                            Vc += (fabs(EWc - EWcN[1])/Lmax > TL) ? ((EWc > EWcN[1]) ? 1*SH : 0) : 0;
                            Vc += (fabs(EWc - EWcN[0])/Lmax > TL) ? ((EWcN[0] > EWc) ? 2*SH : 0) : 0;
                            Vc += (fabs(EWc - EWcN[2])/Lmax > TL) ? ((EWc > EWcN[2]) ? 4*SH : 0) : 0;
                            Vc += (fabs(EWc - EWcN[3])/Lmax > TL) ? ((EWcN[3] > EWc) ? 8*SH : 0) : 0;
                        }

                        // 4-neighbour sub-descriptors are packed into bytes (4 bit at a time)
                        // so we need to process 2 windows to get a full byte. This is not necessary
                        // for 8-neighbour sub-descriptors, in which case we only need to process
                        // 1 window.
                        if(csd==1){
                           SH=16;
                           csd++;
                        }else{
                           D.push_back(Vc);
                           SH=1;
                           Vc=0;
                           csd=1;
                        }
                    }
                }

                // flush any remaining sub-descriptor
                if(SH!=1)
                   D.push_back(Vc);

                assert(D.size()*8 == Pms::IDI);

                // We have a descriptor for the current POI, now
                // create the local fingerprint
                LocalFingerprint_t lf;
                lf.ID = m_LID++;
                lf.T = m_DeltaT + m;
                lf.F = Pms::Kmin + k;
                lf.D = D;

                // Add local fingerprint to stream
                m_LF.push_back(lf);

            }// end if POI
        }
    }
}

// ----------------------------------------------------------------------------

// compute energy within a window of X(t,f), given the window's center coordinates and its radii
float Audioneex::Fingerprint::ComputeWindowEnergy(int WoT, int WoF, int rWT, int rWF,
                                                  std::vector< std::vector<float> > &X)
{
    float EW = 0.0f;
    for(int u=WoT-rWT; u<=WoT+rWT; u++)
        for(int v=WoF-rWF; v<=WoF+rWF; v++)
            if(u>=0 && u<X.size() && v>=0 && v<X[0].size()) // check that point is within spectrum bounds
               EW += std::abs( X[u][v] );
    return EW;
}

// ----------------------------------------------------------------------------

float Audioneex::Fingerprint::ComputeMeanWindowEnergy(int WoT, int WoF, int rWT, int rWF,
                                                      std::vector< std::vector<float> > &X)
{
    float EW = 0.0f;
    float nW=0;
    for(int u=WoT-rWT; u<=WoT+rWT; u++){
        for(int v=WoF-rWF; v<=WoF+rWF; v++){
            if(u>=0 && u<X.size() && v>=0 && v<X[0].size()){ // check that point is within spectrum bounds
               EW += std::abs( X[u][v] );
               nW++;
            }
        }
    }
    return EW/nW;
}

