/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <utility>
#include <ctime>
#include <queue>
#include <iterator>
// for std::inserter;
#include <algorithm>
#include "cirDef.h"
#include "intel_rand.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr(): globalDFSRef(0), globalBFSRef(0)  { mysrand_sse(time(NULL));}
   ~CirMgr() { clearGate();}

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned ) const ;

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

private:
   ofstream           *_simLog;

   // helper functions
   bool buildDFSList      () ;
   bool DFS               (CirGate*, unsigned = 0);
   void BFS_4_optimize    (queue<unsigned>& ) ;
   void clearGate         () ;
   void getNewGDFSRef     () ;
   void getNewGBFSRef     () ;
   void rebuildOutputBak  () ;
   void trySimplify
     (size_t,size_t,queue<unsigned>&);
   // the_gate_we're_working, the_child_we're_checking,
   // the_queue;
   // 
   // "trySimplify" does the following:
   // 1. updates Q if necessary.
   // 2. mark (getPtr(tmp_child_ptr_sizet)) if necessary.
   void maintainDefinedListAndUsedList
     (size_t,size_t);
   // the_gate_we're_working, the_parent_we_want_check

   // helper data fields
   set<unsigned>  definedList;
   // PI, AAG.
   set<unsigned>  usedList;
   // PO, fanin of AAG.

   unsigned  globalDFSRef;
   unsigned  globalBFSRef;
   // one for building DFS list, one for trivial-gate-optimizing.
   unsigned  permanent_line_count;

   set< unsigned > DefButNUsedList;
   set< unsigned > UnDefinedList;
   set< unsigned > ShallBeEliminatedList;
   // stores gate id that have been merged but have not been deleted.
   // i.e. not returned back to system.

   map< unsigned, CirGate* > GateList;

   vector< unsigned > PIIDList;
   vector< unsigned > POIDList;
   vector< unsigned> MILOA;
   // TODO, shall have a field remaining symbol msg.

   vector< pair<CirGate*, unsigned> > DFSList;
   // second is depth.
   vector< string>                    output_bak;


};

bool myPairUnsignedCharCmp (
    const pair< unsigned, char>& ,
    const pair< unsigned, char>& ) ; 

#endif // CIR_MGR_H

