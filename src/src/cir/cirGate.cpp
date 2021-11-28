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
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"
#include "bitset"
#include "sat.h"
#include <cassert>


using namespace std;


// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

unsigned CirGate::_globalFlag = 0;

// static functions
#define MASK 255

void printBits( size_t n ) {
   for( int i = 0; i < 8; ++i ) {
      if ( i ) cout <<  '_';
      for( int j = 0; j < 8; ++j ) {
         cout << n % 2;
         n /= 2;
      }
   }
}

//=== class CirGateV ===

const bool CirGateV::isConst() const {
   return gate() -> getID() == 0; 
}
size_t CirGateV::getSim( int index ) const 
{
   if ( index == -1 ) return phase() ? ~ gate() -> _sim.back() : gate() -> _sim.back();
   //cout << gate()->getTypeStr() << " " << gate() -> _sim.size()  << endl;
   assert( index >= 0 && index < gate()->_sim.size() );
   return phase() ? ~ gate() -> _sim[index] : gate() -> _sim[index];

}

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void CirGate::printGate_netList() const {
   cout << setw(4) << left << getTypeStr();
   cout << _id;
   if ( !_fanin.empty() ) {
      for ( size_t i = 0; i < _fanin.size(); ++ i ) {
         cout << ' ';
         if ( _fanin[i].gate() -> getType() == UNDEF_GATE ) cout << '*';
         if ( _fanin[i].phase() ) cout << '!';
         cout << _fanin[i].gate() -> getID();
      }
   }
   if ( !_name.empty() ) {
      cout << " (" << _name << ')';
   }
   cout << endl;
}
void CirGate::reportGate() const {
   cout << string( 80, '=' ) << endl;
   cout << "= " << getTypeStr() << '(' << _id << ')'; 
   if ( ! _name.empty() ) {
      cout << '\"' << _name << '\"';
   }
   cout << "= Value: ";
   printBits( _sim.back() );
   cout << endl;
   cout << string( 80, '=' ) << endl;

}
void CirGate::reportFanin(int level) const {
   assert (level >= 0);
   _globalFlag += 1;
   CirGate* g = const_cast<CirGate*>(this);
   g -> preOrderPrint( level, 0, true );
}
void CirGate::reportFanout(int level) const {
   assert (level >= 0);
   _globalFlag += 1;
   CirGate* g = const_cast<CirGate*>(this);
   g -> preOrderPrint( level, 0, false );
}
void CirGate::preOrderPrint( int level, int indent, bool fanin ) {
   indent += 2;
   cout  << getTypeStr() << ' ' << _id; 
   vector<CirGateV> &g = fanin ? _fanin : _fanout;
   if ( g.size() && level && _flag == _globalFlag ) {
         cout << " (*)" << endl;
   } else {
      cout << endl;
      if ( level ) {
         for ( size_t i = 0; i < g.size(); ++i ) {
            cout << string( indent, ' ' );
            if ( g[i].phase() ) cout << '!';
            g[i].gate() -> preOrderPrint( level - 1, indent, fanin );
         } 
      }
   }
   if ( level ) {
      _flag = _globalFlag;
   }
}
void CirGate::postOrderVisit( IdList& _netList, IdList& _aigNetlist ) {
   if ( _flag < _globalFlag ) {
      _flag = _globalFlag;
      for (size_t i = 0; i < _fanin.size(); ++i) {
         _fanin[i].gate() -> postOrderVisit( _netList, _aigNetlist );
      }
      _netList.push_back( _id );
      if ( getType() == AIG_GATE ) _aigNetlist.push_back( _id );
   }
}
void CirGate::connect( vector<CirGate*>& gateList ) {
   for (size_t i = 0; i < _fanin.size(); ++i) {
      uint id = _fanin[i].gateId();
      if ( !gateList[id/2] ) {
         gateList[id/2] = new UndefGate( id/2 );
      }
      _fanin[i].set( gateList[id/2], id%2 );
      _fanin[i].gate() -> addFanout( this, id%2 );
   }
}
void CirGate::disconnect() {
   for ( uint i = 0; i < _fanin.size(); ++i ) {
      eraseOneData( _fanin[i].gate() -> _fanout, this );
   }
   for ( uint i = 0; i < _fanout.size(); ++i ) {
      eraseOneData( _fanout[i].gate() -> _fanin, this );
   }
}
// replace a gate with the CirGateV in b
void CirGate::replace( const CirGateV& b ) {
   // TODO: just set the cirGateVs
   for ( size_t i = 0; i < _fanout.size(); i++ ) {
      replaceOne( _fanout[i].gate() -> _fanin, CirGateV( b.gate(), (b.phase()^_fanout[i].phase()) ), this );
      b.gate() -> addFanout( _fanout[i].gate(), (b.phase()^_fanout[i].phase()) );
   }
   for ( size_t i = 0; i < _fanin.size(); i++ ) {
      eraseOneData( _fanin[i].gate() -> _fanout, this );
   }
}
// replace this gate with g (for strash)
void CirGate::replace( CirGate* g ) {
   for ( uint i = 0; i < _fanout.size(); ++i ) {
      g -> _fanout . push_back( _fanout[i] );
      if ( _fanout[i].gate() -> _fanin[0] == this ) {
         _fanout[i].gate() -> _fanin[0] . setGate( g );
      } else {
         _fanout[i].gate() -> _fanin[1] . setGate( g );
      }
   }
   for ( size_t i = 0; i < _fanin.size(); ++i ) {
      eraseOneData( _fanin[i].gate() -> _fanout, this );
   }
}

// === AigGate ===
void AigGate::simulate() {
   _sim.push_back(_fanin[0].getSim() & _fanin[1].getSim());
}
// === POGate ===
void POGate::simulate() {
   //if ( ! _fanin[0].gate() -> simChange() ) return;
   _sim.push_back(_fanin[0].getSim());
}





/*=============== blackbox =================*/
CirGateV 
CirGate::getFanin( unsigned i ) 
{
	assert( i < _fanin.size() );
	return _fanin[i];
}
bool
AigGate::checkXorStruct( CirGate* &a, CirGate* &b, bool &neg )
{
	if ( !_fanin[0].phase() || !_fanin[1].phase()  ) return false;
	
	if ( _fanin[0].gate()->getType() != AIG_GATE ||
	 _fanin[1].gate()->getType() != AIG_GATE ) return false;

	PinList in;

	in.push_back( _fanin[0].gate() -> getFanin(0) );
	in.push_back( _fanin[0].gate() -> getFanin(1) );
	in.push_back( _fanin[1].gate() -> getFanin(0) );
	in.push_back( _fanin[1].gate() -> getFanin(1) );

	if ( in[2].gate() != in[0].gate() ) swap(in[2], in[3]);

	// 0 == 2, 1 == 3, 0.phase != 2, 1.phase != 3
	if ( 	
		in[0].gate() != in[2].gate() ||
		in[1].gate() != in[3].gate() ||
		in[0].phase() == in[2].phase() || 
		in[1].phase() == in[3].phase() 
	) return false;

	if ( in[0].phase() == in[1].phase() ) neg = true;
	else neg = false;

	a = in[0].gate();
	b = in[1].gate();

	return true;

}