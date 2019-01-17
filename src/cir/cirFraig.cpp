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
  size_t operator () (CirGate* c ) const{
    return ( (size_t)
        ((c -> _parent[0]) ) ^
        ((c -> _parent[1]) ) );
  }
} custom_key;

struct myHashEq {
  bool operator () ( CirGate* c1, CirGate* c2 ) const {

      if( c1 == nullptr || c2 == nullptr ){
        if( c1 != nullptr ){
          return false;
        }else if ( c2 != nullptr ){
          return false;
        }else{
          return true;
        }
      }

      if( ( (((size_t)c1->_parent[0])^((size_t)c2->_parent[0])) |
            (((size_t)c1->_parent[1])^((size_t)c2->_parent[1])) )
          == 0 )
        return true;
      if( ( (((size_t)c1->_parent[0])^((size_t)c2->_parent[1])) |
            (((size_t)c1->_parent[1])^((size_t)c2->_parent[0])) )
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
  unordered_set< CirGate*, myHashK, myHashEq > strash_hash ;

  for( auto itor1 : DFSList ){

    auto strash_hash_itor = strash_hash.find( itor1.first );

    if( strash_hash_itor != strash_hash.end() ){

      CirGate* mainPtr = (*strash_hash_itor);
      CirGate* subPtr  = itor1.first;
      if( ! (
            (*strash_hash_itor) -> getTypeStr() == "PI"    ||
            (*strash_hash_itor) -> getTypeStr() == "UNDEF" || 
            (*strash_hash_itor) -> getGateID()  == 0
            ) ){
        if( itor1.first -> getTypeStr() == "PI"    ||
            itor1.first -> getTypeStr() == "UNDEF" || 
            itor1.first -> getGateID()  == 0 ){
          // the new-comer is more suitable for being in hash.
          subPtr  = (*strash_hash_itor );
          mainPtr = itor1.first;
          strash_hash.erase( strash_hash_itor );
          strash_hash.insert( itor1.first );
        }
      }
      if( mainPtr -> getTypeStr() == "PI"    ||
          mainPtr -> getTypeStr() == "UNDEF" || 
          mainPtr -> getGateID()  == 0 ){
        if( subPtr -> getTypeStr() == "PI" ||
            subPtr -> getTypeStr() == "UNDEF" || 
            subPtr -> getGateID() == 0 ){
          // both cannot be merged, actually.
          continue;
        }
      }

#ifdef DEBUG
      cerr << "strashing..." << endl;
      cerr << "mainPtr   : " << mainPtr -> getGateID() << endl;
      cerr << "subPtr    : " << subPtr  -> getGateID() << endl;
      if( mainPtr -> getGateID() == 4 ){
        cerr << "-----" << (size_t)(mainPtr -> _parent[0]) << endl;
        cerr << "-----" << (size_t)(mainPtr -> _parent[1]) << endl;
      }
#endif // DEBUG

      // different gate; try merge
      // we shall merge subPtr to mainPtr;
      for( size_t i = 0; i < 2; ++i ){
        auto parent_ptr = getPtr( subPtr -> _parent[i] );
        if( parent_ptr != nullptr ){
          auto me_itor = parent_ptr -> _child.find(
              getPtrInSize_t( subPtr ) );
          while( me_itor != parent_ptr -> _child.end() ){
            parent_ptr -> _child.erase( me_itor );
            me_itor = parent_ptr -> _child.find(
                getPtrInSize_t( subPtr ) );
          }
          // parent_ptr shall not have empty child,
          // since we're doing strash.
          // don't touch the usedList;
        }
      }
      for( auto childPtrInSizeT : subPtr -> _child ){
        auto child_ptr = getPtr( childPtrInSizeT );
        if( child_ptr != nullptr ){
          for( size_t i = 0; i < 2; ++i ){
            if ( getNonInv(child_ptr -> _parent[i])
                == getPtrInSize_t(subPtr) ){
              bool invert_info = isInverted(child_ptr->_parent[i] );
              child_ptr -> _parent[i] =
                ( invert_info )?
                getInvert( mainPtr ) :
                getNonInv( mainPtr ) ;
            }
          }
        }
        (mainPtr) -> insertChild ( childPtrInSizeT );
      }
      usedList.insert( mainPtr -> getGateID() );
      if( usedList.find( subPtr -> getGateID() ) != usedList.end() )
        usedList.erase( usedList.find( subPtr -> getGateID() ) );
      if( subPtr -> getTypeStr() == "AAG" ){
        auto definedListItor = definedList.find(subPtr->getGateID());
        if( definedListItor != definedList.end() )
          definedList.erase( definedListItor );
      }
      ShallBeEliminatedList.insert( subPtr -> getGateID() );

    }else{
      if( itor1.first -> getTypeStr() != "PO" )
        strash_hash.insert( itor1.first );
    }
  }

  for( auto itor1 : ShallBeEliminatedList ){
    auto itor = GateList.find( itor1 );
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
