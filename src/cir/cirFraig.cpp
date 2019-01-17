/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

struct myHashK {
  static const size_t mask1 = 0xEEEEEEEE;
  static const size_t mask2 = 0x77777777;
  size_t operator () (CirGate* c ) const{
    return ( (size_t)
        (mask1 & (c -> _parent[0]) ) ^
        (mask2 & (c -> _parent[1]) ) );
  }
} custom_key;

struct myHashEq {
  bool
    operator () ( CirGate* c1, CirGate* c2 ) const {

      if( c1 == nullptr || c2 == nullptr ){
        if( c1 != nullptr ){
          return false;
        }else if ( c2 != nullptr ){
          return false;
        }else{
          return true;
        }
      }

      if( ( (c1 -> _parent[0] ^ c2 -> _parent[0]) |
            (c1 -> _parent[1] ^ c2 -> _parent[1]) )
          == 0 )
        return true;
      if( ( (c1 -> _parent[0] ^ c2 -> _parent[1]) |
            (c1 -> _parent[1] ^ c2 -> _parent[0]) )
          == 0 )
        return true;
      return false;

    }
} custom_eq ;


using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
  void
CirMgr::strash()
{
  sweep();
  ShallBeEliminatedList.clear();
  const0PtrInSizet = getPtrInSize_t(GateList.find(0)->second);
  using mySTLMultiHash = unordered_multiset < CirGate*, myHashK, myHashEq >;
  mySTLMultiHash strash_hash ;

  // FIXME
  for( auto it : DFSList ){
    auto strash_hash_itor = strash_hash.find( it.first );
    if( strash_hash_itor != strash_hash.end() ){

      // match
      if( (*strash_hash_itor) -> getGateID() ==
          it.first            -> getGateID() )
        continue;

      // different gate; try merge
      // "it.first" appears later than "*strash_hash_itor"
      // thus we shall merge (it.first) to (*strash_hash_itor);
      for( size_t i = 0; i < 2; ++i ){
        auto parent_ptr = getPtr( it.first -> _parent[i] );
        if( parent_ptr != nullptr ){
          auto me_itor = parent_ptr -> _child.find(
              getPtrInSize_t( it.first ) );
          while( me_itor != parent_ptr -> _child.end() ){
            parent_ptr -> _child.erase( me_itor );
            me_itor = parent_ptr -> _child.find(
                getPtrInSize_t( it.first ) );
          }
          if( parent_ptr -> _child.empty() ){
            auto tmp_itor = usedList.find( parent_ptr 
                -> getGateID() );
            if( tmp_itor != usedList.end() )
              usedList.erase ( tmp_itor );
          }
        }
      }
      for( auto childPtrInSizeT : it.first -> _child ){
        auto child_ptr = getPtr( childPtrInSizeT );
        if( child_ptr != nullptr ){
          for( size_t i = 0; i < 2; ++i ){
            if ( getNonInv(child_ptr -> _parent[i])
                == getPtrInSize_t(it.first) ){
              bool invert_info = isInverted(child_ptr->_parent[i] );
              child_ptr -> _parent[i] =
                ( invert_info )?
                getInvert( *strash_hash_itor ) :
                getNonInv( *strash_hash_itor ) ;
            }
          }
        }
        (*strash_hash_itor) -> insertChild ( childPtrInSizeT );
      }
      if( !((*strash_hash_itor) -> _child.empty()) )
        usedList.insert ((*strash_hash_itor) -> getGateID() );
      strash_hash.erase( strash_hash_itor );
      ShallBeEliminatedList.insert( it.first -> getGateID() );
    }else{
      strash_hash.insert( it.first );
    }
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

  void
CirMgr::fraig()
{
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
