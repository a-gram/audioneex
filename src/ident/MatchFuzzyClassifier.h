/*
   Copyright (c) 2014, Audioneex.com.
   Copyright (c) 2014, Alberto Gramaglia.
	
   This source code is part of the Audioneex software package and is
   subject to the terms and conditions stated in the accompanying license.
   Please refer to the license document provided with the package
   for more information.
	
*/

#ifndef MATCHFUZZYCLASSIFIER_H
#define MATCHFUZZYCLASSIFIER_H

#include <vector>

namespace Audioneex
{

class MatchFuzzyClassifier
{
    /// Fuzzy variables
	enum{
        CONF,    // Confidence of match
        CDUR     // Listening duration
	};

    /// Fuzzy values for CONF
	enum{
        CONF_LOW,
        CONF_MED,
        CONF_HIGH
	};

    /// Fuzzy values for CDUR
    enum{
        CDUR_SHORT,
        CDUR_MED,
        CDUR_LONG
    };

	static const int Nu = 2;    /// Number of fuzzy variables
    static const int Nv = 3;    /// Max number of fuzzy values (per variable)
    static const int Nc = 4;    /// Number of class labels (output variables)
	static const int Nr = 9;    /// Number of rules

    // Triangle-shaped membership functions (with saturation at the edges)
    struct Triple{
        double x1, x2, x3;
    };

    Triple ux[Nu][Nv];


    /// Membership function for fuzzy set CONF_LOW
    /// var = fuzzy variable identifier, such that u(var)=>LOW
    inline double uCONF_LOW(double Hu);

    /// Membership function for fuzzy set CONF_MED
    inline double uCONF_MED(double Hu);

    /// Membership function for fuzzy set CONF_HIGH
    inline double uCONF_HIGH(double Hu);

    // Membership functions for variable CDUR have the same shape as CONF's

    /// Membership function for fuzzy set CDUR_SHORT
    inline double uCDUR_SHORT(double dT);

    /// Membership function for fuzzy set CDUR_MED
    inline double uCDUR_MED(double dT);

    /// Membership function for fuzzy set CDUR_LONG
    inline double uCDUR_LONG(double dT);


 public:

    /// Output Class labels
	enum{
        UNIDENTIFIED,  ///< There is no clear evidence of a match
        SOUNDS_LIKE,   ///< There is some evidence of a match
        IDENTIFIED,    ///< There is a clear evidence of a match
        LISTENING      ///< Not enough evidence. More needed.
	};

    MatchFuzzyClassifier();

    /// Performs classification of given input
    /// Hu = mean confidence
    /// dT = confidence duration
    int Process(double Hu, double dT);

	/// Set cut points for variable 'var'.
	void SetCutPoints(int var, std::vector<double> &points);

    /// Set classification mode
    void SetMode(Audioneex::eIdentificationMode mode);

#ifdef PLOTTING_ENABLED
    void Plot(const std::string &path);
#endif

};


///////////////////////////////////////////////////////////////////////////////
///                               inlines                                   ///
///////////////////////////////////////////////////////////////////////////////


double MatchFuzzyClassifier::uCONF_LOW(double Hu){
    double x2 = ux[CONF][CONF_LOW].x2;
    double x3 = ux[CONF][CONF_LOW].x3;
    return (Hu <= x2) ? 1 : std::max<double>( 0.0, (x3-Hu)/(x3-x2) );
}

double MatchFuzzyClassifier::uCONF_MED(double Hu){
    double x1 = ux[CONF][CONF_MED].x1;
    double x2 = ux[CONF][CONF_MED].x2;
    double x3 = ux[CONF][CONF_MED].x3;
    return (Hu <= x2) ? std::max<double>( 0.0, (Hu-x1)/(x2-x1) ) :
                        std::max<double>( 0.0, (x3-Hu)/(x3-x2) ) ;
}

double MatchFuzzyClassifier::uCONF_HIGH(double Hu){
    double x1 = ux[CONF][CONF_HIGH].x1;
    double x2 = ux[CONF][CONF_HIGH].x2;
    return (Hu >= x2) ? 1 : std::max<double>( 0.0, (Hu-x1)/(x2-x1) );
}

// Membership functions for variable CDUR have the same shape as CONF's

double MatchFuzzyClassifier::uCDUR_SHORT(double dT){
    double x2 = ux[CDUR][CDUR_SHORT].x2;
    double x3 = ux[CDUR][CDUR_SHORT].x3;
    return (dT <= x2) ? 1 : std::max<double>( 0.0, (x3-dT)/(x3-x2) );
}

double MatchFuzzyClassifier::uCDUR_MED(double dT){
    double x1 = ux[CDUR][CDUR_MED].x1;
    double x2 = ux[CDUR][CDUR_MED].x2;
    double x3 = ux[CDUR][CDUR_MED].x3;
    return (dT <= x2) ? std::max<double>( 0.0, (dT-x1)/(x2-x1) ) :
                        std::max<double>( 0.0, (x3-dT)/(x3-x2) ) ;
}

double MatchFuzzyClassifier::uCDUR_LONG(double dT){
    double x1 = ux[CDUR][CDUR_LONG].x1;
    double x2 = ux[CDUR][CDUR_LONG].x2;
    return (dT >= x2) ? 1 : std::max<double>( 0.0, (dT-x1)/(x2-x1) );
}

}// end namespace Audioneex

#endif
