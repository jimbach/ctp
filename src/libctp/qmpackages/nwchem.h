/* 
 *            Copyright 2009-2012 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __VOTCA_CTP_NWCHEM_H
#define	__VOTCA_CTP_NWCHEM_H


#include <votca/ctp/apolarsite.h>
#include <votca/ctp/qmpackage.h>

#include <string> 

using namespace std;

namespace votca { namespace ctp {
/**
    \brief Wrapper for the Gaussian program
 
    The Gaussian class executes the Gaussian package 
    and extracts information from its log and io files
    
*/
class NWChem : public QMPackage
{
public:   

   string getPackageName() { return "nwchem"; }

   void Initialize( Property *options );

   /* Writes Gaussian input file with coordinates of segments
    * and a guess for the dimer (if requested) constructed from the
    * monomer orbitals
    */
   bool WriteInputFile( vector< Segment* > segments, Orbitals* orbitals_guess = NULL);

   bool WriteShellScript();

   bool Run();

   void CleanUp();
   
   bool CheckLogFile();

   bool ParseLogFile( Orbitals* _orbitals );

   bool ParseOrbitalsFile( Orbitals* _orbitals );

  
   
   string getScratchDir( ) { return _scratch_dir; }
   
private:  

    string                              _shell_file_name;
    string                              _chk_file_name;
    string                              _scratch_dir;
    bool                                _is_optimization;
        
    string                              _cleanup;

    int NumberOfElectrons( string _line ); 
    int BasisSetSize( string _line ); 
    int EnergiesFromLog( string _line, ifstream inputfile ); 
    string FortranFormat( const double &number );

    
};


}}

#endif	/* __VOTCA_CTP_NWCHEM_H */

