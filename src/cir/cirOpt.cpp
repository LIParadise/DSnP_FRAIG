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
  // return the pointer representing this gate.
  // e.g. return "const0PtrInSizet" if this gate is to be
  // replaced with CONST0;
  // return value could contain inversion information.
  CirGate* workingGatePtr = getPtr( current_gate_NO_inv_info );
  if( workingGatePtr == nullptr )
    return getPtrInSize_t(nullptr);

  bool ret0_is_xor = isInverted( workingGatePtr -> _parent[0] );
  bool ret1_is_xor = isInverted( workingGatePtr -> _parent[1] );

  if( workingGatePtr -> getTypeStr() == "PI"    ||
      workingGatePtr -> getTypeStr() == "UNDEF" ||
      workingGatePtr -> getGateID()  == 0 ){
    return getNonInv( current_gate_NO_inv_info );
  }

  size_t ret0 = DFS_4_optimize( 
      getNonInv(workingGatePtr -> _parent[0] ) );
  size_t ret1 = DFS_4_optimize( 
      getNonInv(workingGatePtr -> _parent[1] ) );

  if( ret0_is_xor )
    ret0 = getXorInv( ret0 );
  if( ret1_is_xor )
    ret1 = getXorInv( ret1 );

  if( ret1 == ret0 ) {

    merge( getNonInv( current_gate_NO_inv_info ), ret0 );
    return ret0;

  } else if( ret1 == getXorInv( ret0 ) ){

    merge( getNonInv( current_gate_NO_inv_info ), const0PtrInSizet);
    return const0PtrInSizet;
  
  } else if( ret0 == const0PtrInSizet ||
      ret1        == const0PtrInSizet ){

    merge( getNonInv( current_gate_NO_inv_info ), const0PtrInSizet);
    return const0PtrInSizet;

  } else if( ret1 == getInvert( const0PtrInSizet ) ){

    merge( getNonInv( current_gate_NO_inv_info ), ret0 );
    return ret0;

  } else if( ret0 == getInvert( const0PtrInSizet ) ){
     
    merge( getNonInv( current_gate_NO_inv_info ), ret1 );
    return ret1;

  } else {

    if( workingGatePtr -> getTypeStr() == "PO" ){
      mergePO( getNonInv( current_gate_NO_inv_info ), ret0 );
    }
    return getNonInv( current_gate_NO_inv_info );
  }

}

void 
CirMgr::merge( size_t current_gate_NO_inv_info,
    size_t parent_with_inv_info ) {

  CirGate* workingGatePtr = getPtr( current_gate_NO_inv_info );
  CirGate* tmp_ptr        = getPtr( parent_with_inv_info     );
  if( workingGatePtr -> getTypeStr() != "PO" &&
      workingGatePtr -> getTypeStr() != "PI" && 
      workingGatePtr -> getGateID()  != 0 ){
    ShallBeEliminatedList.insert( workingGatePtr -> getGateID() );
    auto definedListItor = definedList.find( 
        workingGatePtr -> getGateID() );
    if( definedListItor != definedList.end() )
      definedList.erase( definedListItor );
  }
#ifdef DEBUG
  cerr << workingGatePtr -> getGateID() << 'm'
    <<( (tmp_ptr == nullptr)? 'C' : tmp_ptr -> getGateID() )<<endl;
#endif // DEBUG

  // manage parents, eliminate me.
  for( size_t idx = 0; idx < 2; ++idx ){
    tmp_ptr = getPtr( workingGatePtr -> _parent[idx] );
    if( tmp_ptr != nullptr ){
      auto tmp_itor = tmp_ptr -> _child.find( 
          getNonInv( current_gate_NO_inv_info ) );
      while( tmp_itor != tmp_ptr -> _child.end() ){
        tmp_ptr -> _child.erase( tmp_itor );
        tmp_itor = tmp_ptr -> _child.find(
            getNonInv( current_gate_NO_inv_info ));
      }
      if( tmp_ptr -> _child.empty() ){
        auto usedListItor = usedList.find( tmp_ptr -> getGateID() );
        if( usedListItor != usedList.end() )
          usedList.erase( usedListItor );
      }
    }
  }

  // manage children, send them to THE parent.
  tmp_ptr = getPtr( parent_with_inv_info );
  for( auto idx : workingGatePtr -> _child ){
    tmp_ptr -> insertChild( idx );
  }
  if( !tmp_ptr -> _child.empty() )
    usedList.insert( tmp_ptr -> getGateID() );

  // manage children, fix their parents.
  for( auto idx : workingGatePtr -> _child ){
    tmp_ptr = getPtr( idx );
    for( size_t i = 0; i < 2; ++i ){
      if( getNonInv( tmp_ptr -> _parent[i] ) ==
          getNonInv( current_gate_NO_inv_info ) ){
        if( isInverted( tmp_ptr -> _parent[i] ) ){
          tmp_ptr -> _parent[i] = 
            getXorInv( parent_with_inv_info );
        }else{
          tmp_ptr -> _parent[i] = parent_with_inv_info ;
        }
      }
    }
  }

}

void
CirMgr::mergePO( size_t current_gate_NO_inv_info,
    size_t parent_with_inv_info ){

  getPtr ( current_gate_NO_inv_info ) -> _parent[0]
    = parent_with_inv_info;
  getPtr ( parent_with_inv_info ) -> insertChild(
      current_gate_NO_inv_info );
}
