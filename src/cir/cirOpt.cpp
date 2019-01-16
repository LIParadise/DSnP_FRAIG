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
  getNewGDFSOptRef();
  ShallBeEliminatedList.clear();
  const0PtrInSizet = getPtrInSize_t(GateList.find(0)->second);
  // FIXME
  // usedList and definedList shall be modified...
  for( auto it : POIDList ){
    CirGate* tmp_gate_ptr = GateList.find( it ) -> second;
    DFS_4_optimize( getPtrInSize_t(tmp_gate_ptr) );
  }

  for( auto it : ShallBeEliminatedList ){
    auto itor = GateList.find( it );
    if ( itor != GateList.end() ){
      delete itor -> second;
      itor -> second = nullptr;
      GateList.erase(itor);
    }
  }

  DefButNUsedList.clear();
  UnDefinedList  .clear();
  DFSList        .clear();
  buildDFSList         ();
  rebuildOutputBak     ();
  set_difference( definedList.begin(), definedList.end(),
      usedList.begin(), usedList.end(),
      inserter(DefButNUsedList, DefButNUsedList.end()));

  set_difference( usedList.begin(), usedList.end(),
      definedList.begin(), definedList.end(),
      inserter(UnDefinedList, UnDefinedList.end()) );
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/

size_t
CirMgr::DFS_4_optimize( size_t current_gate_NO_inv_info ){
  
  current_gate_NO_inv_info = getNonInv( current_gate_NO_inv_info);
  auto workingGatePtr = getPtr( current_gate_NO_inv_info );
  if( workingGatePtr == nullptr )
    return getPtrInSize_t(nullptr);

  if( workingGatePtr -> getTypeStr() == "PI"    ||
      workingGatePtr -> getTypeStr() == "UNDEF" ||
      workingGatePtr -> getGateID () == 0 )
    return getNonInv(current_gate_NO_inv_info);
  // ending conditions;

  size_t _parent0 = DFS_4_optimize(
      getNonInv(workingGatePtr->_parent[0]));
  size_t _parent1 = DFS_4_optimize(
      getNonInv(workingGatePtr->_parent[1]));
#ifdef DEBUG
  if( workingGatePtr -> getTypeStr() == "PO" )
    if( _parent0 == 0 || _parent0 == 1 )
      assert( 0 && "WTF PO in DFS_4_optimize" );
#endif // DEBUG

  if( isInverted( workingGatePtr->_parent[0] ) )
    _parent0 = getInvert( _parent0 );
  if( isInverted( workingGatePtr->_parent[1] ) )
    _parent1 = getInvert( _parent1 );

  if( workingGatePtr -> getTypeStr() == "PO" ){
    workingGatePtr -> _parent[0] = _parent0;
#ifdef DEBUG
    assert( _parent0 != 0 && "WTF PO in DFS_4_optimize" );
#endif // DEBUG
    workingGatePtr -> _parent[1] = getPtrInSize_t(nullptr);
  }

  if( _parent0 == _parent1 ){

    tryEliminateMeWith( current_gate_NO_inv_info, _parent0);
    return getNonInv(_parent0);

  }else if( getXorInv( _parent0 ) == _parent1 ){

    tryEliminateMeWith( 
        current_gate_NO_inv_info, const0PtrInSizet ) ;
    return const0PtrInSizet;

  }else if( _parent0 == const0PtrInSizet ||
      _parent1 == const0PtrInSizet ){

    tryEliminateMeWith(
        current_gate_NO_inv_info, const0PtrInSizet  );
    return const0PtrInSizet;

  }else if( _parent1 == getInvert(const0PtrInSizet) ){

    tryEliminateMeWith( current_gate_NO_inv_info, _parent0  );
    return getNonInv(_parent0);

  }else if( _parent0 == getInvert(const0PtrInSizet) ){

    tryEliminateMeWith( current_gate_NO_inv_info, _parent1 );
    return getNonInv(_parent1);

  }else {

    // PO would end up be here.
    tryEliminateMeWith( current_gate_NO_inv_info, _parent0 );
    return getNonInv(current_gate_NO_inv_info);
  }

}
