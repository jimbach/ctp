For more detailed information about the changes see the history of the [repository](https://github.com/votca/ctp/commits/master).

## Version 1.5.1 (released 20.11.19)
* fix bug on epel7

## Version 1.5 (released 28.01.19)
* Updated manual: EWALD3D documentation
* Moved kinetic Monte Carl to votca-ctp
* KMC now supports more than one charge 
* fixes in pdb2map

## Version 1.4 rc1 (released 01.01.18)
* jobwriter functionality moved to the parallel calculators
* splitting of the jobfile for cluster runs 
* incorporation of the molecular orbital overlap code

## Version 1.3 (released 01.09.15)
* new executables: ctp_tools, ctp_dump, ctp_parallel, ctp_testsuite, ctp_update
* ctp_tools wraps light-weight tools that assist e.g. in generating the system mapping file
* ctp_dump extracts information from the state file to human-readable format
* ctp_parallel wraps heavy-duty job-based calculators, allowing for synchronization across processes
* ctp_testsuite provides an easy-to-use environment to run selected tests as well as individual calculators
* ctp_update updates an existent state file to the current version
* new calculators: edft, idft, pdb2map, xqmultipole, ...
* edft / idft (provide interfaces to the GAUSSIAN, TURBOMOLE & NWCHEM packages to compute couplings, internal energies and partial charges)
* pdb2map (generates a system mapping file from an input coordinate file)
* xqmultipole (computes classical configuration energies of charged clusters embedded in a molecular environment)
* enhanced usability via the command-line help, tutorial & test-suite
* a GUI tutorial assists with the first practical steps in using VOTCA-CTP
* an extended and homogenized help system provides short infos on individual calculator options from the command line

## Version 1.0 (released 23.10.11)
* parallel evaluation of site energies (Thole model + GDMA) - Tinker no longer required
* much clearer input files (and many more checks for input errors)
* most of calculators are parallel and can be used on a cluster
* bug in zindo/ctp interface fixed
* state file now contains the atomistic trajectory, rigid fragments and conjugates segments
* support for several MD frames
