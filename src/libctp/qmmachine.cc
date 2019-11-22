/*                                                                                                                                                    
 * Copyright 2009-2018 The VOTCA Development Team (http://www.votca.org)                                                                                   
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

#include <votca/ctp/qmmachine.h>
#include <sys/stat.h>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <votca/ctp/logger.h>
//#include <votca/ctp/linalg.h>

using boost::format;

namespace votca { namespace ctp {

template<class QMPackage>
QMMachine<QMPackage>::QMMachine(XJob *job, XInductor *xind, QMPackage *qmpack,
                     Property *opt, string sfx, int nst, bool mav) 
                   : _job(job), _xind(xind), _qmpack(qmpack), _subthreads(nst),
                     _isConverged(false) {
    
    string key = sfx + ".qmmmconvg";
    if (opt->exists(key+".dR")) {
        _crit_dR = opt->get(key+".dR").as<double>();
    }
    else {
        _crit_dR = 0.01; // nm
    }
    if (opt->exists(key+".dQ")) {
        _crit_dQ = opt->get(key+".dQ").as<double>();
    }
    else {
        _crit_dQ = 0.01; // e
    }
    if (opt->exists(key+".dE_QM")) {
        _crit_dE_QM = opt->get(key+".dE_QM").as<double>();
    }
    else {
        _crit_dE_QM = 0.001; // eV
    }
    if (opt->exists(key+".dE_MM")) {
        _crit_dE_MM = opt->get(key+".dE_MM").as<double>();
    }
    else {
        _crit_dE_MM = _crit_dE_QM; // eV
    }
    if (opt->exists(key+".max_iter")) {
        _maxIter = opt->get(key+".max_iter").as<int>();
    }
    else {
        _maxIter = 32;
    }
    
    key = sfx + ".control";
    if (opt->exists(key+".split_dpl")) {
        _split_dpl = opt->get(key+".split_dpl").as<bool>();
    }
    else {
        _split_dpl = true;
    }
    if (opt->exists(key+".dpl_spacing")) {
        _dpl_spacing = opt->get(key+".dpl_spacing").as<double>();
    }
    else {
        _dpl_spacing = 1e-3;
    }
    

}


template<class QMPackage>
QMMachine<QMPackage>::~QMMachine() {
    
    vector<QMMIter*> ::iterator qit;
    for (qit = _iters.begin(); qit < _iters.end(); ++qit) {
        delete *qit;
    }
    _iters.clear();
}


template<class QMPackage>
void QMMachine<QMPackage>::Evaluate(XJob *job) {
    
    CTP_LOG(logINFO,*_log)
       << format("... dR %1$1.4f dQ %2$1.4f QM %3$1.4f MM %4$1.4f IT %5$d")
       % _crit_dR % _crit_dQ % _crit_dE_QM % _crit_dE_MM % _maxIter << flush;
    
    // FIGURE OUT CHARGE + MULTIPLICITY
    double dQ = 0.0;
    for (unsigned int i = 0; i < _job->getPolarTop()->QM0().size(); ++i) {
        dQ += _job->getPolarTop()->QM0()[i]->CalcTotQ();
    }
    int chrg = round(dQ);
    int spin = ( (chrg < 0) ? -chrg:chrg ) % 2 + 1;
    CTP_LOG(logINFO,*_log) << "... Q = " << chrg << ", 2S+1 = " << spin << flush;
    
    
    // PREPARE JOB DIRECTORY
    string jobFolder = "xjob_" + boost::lexical_cast<string>(_job->getId())
                     + "_" + _job->getTag();    
    bool created = boost::filesystem::create_directory(jobFolder);
    if (created){ 
        CTP_LOG(logINFO,*_log) << "Created directory " << jobFolder << flush;
    }
    
    
    // SET ITERATION-TIME CONSTANTS
    // TO ADJUST
    
    _qmpack->setCharge(chrg);
    _qmpack->setSpin(spin);

    int iterCnt = 0;
    int iterMax = _maxIter;
    for ( ; iterCnt < iterMax; ++iterCnt) {
        
	(void)Iterate(jobFolder, iterCnt);
        if (hasConverged()) { break; }
    }
    
    if (iterCnt == iterMax-1 && !_isConverged) {
        CTP_LOG(logWARNING,*_log)
            << format("Not converged within %1$d iterations.") % iterMax;
    }
    
    return;
}


template<class QMPackage>
bool QMMachine<QMPackage>::Iterate(string jobFolder, int iterCnt) {

    // CREATE ITERATION OBJECT & SETUP RUN DIRECTORY
    QMMIter *thisIter = this->CreateNewIter();
    int iter = iterCnt;
    string runFolder = jobFolder + "/iter_" + boost::lexical_cast<string>(iter);
       
    bool created = boost::filesystem::create_directory(runFolder);
    if (created) 
        CTP_LOG(logDEBUG,*_log) << "Created directory " << runFolder << flush;
    else
        CTP_LOG(logWARNING,*_log) << "Could not create directory " << runFolder << flush;
    
    
    // RUN CLASSICAL INDUCTION & SAVE
    _job->getPolarTop()->PrintPDB(runFolder + "/QM0_MM1_MM2.pdb");
    _xind->Evaluate(_job);
    assert(_xind->hasConverged());
    thisIter->setE_FM(_job->getEF00(), _job->getEF01(), _job->getEF02(),
                      _job->getEF11(), _job->getEF12(), _job->getEM0(),
                      _job->getEM1(),  _job->getEM2(),  _job->getETOT());
    
    // WRITE AND SET QM INPUT FILE
    Orbitals orb_iter_input;
    
    vector<Segment*> empty;
    thisIter->GenerateQMAtomsFromPolarSegs(_job->getPolarTop(), orb_iter_input, _split_dpl, _dpl_spacing);
      
    _qmpack->setRunDir(runFolder);
    
    CTP_LOG(logDEBUG,*_log) << "Writing input file " << runFolder << flush;
    
    _qmpack->WriteInputFile(empty, &orb_iter_input);
 
    FILE *out;
    out = fopen((runFolder + "/system.pdb").c_str(),"w");
    orb_iter_input.WritePDB( out );
    fclose(out);
         
    // RUN HERE (OVERRIDE - COPY EXISTING CTP_LOG-FILE)
    //string cpstr = "cp e_1_n.log " + path_logFile;
    //int sig = system(cpstr.c_str());
    //_qmpack->setLogFileName(path_logFile);
    
    //Commented out for test Jens 
    _qmpack->Run();
    
    // EXTRACT CTP_LOG-FILE INFOS TO ORBITALS   
    Orbitals orb_iter_output;
    _qmpack->ParseLogFile(&orb_iter_output);
    
    out = fopen((runFolder + "/parsed.pdb").c_str(),"w");
    orb_iter_input.WritePDB( out );
    fclose(out);
    
    assert(orb_iter_output.hasSelfEnergy());
    assert(orb_iter_output.hasQMEnergy());
    
    // EXTRACT & SAVE QM ENERGIES
    double energy___sf = orb_iter_output.getSelfEnergy();
    double energy_qmsf = orb_iter_output.getQMEnergy();
    double energy_qm__ = energy_qmsf - energy___sf ;
    thisIter->setQMSF(energy_qm__, energy___sf);
    _job->setEnergy_QMMM(thisIter->getQMEnergy(), 0.0, thisIter->getSFEnergy(),
                         thisIter->getQMMMEnergy());
    
    // EXTRACT & SAVE QMATOM DATA
    vector< QMAtom* > &atoms = *(orb_iter_output.getAtoms());
    
    thisIter->UpdatePosChrgFromQMAtoms(atoms, _job->getPolarTop()->QM0());

    CTP_LOG(logINFO,*_log) 
        << format("Summary - iteration %1$d:") % (iterCnt+1) << flush;
    CTP_LOG(logINFO,*_log)
        << format("... QM Size  = %1$d atoms") % int(atoms.size()) << flush;
    CTP_LOG(logINFO,*_log)
        << format("... E(QM)    = %1$+4.9e") % thisIter->getQMEnergy() << flush;
    CTP_LOG(logINFO,*_log)
        << format("... E(SF)    = %1$+4.9e") % thisIter->getSFEnergy() << flush;
    CTP_LOG(logINFO,*_log)
        << format("... E(FM)    = %1$+4.9e") % thisIter->getFMEnergy() << flush;
    CTP_LOG(logINFO,*_log)
        << format("... E(MM)    = %1$+4.9e") % thisIter->getMMEnergy() << flush;
    CTP_LOG(logINFO,*_log)
        << format("... E(QMMM)  = %1$+4.9e") % thisIter->getQMMMEnergy() << flush;
    CTP_LOG(logINFO,*_log)
        << format("... RMS(dR)  = %1$+4.9e") % thisIter->getRMSdR() << flush;
    CTP_LOG(logINFO,*_log)
        << format("... RMS(dQ)  = %1$+4.9e") % thisIter->getRMSdQ() << flush;
    CTP_LOG(logINFO,*_log)
        << format("... SUM(dQ)  = %1$+4.9e") % thisIter->getSUMdQ() << flush;
    
    // CLEAN DIRECTORY
    _qmpack->CleanUp();

    
    /*
    int removed = boost::filesystem::remove_all(runFolder);
    if (removed > 0) 
        CTP_LOG(logDEBUG,*_log) << "Removed directory " << runFolder << flush;
    else 
        CTP_LOG(logWARNING,*_log) << "Could not remove dir " << runFolder << flush;
    */
    return 0;
     
}


template<class QMPackage>
QMMIter *QMMachine<QMPackage>::CreateNewIter() {
    
    QMMIter *newIter = new QMMIter(_iters.size());
    this->_iters.push_back(newIter);
    return newIter;
}

/*
template<class QMPackage>
void QMMachine<QMPackage>::WriteQMPackInputFile(string inputFile, QMPackage *qmpack, XJob *job) {
    
    // TODO _qmpack should do this entirely independently
    FILE *out;
    out = fopen(inputFile.c_str(), "w");

    // TO ADJUST
    //_qmpack->WriteInputHeader(out, job->getTag());
    job->getPolarTop()->PrintInduState(out, _qmpack->getPackageName(), true, 1e-04);
    fclose(out);
    
}
*/

template<class QMPackage>
bool QMMachine<QMPackage>::hasConverged() {
    
    _convg_dR = false;
    _convg_dQ = false;
    _convg_dE_QM = false;
    _convg_dE_MM = false;
    
    if (_iters.size() > 1) {
        
        QMMIter *iter_0 = _iters[_iters.size()-2];
        QMMIter *iter_1 = _iters[_iters.size()-1];
        
        double dR = iter_1->getRMSdR();
        double dQ = iter_1->getRMSdQ();
        double dE_QM = iter_1->getQMEnergy() - iter_0->getQMEnergy();
        double dE_MM = iter_1->getMMEnergy() - iter_0->getMMEnergy();
        
        if (dR <= _crit_dR) _convg_dR = true;
        if (dQ <= _crit_dQ) _convg_dQ = true;
        if (dE_QM*dE_QM <= _crit_dE_QM*_crit_dE_QM) _convg_dE_QM = true;
        if (dE_MM*dE_MM <= _crit_dE_MM*_crit_dE_MM) _convg_dE_MM = true;        
    }
    
    _isConverged = ((_convg_dR && _convg_dQ) && (_convg_dE_QM && _convg_dE_MM));
    
    CTP_LOG(logINFO,*_log) 
        << format("... Convg dR = %s") % (_convg_dR ? "true" : "false") << flush;
    CTP_LOG(logINFO,*_log) 
        << format("... Convg dQ = %s") % (_convg_dQ ? "true" : "false") << flush;
    CTP_LOG(logINFO,*_log) 
        << format("... Convg QM = %s") % (_convg_dE_QM ? "true" : "false") << flush;
    CTP_LOG(logINFO,*_log) 
        << format("... Convg MM = %s") % (_convg_dE_MM ? "true" : "false") << flush;
    
    return _isConverged;
}


void QMMIter::ConvertPSitesToQMAtoms(vector< PolarSeg* > &psegs,
                                       vector< QMAtom * > &qmatoms) {
    
    assert(qmatoms.size() == 0);    
    return;   
}


void QMMIter::ConvertQMAtomsToPSites(vector< QMAtom* > &qmatoms,
                                       vector< PolarSeg* > &psegs) {
    assert(qmatoms.size() == 0);
    return;
}


void QMMIter::UpdatePosChrgFromQMAtoms(vector< QMAtom* > &qmatoms,
                                         vector< PolarSeg* > &psegs) {
    
    double AA_to_NM = 0.1; // Angstrom to nanometer
    
    double dR_RMS = 0.0;
    double dQ_RMS = 0.0;
    double dQ_SUM = 0.0;
    
    for (unsigned int i = 0, qac = 0; i < psegs.size(); ++i) {
        PolarSeg *pseg = psegs[i];
        for (unsigned int j = 0; j < pseg->size(); ++j, ++qac) {
            
            // Retrieve info from QMAtom
            QMAtom *qmatm = qmatoms[qac];
            vec upd_r = vec(qmatm->x, qmatm->y, qmatm->z);
            upd_r *= AA_to_NM;
            double upd_Q00 = qmatm->charge;
            
            // Compare to previous r, Q00
            APolarSite *aps = (*pseg)[j];
            vec old_r = aps->getPos();
            double old_Q00 = aps->getQ00();
            double dR = abs(upd_r - old_r);
            double dQ00 = upd_Q00 - old_Q00;
            
            dR_RMS += dR*dR;
            dQ_RMS += dQ00*dQ00;
            dQ_SUM += dQ00;
            
            // Forward updated r, Q00 to APS
            aps->setPos(upd_r);
            aps->setQ00(upd_Q00, 0);            
        }
    }
    
    dR_RMS /= qmatoms.size();
    dQ_RMS /= qmatoms.size();
    dR_RMS = sqrt(dR_RMS);
    dQ_RMS = sqrt(dQ_RMS);

    this->setdRdQ(dR_RMS, dQ_RMS, dQ_SUM);
}


void QMMIter::GenerateQMAtomsFromPolarSegs(PolarTop *ptop, Orbitals &orb, 
        bool split_dpl, double dpl_spacing) {
    
    double AA_to_NM = 0.1; // Angstrom to nanometer
    
    // INNER SHELL QM0
    for (unsigned int i = 0; i < ptop->QM0().size(); ++i) {
        PolarSeg *pseg = ptop->QM0()[i];
        for (unsigned int j = 0; j < pseg->size(); ++j) {
            
            APolarSite *aps = (*pseg)[j];
            vec pos = aps->getPos()/AA_to_NM;
            double Q = aps->getQ00();
            string type = "qm";

            orb.AddAtom(aps->getName(), pos.x(), pos.y(), pos.z(), Q, false);            
              
        }
    }
    
    // MIDDLE SHELL MM1
    for (unsigned int i = 0; i < ptop->MM1().size(); ++i) {
        PolarSeg *pseg = ptop->MM1()[i];
        for (unsigned int j = 0; j < pseg->size(); ++j) {
            
            APolarSite *aps = (*pseg)[j];
            vec pos = aps->getPos()/AA_to_NM;
            double Q = aps->getQ00();
            string type = "mm";

            orb.AddAtom(aps->getName(), pos.x(), pos.y(), pos.z(), Q, true);
            
            if (split_dpl) {
                vec tot_dpl = vec(aps->U1x,aps->U1y,aps->U1z);
                if (aps->getRank() > 0)
                    { tot_dpl += vec(aps->Q1x,aps->Q1y,aps->Q1z); }            
                // Calculate virtual charge positions
                double a        = dpl_spacing; // this is in nm
                double mag_d    = abs(tot_dpl); // this is in e * nm
                vec    dir_d_0  = tot_dpl.normalize(); 
                vec    dir_d    = dir_d_0.normalize();
                vec    A        = pos + 0.5 * a * dir_d /AA_to_NM; // converted to AA
                vec    B        = pos - 0.5 * a * dir_d /AA_to_NM;
                double qA       = mag_d / a;
                double qB       = - qA;
                // Zero out if magnitude small [e*nm]
                if (aps->getIsoP() < 1e-9 || mag_d < 1e-9) {
                    A = aps->getPos() + 0.1*a*vec(1,0,0); // != pos since self-energy may diverge
                    B = aps->getPos() - 0.1*a*vec(1,0,0);
                    qA = 0;
                    qB = 0;
                }
                orb.AddAtom("A", A.x(), A.y(), A.z(), qA, true);
                orb.AddAtom("B", B.x(), B.y(), B.z(), qB, true);
            }             
        }
    }
    
    // OUTER SHELL MM2
    for (unsigned int i = 0; i < ptop->MM2().size(); ++i) {
        PolarSeg *pseg = ptop->MM2()[i];
        for (unsigned int j = 0; j < pseg->size(); ++j) {
            
            APolarSite *aps = (*pseg)[j];
            vec pos = aps->getPos()/AA_to_NM;
            double Q = aps->getQ00();
            string type = "mm";

            orb.AddAtom(aps->getName(), pos.x(), pos.y(), pos.z(), Q, true);              
        }
    }
    return;
    
    
}


void QMMIter::setdRdQ(double dR_RMS, double dQ_RMS, double dQ_SUM) {
    
    _hasdRdQ = true;    
    _dR_RMS = dR_RMS;
    _dQ_RMS = dQ_RMS;
    _dQ_SUM = dQ_SUM;
    return;
}


void QMMIter::setQMSF(double energy_QM, double energy_SF) {
    
    _hasQM = true;
    _e_QM = energy_QM;
    _e_SF = energy_SF;    

    _hasGWBSE = true;
   
    return;
}


void QMMIter::setE_FM(double ef00, double ef01, double ef02, 
    double ef11, double ef12, double em0, double em1,  double em2, double efm) {
    
    _hasMM = true;
    _ef_00 = ef00;
    _ef_01 = ef01;
    _ef_02 = ef02;
    _ef_11 = ef11;
    _ef_12 = ef12;
    _em_0_ = em0;
    _em_1_ = em1;
    _em_2_ = em2;
    _e_fm_ = efm;
    return;
}


double QMMIter::getMMEnergy() {
    
    assert(_hasMM);
    return _ef_11 + _ef_12 + _em_1_ + _em_2_;
}


double QMMIter::getQMMMEnergy() {
    
    assert(_hasQM && _hasMM && _hasGWBSE);    
    return _e_QM + + _e_GWBSE + _ef_11 + _ef_12 + _em_1_ + _em_2_;    
}


// REGISTER QM PACKAGES
template class QMMachine<QMPackage>;
    
    
    
}}
