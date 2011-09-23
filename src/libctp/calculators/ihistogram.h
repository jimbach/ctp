/*
 * Copyright 2009-2011 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _HIST_INTEGRALS_H
#define	_HIST_INTEGRALS_H

#include <votca/ctp/qmpair.h>
#include <votca/ctp/paircalculator.h>
#include <votca/tools/histogramnew.h>

namespace votca { namespace ctp {


/**
	\brief Histogram of transfer integrals of neighbor list pairs

Callname: ihistogram   
*/
class CalcHistIntegrals : public PairCalculator
{
public:
    CalcHistIntegrals() {};
    ~CalcHistIntegrals() {};

    const char *Description() { return "Histogram of transfer integrals of neighbor list pairs"; }

    void Initialize(QMTopology *top, Property *options);
    void EvaluatePair(QMTopology *top, QMPair *pair);
    void EndEvaluate(QMTopology *top);

private:
    HistogramNew histogram;
    double _min, _max;
    int _nbins;
    string _outfile;
};

inline void CalcHistIntegrals::Initialize(QMTopology *top, Property *options){
    _min = options->get("options.ihistogram.min").as<double>();
    _max = options->get("options.ihistogram.max").as<double>();
    _nbins = options->get("options.ihistogram.nbins").as<int>();
    _outfile = options->get("options.ihistogram.file").as<string>();

    histogram.Initialize(_min,_max,_nbins);
}

inline void CalcHistIntegrals::EndEvaluate(QMTopology *top){
    cout << "Writing Distribution of Log10(J_eff^2) to file " << _outfile << endl;
    histogram.data().Save(_outfile);
}

inline void CalcHistIntegrals::EvaluatePair(QMTopology *top, QMPair *pair){
    double logJ2eff = log10(pair->calcJeff2());
    histogram.Process( logJ2eff );
}

}}

#endif	/* _HIST_INTEGRALS_H */