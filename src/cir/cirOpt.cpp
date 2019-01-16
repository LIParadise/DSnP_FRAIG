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

        auto tmp_itor = usedList.find( itor -> second -> getGateID() );
        if( tmp_itor != usedList.end() )
          usedList.erase( tmp_itor );

        itor -> second -> makeForgetMe ();
        delete( itor -> second );
        itor -> second = nullptr;
        GateList.erase(itor);

      }else if( itor -> second -> getTypeStr() == "AIG" ){
        if( itor -> second -> getGateID() != 0 ){

          auto tmp_itor = definedList.find( itor -> second -> getGateID() );
          if( tmp_itor != definedList.end() )
            definedList.erase( tmp_itor );
          tmp_itor = usedList.find( itor -> second -> getGateID() );
          if( tmp_itor != usedList.end() )
            usedList.erase( tmp_itor );

          itor -> second -> makeForgetMe ();
          delete( itor -> second );
          itor -> second = nullptr;
          GateList.erase(itor);
        }
      }
    }
  }

  DefButNUsedList.clear();
  UnDefinedList  .clear();
  set_difference( usedList.begin(), usedList.end(),
                 definedList.begin(), definedList.end(),
                 inserter(UnDefinedList, UnDefinedList.end()) );
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  sweep();
  ShallBeEliminatedList.clear();
  queue<unsigned> BFS_Q;
  for( auto it : PIIDList )
    BFS_Q.push( it );
  getNewGBFSRef();
  BFS_4_optimize( BFS_Q );

  DefButNUsedList.clear();
  UnDefinedList  .clear();
  DFSList        .clear();
  buildDFSList();
  set_difference( definedList.begin(), definedList.end(),
                 usedList.begin(), usedList.end(),
                 inserter(DefButNUsedList, DefButNUsedList.end()));

  set_difference( usedList.begin(), usedList.end(),
                 definedList.begin(), definedList.end(),
                 inserter(UnDefinedList, UnDefinedList.end()) );
  // FIXME
  // usedList and definedList shall be modified...
}

void
CirMgr::BFS_4_optimize( queue<unsigned>& Q ){

  while( !Q.empty() ){
    
    auto it = Q.front();
    Q.pop();
    auto tmp_gatelist_itor = GateList.find( it );

    if( tmp_gatelist_itor != GateList.end() ){

      auto tmp_cirgate_ptr = tmp_gatelist_itor -> second;

      for( auto tmp_child_ptr_sizet : tmp_cirgate_ptr -> _child ){
        trySimplify( 
            getPtrInSize_t( tmp_cirgate_ptr ), tmp_child_ptr_sizet, Q 
            );
      }
    }
  }

  for( auto to_B_deleted_GID : ShallBeEliminatedList ){
    auto tmp_gatelist_itor = GateList.find( to_B_deleted_GID );
    if( tmp_gatelist_itor != GateList.end() ){
      delete ( tmp_gatelist_itor -> second );
      tmp_gatelist_itor -> second = nullptr ;
      GateList.erase( tmp_gatelist_itor );
    }
  }
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
