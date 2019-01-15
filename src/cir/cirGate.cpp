/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include <string.h> // ffsll
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

extern CirMgr *cirMgr;

set<unsigned>CirGate::_haveMetBefore ( {} );

// TODO: Implement memeber functions for class(es) in cirGate.h

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::reportGate() const
{
  cout << "==================================================" << endl;
  if( getGateID() != 0 )
    cout << "= " << getTypeStr() << '(' << getGateID() << ')' ;
  else
    cout << "= " << "CONST(0)";
  if( getSymbolMsg() != "" )
    cout << '\"' << getSymbolMsg() << '\"';
  cout << ", line " << getLineNo() << endl;
  cout << "==================================================" << endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   _haveMetBefore.clear();
   reportFanin( level+1, 0, false );
}

void
CirGate::reportFanin( int total_level, int indent, bool print_exclam ) const {

  auto itor = _haveMetBefore . find( getGateID() );

  if( !( indent < total_level ) ){
    return;
  }

  for( int i = 0; i < indent*2; ++i )
    cout << ' ';

  if( print_exclam )
    cout << '!';

  if( getGateID() != 0 )
    cout << getTypeStr() << ' ' << getGateID();
  else
    cout << "CONST 0";

  if( itor != _haveMetBefore.end() ){
    cout << " (*)";
    indent = total_level;
  }

  // don't report further.
  cout << endl;

  if( !isDefined() )
    indent = total_level;
  // don't report further.

  if( _parent[0] != 0 ){
    getPtr( _parent[0] ) -> 
      reportFanin( total_level, indent+1,
          (( isInverted( _parent[0] ))? true : false ) );
  }
  if( _parent[1] != 0 ){
    getPtr( _parent[1] ) -> 
      reportFanin( total_level, indent+1,
          (( isInverted( _parent[1] ))? true : false ) );
  }

  if( indent < (total_level-1) 
      && (_parent[0] != 0 || _parent[1] != 0) ) {
    _haveMetBefore.insert( getGateID() );
  }
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   _haveMetBefore.clear();
   reportFanout( level+1, 0, 0 );
}

void
CirGate::reportFanout( int total_level, int indent, size_t parent_ptr ) const {

  auto   itor = _haveMetBefore . find( getGateID() );
  size_t parent_tricky_idx    = findParent( parent_ptr );
  size_t parent_with_inv_info = (parent_tricky_idx == 0 )?
    0 : _parent[parent_tricky_idx-1] ;

  if( !( indent < total_level ) ){
    return;
  }

  for( int i = 0; i < indent*2; ++i )
    cout << ' ';
  if( parent_with_inv_info && (parent_ptr != 0) )
    if( isInverted( parent_with_inv_info ) )
      cout << '!';
  if( getGateID() != 0 )
    cout << getTypeStr() << ' ' << getGateID();
  else
    cout << "CONST 0";

  if( itor != _haveMetBefore.end() ){
    cout << " (*)";
    indent = total_level;
  }

  // don't report further.
  cout << endl;

  for( auto it : _child )
    getPtr( it ) ->
      reportFanout( total_level, indent+1, 
          reinterpret_cast<size_t>(this) );

  if( indent < (total_level-1) && ! _child.empty() )
    _haveMetBefore.insert( getGateID() );
}

bool
isInverted( const size_t s ) {
  size_t i = (s & 1);
  return i;
}

bool
isInverted( const unsigned s ) {
  unsigned i = (s & 1);
  return i;
}

bool
isInverted( const int s ) {
  int i = (s & 1);
  return i;
}

size_t
getInvert( const size_t& s) { 
  return (s | 1);
}

size_t
getNonInv( const size_t& s ) {
  return (s & (~1));
}

size_t
getXorInv( const size_t& s ) {
  return (s ^ (1));
}

CirGate*
getPtr( size_t s ) {
  size_t ret = getNonInv(s);
  return reinterpret_cast< CirGate* > (ret);
}

pair< set<size_t>::iterator, bool>
CirGate::insertChild( size_t s) {
  // shall NOT contain inverted info!!
  return _child.insert( getNonInv(s) );
}

set<size_t>::iterator
CirGate::findChild( size_t s ) const{
  // no inv info shall be included.
  return _child.find( getNonInv(s) );
}

size_t
CirGate::findParent( size_t s ) const{
  // _parent contains inv info.
  // minus 1 to get index.
  if( _parent[0] == s || _parent[0] == getXorInv(s) )
    return 1;
  else if( _parent[1] == s || _parent[1] == getXorInv(s) )
    return 2;
  else
    return 0;
}

void
CirGate::setLineCnt( unsigned u){
  _lineNo = u;
}

void
POGate::printGate() const {
  cout << "PO  " << getGateID();
  cout << ' ';
  if( isInverted( getRefGateVar() ) )
    cout << '!' ;
  cout << getRefGateVar()/2 ;
  if( _symbolMsg != "" )
    cout << "  (" << _symbolMsg << ')';
}

void
PIGate::printGate() const {
  cout << "PI  " << getGateID();
  if( _symbolMsg != "" )
    cout << "  (" << _symbolMsg << ')';
}

void
AAGate::printGate() const {
  if( getGateID() == 0 )
    cout << "CONST0";
  else{
    cout << "AIG " << getGateID() << ' ';

    if( getPtr( _parent[0] ) -> getTypeStr() == "UNDEF" )
      cout << '*';
    if( isInverted( _parent[0] ))
      cout << '!';
    cout << getPtr( _parent[0] ) -> getGateID();

    cout << ' ';

    if( getPtr( _parent[1] ) -> getTypeStr() == "UNDEF" )
      cout << '*';
    if( isInverted( _parent[1] ))
      cout << '!';
    cout << getPtr( getNonInv( _parent[1] )) -> getGateID();
  }
}

void
CirGate::makeForgetMe() {
  // make my parents forget about me.
  // notice:
  // 1. shall be called with some proper fixups.
  //    e.g.: buildDFSList, GateList.erase,
  // 2. shall be called with delete.
  // 
  // may make some gates "defined but not used",
  // -- put it in DefButNUsedList in this case;
  // may make some gates have dangerous floating fan-in,
  // well, that's a FIXME, though...
  for( size_t i = 0; i < 2; ++i ){
    auto tmp_parent_ptr = getPtr( _parent[i] );
    if( tmp_parent_ptr != nullptr ){
      auto me_itor = tmp_parent_ptr -> findChild( getPtrInSize_t( this));
      if( me_itor != tmp_parent_ptr -> _child.end() )
        tmp_parent_ptr -> _child . erase( me_itor );
    }
  }

  // make my children forget about me.
  for( auto itor = _child.begin(); itor != _child.end(); ++itor ){
    auto tmp_child_ptr = getPtr( *itor );
    auto parent_tricky_idx = tmp_child_ptr -> findParent( 
        getPtrInSize_t( this ) );
    if( parent_tricky_idx != 0 ){
      tmp_child_ptr -> _parent[ parent_tricky_idx-1 ] = 0;
    }
  }

}

bool
AAGate::isAig() const {
  return _IsDefined;
}

size_t
getInvert( const void * const ptr ) {
  return getInvert( reinterpret_cast<size_t> (ptr) );
}

size_t
getNonInv( const void * const ptr ) {
  return getNonInv( reinterpret_cast<size_t> (ptr) );
}

size_t
getXorInv( const void * const ptr ) {
  return getXorInv( reinterpret_cast<size_t> (ptr) );
}

size_t
getPtrInSize_t( const void* const ptr ) {
  return reinterpret_cast<size_t>(getPtr((size_t)ptr));
}
