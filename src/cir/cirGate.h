/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
 ****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include "cirDef.h"
#include "sat.h"
#include "intel_rand.h"

using namespace std;

size_t   getInvert       ( const size_t& ) ;
size_t   getNonInv       ( const size_t& ) ;
size_t   getXorInv       ( const size_t& ) ;
size_t   getInvert       ( const void* const ) ;
size_t   getNonInv       ( const void* const ) ;
size_t   getXorInv       ( const void* const ) ;
size_t   getPtrInSize_t  ( const void* const ) ;
bool     isInverted      ( const size_t ) ;
bool     isInverted      ( const unsigned ) ;
bool     isInverted      ( const int ) ;
CirGate* getPtr          ( size_t  ) ;

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
// TODO: Define your own data members and member functions, or classes
class CirGate
{
  public:
    CirGate(): 
      _parent{0}, _active(false), _lineNo(0), 
      _gateID(0), _gateDFSRef(0), _IsDefined(true), _symbolMsg("") {}
    CirGate(int i) : 
      _parent{0}, _active(false), _lineNo(0), 
      _gateID(i), _gateDFSRef(0), _IsDefined(true ), _symbolMsg("") {}
    virtual ~CirGate() {}

    // Basic access methods
    virtual         string getTypeStr() const { return "base"; }
    unsigned        getLineNo() const { return _lineNo; }
    unsigned        getGateID() const { return _gateID; }

    virtual  string getSymbolMsg()                const  = 0;
    virtual  void   setSymbolMsg( const string& )        = 0;
    virtual  bool   isAig ()                      const  = 0;

    // Printing functions
    virtual void    printGate() const = 0;
    void            reportGate() const;
    void            reportFanin   (int) const;
    void            reportFanout  (int) const;
    void            reportFanin   (int, int, bool)  const;
    void            reportFanout  (int, int, size_t)  const;

    // public helpers.
    void            setLineCnt  ( unsigned )      ;
    void            setGateId   ( unsigned )      ;
    void            setGateRef  ( unsigned s ) { _gateDFSRef = s ; }
    size_t          getGateRef  ( )          {return _gateDFSRef ; }
    void            setActive   ( )          { _active = true; }
    void            unsetActive ( )          { _active = false; }
    bool            isActive    ()           { return _active; }
    bool            isDefined   () const     { return _IsDefined; }
    void            makeForgetMe() ;
    bool            makeSkipMe  ( size_t );
    pair< set<size_t>::iterator, bool> insertChild ( size_t );
    set<size_t>::iterator findChild                ( size_t ) const;
    size_t                findParent               ( size_t ) const;
    // for findParent, 1 if it's _parent[0],
    // 2 if it's _parent[1],
    // 0 if it's not my parent.

    // public data fields.
    size_t            _parent[2];
    set<size_t>       _child;

  private:

    bool                    _active;
    unsigned                _lineNo;
    unsigned                _gateID;
    unsigned                _gateDFSRef;

    // helper for reportFanin
    static set<unsigned>    _haveMetBefore;

  protected:
    CirGate( int i , bool b) :
      _parent{0}, _active(false), _lineNo(0), 
      _gateID(i), _gateDFSRef(0), _IsDefined( b ), _symbolMsg("") {}
    // for AAGate only.
    bool             _IsDefined;
    string           _symbolMsg;

};

class POGate : public CirGate {
  public:
    POGate(): 
      CirGate(), initial_inv_info(0) {}
    POGate(int gid, unsigned ini): 
      CirGate(gid), initial_inv_info(ini) {}
    virtual string getTypeStr()    const { return "PO"; }
    virtual string getSymbolMsg()  const { return _symbolMsg; }
    virtual void   printGate()     const ;
    virtual bool   isAig()         const { return false ; }
    virtual void   setSymbolMsg(const string& str) { _symbolMsg = str ; }
    unsigned       getRefGateVar() const {
      if( isInverted( _parent[0] ) ){
        return 2*(getPtr(_parent[0])->getGateID())+1;
      }else{
        return 2*(getPtr(_parent[0])->getGateID());
      }
    }
    unsigned       getIniInvInfo() const {
      return initial_inv_info;
    }
  private:
    unsigned initial_inv_info;
};


class PIGate : public CirGate {
  public:
    PIGate(): CirGate() {}
    PIGate(int i): CirGate(i) {}

    virtual  string getTypeStr()   const { return "PI"; }
    virtual  string getSymbolMsg() const { return _symbolMsg; }
    virtual  void   printGate() const ;
    virtual  bool   isAig()         const { return false ; }
    virtual  void   setSymbolMsg(const string& str) { _symbolMsg = str ; }

  private:
};


class AAGate : public CirGate {
  public:
    AAGate(): CirGate(0, true) {}
    AAGate(bool boolean): CirGate(0, boolean) {}
    AAGate(int i, bool boolean) : CirGate(i, boolean) {}
    virtual string getTypeStr() const 
    { return (  (_IsDefined)? "AIG" : "UNDEF" ); }
    virtual string getSymbolMsg()  const { return ""; }
    virtual void   setSymbolMsg(const string& str) { }
    void setDefined() { _IsDefined = true; }
    void setUNDEF() { _IsDefined = false; }
    virtual bool isAig() const ;
    virtual void printGate() const ;
  private:
};


#endif // CIR_GATE_H
