/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <unordered_set>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void 
CirMgr::sweep() {

  unordered_set<unsigned> ID_in_DFSList;
  ID_in_DFSList.reserve( DFSList.size() );

  for( auto itor = DFSList.begin(); itor != DFSList.end() ; ++itor ){
    ID_in_DFSList.insert( itor -> first -> getGateID() );
  }

  for( auto itor = GateList.begin(); itor != GateList.end(); ++itor ){
    if( ID_in_DFSList.find( itor -> second -> getGateID() )
        == ID_in_DFSList.end () ){
      if( itor -> second -> getTypeStr() == "UNDEF") {
        GateList.erase(itor);
      }else if( itor -> second -> getTypeStr() == "AIG" ){
        if( itor -> second -> getGateID() != 0 ){
          GateList.erase(itor);
        }
      }
    }
  }

  for( auto itor = UnDefinedList.begin(); itor != UnDefinedList.end();
      ++itor ) {
    auto GateList_itor = GateList.find( *itor );
    if( GateList_itor != GateList.end() ){
      delete ( GateList_itor->second );
      GateList.erase( GateList_itor );
    }
  }
  UnDefinedList.clear();
  // done 0119 1610
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  // FIXME
  // shall do BFS...
//  sweep();
//  queue<unsigned> 
//  for( auto pi_idx : PIIDList ){
//    auto tmp_gate_ptr = GateList.find( pi_idx );
//    // shall always be present, thus no robustness test here.
//    for( auto child_ptr_size_t : tmp_gate_ptr -> _child ){
//      
//    }
//  }
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
