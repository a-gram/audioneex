/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifdef TESTING

#ifndef TEST_H
#define TEST_H

#include <list>
#include <vector>
#include <string>
#include <ctime>
#include <map>
#include <fstream>

#include "common.h"

#include "Parameters.h"
#include "Fingerprint.h"
#include "Matcher.h"
#include "Utils.h"

#ifdef PLOTTING_ENABLED
 #include "gnuplot.h"
#endif

// Put testing-related defines here
#define DEBUG_TF_COHERENCE_ENABLED 0


namespace Audioneex
{

class Tester
{
    typedef std::list<std::pair< std::string, std::vector<float> > > ListDescriptorLabel;
    typedef std::list<std::vector<float> > ListDescriptorROI;

    std::vector< std::vector<float> >  SPECTRUM;
    std::vector< std::vector<float> >  DESCRIPTORS_POIS;
    ListDescriptorROI                  DESCRIPTORS_ROIS;
    ListDescriptorLabel                DESCRIPTORS_LABELS;

    std::vector<std::string> ProfileCodeBlockName;
    std::vector<float>       ProfileCodeBlock;
    std::vector<float>       ProfileStartTime;
    std::vector<int>         ProfileIterations;

 public:

	Tester()
	{
	    ProfileCodeBlock.resize(25);
	    ProfileIterations.resize(25);
	    ProfileStartTime.resize(25);
        ProfileCodeBlockName.resize(25, std::string("-"));
	}
	
	// ----------------------------------------------------------------------------
	
	void Plot2DBox(float left, float bottom, float right, float top)
	{
	    std::vector<float> dsc(4);
	    dsc[0]=left; dsc[1]=bottom; dsc[2]=right; dsc[3]=top;
	    DESCRIPTORS_ROIS.push_back(dsc);
	}
	
	// ----------------------------------------------------------------------------
	
	void Plot2DLabel(std::string label, float x, float y)
	{
	    std::pair< std::string, std::vector<float> > lab;
	    lab.first = label;
	    lab.second.resize(2);
	    lab.second[0] = x; lab.second[1] = y;
	    DESCRIPTORS_LABELS.push_back(lab);
	}
	
	// ----------------------------------------------------------------------------
	
	void ResetSpectrumPlotting()
	{
	    DESCRIPTORS_POIS.clear();
	    DESCRIPTORS_ROIS.clear();
	    DESCRIPTORS_LABELS.clear();
	    SPECTRUM.clear();
	}
	
	// ----------------------------------------------------------------------------
	
	void PlotSimilarityMatrix(const std::vector< std::vector<float> > &Mc, float Mcmax, const std::string &name)
	{
	#ifdef PLOTTING_ENABLED
	    std::vector<float> px(Mc.size());    for(size_t i=0; i<px.size(); ++i) px[i]=i+1;
	    std::vector<float> py(Mc[0].size()); for(size_t i=0; i<py.size(); ++i) py[i]=i;
	
	    Gnuplot plot;
	    plot.setPngPlotDevice(name, 1200);
	    plot.set_cbrange(0,Mcmax);
	    //plot.cmd("set tmargin at screen 0.15");
	    plot.plot_map(Mc,px,py,"Similarity Matrix",false);
	#endif
	}
	
	// ----------------------------------------------------------------------------
	
	void PlotHistogram(const std::vector<float> &histo, const std::string &name)
	{
	#ifdef PLOTTING_ENABLED
	    Gnuplot plot;
	    plot.setPngPlotDevice(name);
	    plot.plot_histogram(histo);
	#endif
	}
	
	// ----------------------------------------------------------------------------
	
	void Dump(const Fingerprint_t &fp)
	{
	    DEBUG_MSG("ID: "<<fp.ID)
	    DEBUG_MSG("Nlfs : "<<fp.LFs.size())
	    //DEBUG_MSG("Metadata.Title : "<<fp.Metadata.Title.c_str())
	    //DEBUG_MSG("Metadata.Artist : "<<fp.Metadata.Artist.c_str())
	    //DEBUG_MSG("Metadata.Duration : "<<fp.Metadata.Duration)
	}
	
	// ----------------------------------------------------------------------------
	
	void Dump(const LocalFingerprint_t &lf)
	{
	    DEBUG_MSG("ID: "<<lf.ID)
	    DEBUG_MSG("T : "<<lf.T)
	    DEBUG_MSG("F : "<<lf.F)
	    DEBUG_MSG("D : ")
	    for(unsigned char v : lf.D) {
	       DEBUG_MSG(int(v)<<" ")
		}
	    DEBUG_MSG("")
	}
	
	// ----------------------------------------------------------------------------
	
    void Dump(const MatchResults_t &res)
	{
	    std::string str;
	
	    for(const hashtable_Qi::value_type &e : res.Top_K){
	        int score = e.first;
	        str.append("*** score: ")
	           .append(std::to_string(score))
	           .append("\t");
            std::list<int> tie_list = e.second;
	        for(int Qi : tie_list){
	            str.append(" Q").append(std::to_string(Qi));
	        }
	        str.append("\n");
	    }
	
	    DEBUG_MSG(str)
	}
	
	// ----------------------------------------------------------------------------
	
	void Dump(const hashtable_Qhisto &table)
	{
	    std::string str;
	    for(const hashtable_Qhisto::value_type &e : table){
	        int score = e.first;
	        str.append(" score: ")
	           .append(std::to_string(score))
	           .append("\t");
            const std::list<Qhisto_t> &tie_list = e.second;
	        for(Qhisto_t H : tie_list){
	            str.append(" Q").append(std::to_string(H.Qi));
	        }
	        str.append("\n");
	    }
	    DEBUG_MSG(str)
	}
	
	// ----------------------------------------------------------------------------
	
	void Dump(const hashtable_Qi &table)
	{
	    std::string str;
	    for(const hashtable_Qi::value_type &e : table){
	        int score = e.first;
            const std::list<int> &tie_list = e.second;
	        for(int Qi : tie_list){
	            str.append(" Q").append(std::to_string(Qi));
	        }
	        str.append("\tscore: ")
	           .append(std::to_string(score))
	           .append("\n");
	    }
	    DEBUG_MSG(str)
	}
	
	// ----------------------------------------------------------------------------
	
	void Dump(const hashtable_Qc &table)
	{
	    std::string str;
	    for(const hashtable_Qc::value_type &e : table){
	        int Qi = e.first;
            const Ac_t &H = e.second;
	        str.append(" Q").append(std::to_string(Qi));
	        str.append("\tscore: ")
               .append(std::to_string( H.Ac ))
	           .append("\n");
	    }
	    DEBUG_MSG(str)
	}
	
	// ----------------------------------------------------------------------------
	
	void Dump(const std::vector< std::vector<float> > &mat)
	{
	    std::string str;
        for(const std::vector<float> &col : mat) {
	         for(float row : col) {
	              str.append(std::to_string(row)).append("   ");
			 }
		}
	    DEBUG_MSG(str)
	}
	
	// ----------------------------------------------------------------------------
	
	void AddToPlotSpectrum(const Fingerprint &fp)
	{
	    // add fingerprint spectrum to plot spectrum
	
	    SPECTRUM.insert(SPECTRUM.end(), fp.m_Spectrum.begin(), fp.m_Spectrum.end());
	
	    // add fingerprint POIs and descriptors to plot spectrum
	
	    std::vector<float> point(2);
	
	    for(const LocalFingerprint_t &lf : fp.m_LF)
		{
	        int m = lf.T;
	        int k = lf.F;
	        point[0] = m * Pms::dt;
	        point[1] = k * Pms::df;
	        DESCRIPTORS_POIS.push_back(point);
	        Plot2DBox(m*Pms::dt-Pms::dTNp/2, (k*Pms::df)-Pms::dFNp/2, m*Pms::dt+Pms::dTNp/2, (k*Pms::df)+Pms::dFNp/2);
	        Plot2DBox(m*Pms::dt-Pms::dTNp/2, ((k*Pms::df)+Pms::dFNp/2)-Pms::dFWc, (m*Pms::dt-Pms::dTNp/2)+Pms::dTWc, (k*Pms::df)+Pms::dFNp/2);
	        Plot2DBox(m*Pms::dt-Pms::dTNp/2+((Pms::bt/100.f)*Pms::dTWc), ((k*Pms::df)+Pms::dFNp/2)-Pms::dFWc, (m*Pms::dt-Pms::dTNp/2)+Pms::dTWc+((Pms::bt/100.f)*Pms::dTWc), (k*Pms::df)+Pms::dFNp/2);
	        Plot2DBox(m*Pms::dt-Pms::dTNp/2, ((k*Pms::df)+Pms::dFNp/2)-Pms::dFWc-((Pms::bf/100.f)*Pms::dFWc), (m*Pms::dt-Pms::dTNp/2)+Pms::dTWc, (k*Pms::df)+Pms::dFNp/2-((Pms::bf/100.f)*Pms::dFWc));
	        Plot2DLabel(std::to_string(lf.ID), m*Pms::dt+Pms::dTNp/10, Pms::Fmin+k*Pms::df+Pms::dFNp/10);
	    }
	
	}
	
	// ----------------------------------------------------------------------------
	
    void PlotSpectrum(const std::string &name, size_t width=1200)
	{
	#ifdef PLOTTING_ENABLED
	    int Kmin = floor(Pms::windowSize*Pms::Fmin/Pms::Fs);
	    int Kmax = floor(Pms::windowSize*Pms::Fmax/Pms::Fs);
	    int nbins = Kmax - Kmin + 1;
	
	    std::vector< std::vector<float> > dspectrum;
	    std::vector<float> dframe(nbins);
	
	    for(int f=0; f<SPECTRUM.size(); f++){
	        for(int k=Kmin; k<Kmax; k++)
	            dframe[k-Kmin] = sqrt(SPECTRUM[f][k]);
	        dspectrum.push_back(dframe);
	    }
	
	    // compute time and frequency axes
	
	    std::vector<float> taxis;
	    std::vector<float> faxis;
	
	    for(size_t i=0; i<SPECTRUM.size(); i++)
	        taxis.push_back(i*Pms::dt);
	
	    for(size_t i=Kmin; i<=Kmax; i++)
	        faxis.push_back(i*Pms::df);
	
	
	    Gnuplot gp;
	    gp.setPngPlotDevice(name+".png",width);
	    gp.plot_map(dspectrum, taxis, faxis, "Spectrogram");
	    gp.plot_2D_points(DESCRIPTORS_POIS);
	    gp.plot_2D_boxes(DESCRIPTORS_ROIS);
	    gp.plot_2D_text(DESCRIPTORS_LABELS);
	#endif
	}
	
	// ----------------------------------------------------------------------------
	
	void PlotProfiles(const std::string &file, bool auto_y_range, float y_range)
	{
	#ifdef PLOTTING_ENABLED
	    if(!auto_y_range && y_range<=0){
	       WARNING_MSG("Invalid y range value. Enabling auto range.")
	       auto_y_range = true;
	    }
	
	    // Check whether we have profiles
	    bool haveProfiles = false;
	    float ymax = -1;
	
	    for(size_t i=0; i<ProfileIterations.size(); i++){
	        if(ProfileIterations[i]) haveProfiles=true;
	        ymax = std::max<float>(ymax, ProfileCodeBlock[i]);
	    }
	
	    if(ymax<=0) ymax=5;
	
	    if(haveProfiles){
	       Gnuplot plot;
	       plot.setPngPlotDevice(file);
	
	       if(auto_y_range)
	          plot.set_yrange(0,ymax*1.1);
	       else
	          plot.set_yrange(0, y_range);
	
           std::vector< std::vector<float> > profileGroups;
	       profileGroups.push_back(ProfileCodeBlock);
	
	       plot.plot_histogram(profileGroups, ProfileCodeBlockName);
	    }
	#endif
	}
	
	// ----------------------------------------------------------------------------
	
	void PlotProfilesMean(const std::string &file, bool auto_y_range, float y_range)
	{
	#ifdef PLOTTING_ENABLED
	    if(!auto_y_range && y_range<=0){
	       WARNING_MSG("Invalid y range value. Enabling auto range.")
	       auto_y_range = true;
	    }
	
	    // Check whether we have profiles
	    bool haveProfiles = false;
	    float ymax = -1;
	
	    for(size_t i=0; i<ProfileIterations.size(); i++)
	        if(ProfileIterations[i]) haveProfiles=true;   
	
	    if(haveProfiles){
	
	       // Compute mean times
	       std::vector<float> ProfileMeans(ProfileCodeBlock.size());
	
	       for(size_t i=0; i<ProfileCodeBlock.size(); i++)
	           if(ProfileIterations[i]){
	              DEBUG_MSG("PROFILING: block="<<i<<", total time="<<ProfileCodeBlock[i]<<", iter="<<ProfileIterations[i])
	              ProfileMeans[i] = ProfileCodeBlock[i] / ProfileIterations[i];
	              ymax = std::max<float>(ymax, ProfileMeans[i]);
	           }
	
	       if(ymax<=0) ymax=5;
	
	       Gnuplot plot;
	       plot.setPngPlotDevice(file);
	
	       if(auto_y_range)
	          plot.set_yrange(0,ymax*1.1);
	       else
	          plot.set_yrange(0, y_range);
	
	
	       std::vector< std::vector<float> > profileGroups;
	       profileGroups.push_back(ProfileMeans);
	
	       plot.plot_histogram(profileGroups, ProfileCodeBlockName);
	    }
	#endif
	}

    void SaveProfiles(const std::string &file)
    {
        std::ofstream pfile(file);
        if(!pfile.is_open()) throw std::logic_error("Couldn't open "+file);
        for(size_t i=0; i<ProfileCodeBlock.size(); i++)
            if(ProfileIterations[i])
               pfile << ProfileCodeBlockName[i] << "\t" << ProfileCodeBlock[i] << std::endl;
    }

	void StartProfile(int codeBlock, std::string codeBlockName){
	    ProfileCodeBlockName[codeBlock] = codeBlockName;
	    ProfileStartTime[codeBlock] = static_cast<float>(clock()) / CLOCKS_PER_SEC;
	}
	
	void StopProfile(int codeBlock){
	    float now = static_cast<float>(clock()) / CLOCKS_PER_SEC;
	    ProfileCodeBlock[codeBlock] += (now - ProfileStartTime[codeBlock]);
	    ProfileIterations[codeBlock]++;
	}
	
	void ResetProfiles(){
	    std::fill(ProfileCodeBlock.begin(), ProfileCodeBlock.end(), 0.0f);
	    std::fill(ProfileStartTime.begin(), ProfileStartTime.end(), 0.0f);
	    std::fill(ProfileIterations.begin(), ProfileIterations.end(), 0);
	    std::fill(ProfileCodeBlockName.begin(), ProfileCodeBlockName.end(), std::string("-"));
	}

};


}// end namespace Audioneex

#endif // TEST_H

#endif // #ifdef TESTING
