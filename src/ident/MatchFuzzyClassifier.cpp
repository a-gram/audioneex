/*
   Copyright (c) 2014, Audioneex.com.
   Copyright (c) 2014, Alberto Gramaglia.
	
   This source code is part of the Audioneex software package and is
   subject to the terms and conditions stated in the accompanying license.
   Please refer to the license document provided with the package
   for more information.
	
*/

#include <algorithm>
#include <vector>
#include <cstring>

#include "common.h"
#include "audioneex.h"
#include "MatchFuzzyClassifier.h"

#ifdef TESTING
 #include "gnuplot.h"
#endif


Audioneex::MatchFuzzyClassifier::MatchFuzzyClassifier()
{
    SetMode(EASY_IDENTIFICATION);
}

int Audioneex::MatchFuzzyClassifier::Process(double Hu, double dT)
{
    double uc[Nc]     = { };
    double Rc[Nc][Nr] = { };

	std::memset(Rc, 0, Nc*Nr*sizeof(double));

	// Evaluate rules

    // Rules for LISTENING classification

    // if CONF is HIGH and CDUR is SHORT => RES is LISTENING
    Rc[LISTENING][0] = std::min<double>(uCONF_HIGH(Hu), uCDUR_SHORT(dT));
    // if CONF is MED and CDUR is SHORT => RES is LISTENING
    Rc[LISTENING][1] = std::min<double>(uCONF_MED(Hu), uCDUR_SHORT(dT));
    // if CONF is MED and CDUR is MED => RES is LISTENING
    Rc[LISTENING][2] = std::min<double>(uCONF_MED(Hu), uCDUR_MED(dT));
    // if CONF is LOW and CDUR is SHORT => RES is LISTENING
    Rc[LISTENING][3] = std::min<double>(uCONF_LOW(Hu), uCDUR_SHORT(dT));
    // if CONF is LOW and CDUR is MED => RES is LISTENING
    Rc[LISTENING][4] = std::min<double>(uCONF_LOW(Hu), uCDUR_MED(dT));

    // Rules for IDENTIFIED classification

    // if CONF is HIGH and CDUR is MED => RES is IDENTIFIED
    Rc[IDENTIFIED][0] = std::min<double>(uCONF_HIGH(Hu), uCDUR_MED(dT));
    // if CONF is HIGH and CDUR is LONG => RES is IDENTIFIED
    Rc[IDENTIFIED][1] = std::min<double>(uCONF_HIGH(Hu), uCDUR_LONG(dT));

    // Rules for SOUNDS_LIKE classification

    // if CONF is MED and CDUR is LONG => RES is SOUNDS_LIKE
    Rc[SOUNDS_LIKE][0] = std::min<double>(uCONF_MED(Hu), uCDUR_LONG(dT));

    // Rules for UNIDENTIFIED classification

    // if CONF is LOW and CDUR is LONG => RES is UNIDENTIFIED
    Rc[UNIDENTIFIED][0] = std::min<double>(uCONF_LOW(Hu), uCDUR_LONG(dT));

	// Maximum aggregation

    uc[LISTENING] = *std::max_element(Rc[LISTENING], Rc[LISTENING]+Nr);
    uc[UNIDENTIFIED] = *std::max_element(Rc[UNIDENTIFIED], Rc[UNIDENTIFIED]+Nr);
    uc[SOUNDS_LIKE] = *std::max_element(Rc[SOUNDS_LIKE], Rc[SOUNDS_LIKE]+Nr);
    uc[IDENTIFIED] = *std::max_element(Rc[IDENTIFIED], Rc[IDENTIFIED]+Nr);

	int clabel = std::distance(uc, std::max_element(uc, uc+Nc));

    return clabel;
}


void Audioneex::MatchFuzzyClassifier::SetCutPoints(int var, std::vector<double> &points)
{

//	if(points.size() != Np)
//		throw std::runtime_error("Invalid number of cut points.");

//	if(var<0 || var>=Nu)
//		throw std::runtime_error("Invalid variable number.");

//	for(size_t i=0; i<Np; i++)
//		ux[var][i] = points[i];

//	if(mDebug){
//       DEBUG_MSG( (var?"MD":"MA") << " cut points: ")
//		   for(size_t i=0; i<points.size(); i++)
//			   DEBUG_MSG( "  x" << i << "=" << points[i])
//	   //Plot();
//	}

}

void Audioneex::MatchFuzzyClassifier::SetMode(Audioneex::eIdentificationMode mode)
{
    if(mode == EASY_IDENTIFICATION){
        // Set variable CONF
        ux[CONF][CONF_LOW].x2 = 0.55;
        ux[CONF][CONF_LOW].x3 = 0.65;
        ux[CONF][CONF_MED].x1 = 0.60;
        ux[CONF][CONF_MED].x2 = 0.70;
        ux[CONF][CONF_MED].x3 = 0.80;
        ux[CONF][CONF_HIGH].x1 = 0.75;
        ux[CONF][CONF_HIGH].x2 = 0.90;

        // Set variable CDUR
        ux[CDUR][CDUR_SHORT].x2 = 1.5;
        ux[CDUR][CDUR_SHORT].x3 = 3;
        ux[CDUR][CDUR_MED].x1 = 2;
        ux[CDUR][CDUR_MED].x2 = 10;
        ux[CDUR][CDUR_MED].x3 = 22;
        ux[CDUR][CDUR_LONG].x1 = 17.5;
        ux[CDUR][CDUR_LONG].x2 = 20;
    }
    else if(mode == STRICT_IDENTIFICATION){
        // Set variable CONF
        ux[CONF][CONF_LOW].x2 = 0.55;
        ux[CONF][CONF_LOW].x3 = 0.65;
        ux[CONF][CONF_MED].x1 = 0.60;
        ux[CONF][CONF_MED].x2 = 0.70;
        ux[CONF][CONF_MED].x3 = 0.92;
        ux[CONF][CONF_HIGH].x1 = 0.875;
        ux[CONF][CONF_HIGH].x2 = 0.95;

        // Set variable CDUR
        ux[CDUR][CDUR_SHORT].x2 = 2;
        ux[CDUR][CDUR_SHORT].x3 = 5;
        ux[CDUR][CDUR_MED].x1 = 2.8;
        ux[CDUR][CDUR_MED].x2 = 12;
        ux[CDUR][CDUR_MED].x3 = 19.2;
        ux[CDUR][CDUR_LONG].x1 = 15;
        ux[CDUR][CDUR_LONG].x2 = 20;
    }
}


#ifdef PLOTTING_ENABLED

#include <strstream>

void Audioneex::MatchFuzzyClassifier::Plot(const std::string &path)
{
	std::ostringstream ss;

	Gnuplot plot1;

    plot1.setPngPlotDevice(path+"fuzzy_CONF.png");

	ss << "max(x,y) = (x > y) ? x : y" << std::endl;

	ss << "set samples 1001" << std::endl;

    ss << "uCONF_LOW(x) = (x <= " << ux[CONF][CONF_LOW].x2 << ") ? "
	   << "1 : "
       << "max( 0, (" << ux[CONF][CONF_LOW].x3 
	   << "-x)/(" << ux[CONF][CONF_LOW].x3 << "-" 
	   << ux[CONF][CONF_LOW].x2 << ") )" << std::endl;

    ss << "uCONF_MED(x) = (x <= " << ux[CONF][CONF_MED].x2 << ") ? "
       << "max( 0, (x-" << ux[CONF][CONF_MED].x1 
	   << ")/(" << ux[CONF][CONF_MED].x2 << "-" 
	   << ux[CONF][CONF_MED].x1 << ") ) : "
       << "max( 0, (" << ux[CONF][CONF_MED].x3 
	   << "-x)/(" << ux[CONF][CONF_MED].x3 << "-" 
	   << ux[CONF][CONF_MED].x2 << ") )" << std::endl;

    ss << "uCONF_HIGH(x) = (x < " << ux[CONF][CONF_HIGH].x2 << ") ? "
       << "max( 0, (x-" << ux[CONF][CONF_HIGH].x1 << ")/(" 
	   << ux[CONF][CONF_HIGH].x2 << "-" << ux[CONF][CONF_HIGH].x1 << ") ) : "
	   << "1" << std::endl;

	plot1.cmd( ss.str() );
    plot1.set_title("Confidence");
    plot1.set_xrange(0, 1.5);
    plot1.set_yrange(0, 1.5);
    plot1.set_xlabel("p(match)");
	plot1.set_ylabel("u(x)");
    plot1.plot_equation("uCONF_LOW(x), uCONF_MED(x), uCONF_HIGH(x)");

	ss.str("");


	Gnuplot plot2;

    plot2.setPngPlotDevice(path+"fuzzy_CDUR.png");

	ss << "max(x,y) = (x > y) ? x : y" << std::endl;

	ss << "set samples 1001" << std::endl;

    ss << "uCDUR_SHORT(x) = (x <= " << ux[CDUR][CDUR_SHORT].x2 << ") ? "
	   << "1 : "
       << "max( 0, (" << ux[CDUR][CDUR_SHORT].x3 << "-x)/(" 
	   << ux[CDUR][CDUR_SHORT].x3 << "-" 
	   << ux[CDUR][CDUR_SHORT].x2 << ") )" << std::endl;

    ss << "uCDUR_MED(x) = (x <= " << ux[CDUR][CDUR_MED].x2 << ") ? "
       << "max( 0, (x-" << ux[CDUR][CDUR_MED].x1 << ")/(" 
	   << ux[CDUR][CDUR_MED].x2 << "-" << ux[CDUR][CDUR_MED].x1 << ") ) : "
       << "max( 0, (" << ux[CDUR][CDUR_MED].x3 
	   << "-x)/(" << ux[CDUR][CDUR_MED].x3 << "-" 
	   << ux[CDUR][CDUR_MED].x2 << ") )" << std::endl;

    ss << "uCDUR_LONG(x) = (x < " << ux[CDUR][CDUR_LONG].x2 << ") ? "
       << "max( 0, (x-" << ux[CDUR][CDUR_LONG].x1 << ")/(" 
	   << ux[CDUR][CDUR_LONG].x2 << "-" << ux[CDUR][CDUR_LONG].x1 << ") ) : "
	   << "1" << std::endl;

	plot1.cmd( ss.str() );
    plot1.set_title("Listening duration");
    plot1.set_xrange(0, 25);
	plot1.set_yrange(0, 1.5);
	plot1.set_xlabel("seconds");
	plot1.set_ylabel("u(x)");
    plot1.plot_equation("uCDUR_SHORT(x), uCDUR_MED(x), uCDUR_LONG(x)");

}

#endif


