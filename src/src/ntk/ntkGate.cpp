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
#include "ntkGate.h"
#include "ntkMgr.h"

using namespace std;

extern      NtkMgr *ntkMgr;
unsigned    NtkGate::_globalFlag = 0;
unsigned 	NtkGate::_globalFlagB = 0;
unsigned    NtkGate::_globalFrontierSize = 0;

NtkGate::NtkGate(uint id, string name, NtkGateType type ) : 
	_id(id), _netId(0), _name(name), _type(type), _level(0), 
	_recCnt(0),
	_mg_list(0),_mg_head(0), _mg_next(0),
	_cmg_list(0),_cmg_head(0), _cmg_next(0),
	_rPair(0), _aigMiterNode(0), _aigNode(0), _muxId(0),
	_flag(0), _flagB(0), mergeFrontierNum(0), _cut_flag(0), 
	_maxScore(0),
	_blackBoxFlag(1),
	_blackBoxInternal(false),
	_mergeFlag(false), _pseudoMerge(false) ,
	_aboveMergeFrontier(false), _isMergeFrontier(false)
{
	if ( _type == NTK_GATE_CONST )
	{
		if ( name.find('0') != string::npos ) _satPhase = false;
		else _satPhase = true;
	}
	else if ( _type == NTK_GATE_NAND ||
		_type == NTK_GATE_OR ||
		_type == NTK_GATE_XNOR ||
		_type == NTK_GATE_NOT ) _satPhase = true;
	else _satPhase = false;
}

void
NtkGate::connectFanin()
{
	NtkGate* g;
	for( size_t i = 0; i < _fanin.size(); i++ )
	{
		g = ntkMgr -> getGate( (size_t)(_fanin[i]) );
// debug
//cout << _name << " adding fanin " <<  g -> getName() << endl;
		_fanin[i] = g;
		g -> _fanout.push_back( this );
	}
}
void
NtkGate::addFaninByName( string &name )
{
	string s = name;
	size_t fanin =ntkMgr->findOrCreateIdByName( s );
	_fanin.push_back( (NtkGate*)fanin );
}

string
NtkGate::getTypeStr() const
{
	switch( _type ) 
	{
		case NTK_GATE_AND: return "and";
		case NTK_GATE_NAND: return "nand";
		case NTK_GATE_OR: return "or";
		case NTK_GATE_NOR: return "nor";
		case NTK_GATE_XOR: return "xor";
		case NTK_GATE_XNOR: return "xnor";
		case NTK_GATE_NOT: return "not";
		case NTK_GATE_BUF: return "buf";
		case NTK_GATE_PI: return "pi";
		case NTK_GATE_CONST: return "const";
		default:	return "default";
	}
}

// printing
void NtkGate::reportGateSimple() const 
{

	cout 
		<< setw(6) << _id;
	
	if ( _aigMiterNode.gate() ) cout << setw(16) << _aigMiterNode.gate()->getID() << dec;
	else cout << setw(16) << "x";
	if ( _aigNode.gate() ) cout <<setw(6) << _aigNode.gate()->getID();
	else cout << "     x";

	cout 
		<< setw(8) << _level
		<< setw(W_NAME) << _name
		<< setw(6) << getTypeStr()
		<< setw(4) << mg_len()
		<< setw(8) << _flag
		<< setw(4) << _cut_flag
		<< setw(3) << _mergeFlag;

	cout << endl;

}

void NtkGate::reportGate() const
{
	cout << "========================================\n\n";
	cout << setw(20) << left << _name << right << 
		setw(5) << getTypeStr() <<
		setw(4) << dec << _level;


	cout << "\n\nFanins: ";
	if ( _type != NTK_GATE_PI )
		for( size_t i = 0; i < _fanin.size(); i++ )
			cout << setw(12) << _fanin[i] -> _name;

	cout << "\nFanouts: ";
	for( size_t i = 0; i < _fanout.size(); i++ )
		cout << setw(12) << _fanout[i] -> _name;

	cout << "\nMerged gates:";
	NtkGate* g = _mg_list;
	while( g ) 
	{
		cout << setw(12) << g -> _name;
		g = g -> _mg_next;
	}

	cout << "\nSim: " << hex << getSim(0) << dec;

	cout << "\n\n========================================" << endl;

}

void NtkGate::postOrderPrint( int level, int indent ) {

	for ( int i = 0; i < indent; i++ )
		cout << "  | ";
	if ( _flag >= _globalFlag ) cout << "  * ";
	else cout << setw(3) << _level << " ";
	cout << _name.substr( 0, _name.size()-SUFFIX_LEN) << " (" << getTypeStr() << ")"
		<< setw(7) << fixed << setprecision(3) << _maxScore
		<< setw(2) << _mergeFlag
		<< endl;

	if ( _flag >= _globalFlag ) return;
	_flag = _globalFlag;
	if ( _level <= level ) return;
	for (size_t i = 0; i < _fanin.size(); ++i) {
		_fanin[i] -> postOrderPrint( level, indent+1 );
	}
}

void 
NtkGate::printUntilMerge( int indent )
{
	for ( int i = 0; i < indent; i++ )
		cout << "  | ";
	if ( _flag >= _globalFlag ) cout << "  * ";
	else cout << setw(3) << _level << " ";
	cout << getNameShort() << " (" << getTypeStr() << ")";

	if ( _flag >= _globalFlag ) 
	{
		cout << endl;
		return;
	}

	_flag = _globalFlag;

	if ( _mg_list )
	{
		cout << " = " << _mg_list -> getNameShort() << endl;
		return;
	}

	cout << endl;
	for (size_t i = 0; i < _fanin.size(); ++i) {
		_fanin[i] -> printUntilMerge( indent+1 );
	}

}


// used when generating netlist
void NtkGate::postOrderVisit( IdList& netList ) {

   if ( _flag >= _globalFlag ) return;

	_flag = _globalFlag;
	for (size_t i = 0; i < _fanin.size(); ++i) {
		_fanin[i] -> postOrderVisit( netList );
	}

	_level = 0;
	if ( _type != NTK_GATE_PI ) 
	{
		for (size_t i = 0; i < _fanin.size(); ++i) {
			if ( _fanin[i] -> _level + 1 > _level )
				_level = _fanin[i] -> _level + 1;
		}
	}
	if ( ntkMgr -> _p_rev && ntkMgr -> _p_gol == this ) 
	{
		if ( _level <= ntkMgr->_p_in -> _level ) _level = ntkMgr->_p_in->_level + 1;
	}
	netList.push_back( _id );
}

void
NtkGate::merge( NtkGate* g, bool global )
{
	NtkGate* head = global ? _mg_head : _cmg_head;
	if ( !head ) mg_create( global );

	if ( g->_old == _old ) mg_append(g, global);
	else mg_appendMerged(g, global);
}


NtkGate* NtkGate::mg_getNext() const { return _mg_next; }
unsigned NtkGate::mg_len() const {

	if ( !_mg_list ) return 0;

   NtkGate* g1 = _mg_list -> mg_getNext();
   uint size=1;
   while( g1 ) {
      g1 = g1 -> mg_getNext();
      size++;
   }
   return size;
}
void
NtkGate::mg_create( bool global )
{
	if ( global )
	{
		_mg_head = this;
		_mg_list = 0;
		_mg_next = 0;
	}
	else
	{
		_cmg_head = this;
		_cmg_list = 0;
		_cmg_next = 0;

	}
}
void
NtkGate::mg_append( NtkGate* g, bool global )
{
	if ( global )
	{
		g -> _mg_list = _mg_list;
		g -> _mg_next = _mg_next;
		g -> _mg_head = this;
		_mg_next = g;
	}
	else
	{
		g -> _cmg_list = _cmg_list;
		g -> _cmg_next = _cmg_next;
		g -> _cmg_head = this;
		_cmg_next = g;
	}
	
}
void
NtkGate::mg_appendMerged( NtkGate* g, bool global )
{
	if ( global )
	{
		if ( _mg_list == 0 )
		{
			g -> mg_create( global );
			g -> _mg_list = this;
			mg_update(g, global);
		}
		else 
		{
			_mg_list -> mg_append( g, global );
		}
	}
	else
	{
		if ( _cmg_list == 0 )
		{
			g -> mg_create( global );
			g -> _cmg_list = this;
			mg_update( g, global );
		}
		else 
		{
			_cmg_list -> mg_append( g, global );
		}
	}
	

}
void
NtkGate::mg_update( NtkGate* g, bool global )
{
	if ( global )
	{
		_mg_list = g;
		if ( _mg_next ) _mg_next -> mg_update(g, global);
	}
	else
	{
		_cmg_list = g;
		if ( _cmg_next ) _cmg_next -> mg_update(g, global);
	}
}
bool
NtkGate::isMergedWith( NtkGate* g ) const
{
	NtkGate* g2 = _mg_list;
	while( g2 )
	{
		if ( g2 == g ) return true;
		g2 = g2 -> _mg_next;
	}
	return false;
}

NtkGate*
NtkGate::getRecPairFanin( NtkGate* fanin )
{
	for( size_t i = 0; i < _recPair_fanin.size(); i++ )
	{
		if ( _fanin[i] == fanin ) return _recPair_fanin[i];
	}
	return 0;
}


void
NtkGate::reportFanoutRecPair(bool detail)
{
	if ( _recPair_fanout.empty() ) return;

	cout << setw(W_NAME-1) << getNameShort() << ":";


	if ( _fanout.empty() )
	{
		cout << setw(W_NAME) << _recPair_fanout[0] -> getNameShort() << setw(5) << "(PO)";
		if ( detail ) cout << endl << endl;
	}
	else if ( detail )
	{

		vector<NtkGate*> rec;

		for( size_t i =0 ; i < _fanout.size(); i++ )
		{
			NtkGate* g = _fanout[i];
			if ( _fanout[i] -> _flagB == _globalFlagB  ) cout << setw(W_NAME) << g -> getNameShort();
			else cout << setw(W_NAME-1) << g -> getNameShort() << "*";

			rec.push_back( _fanout[i] -> getRecPairFanin(this) );
		}

		cout << endl << setw(W_NAME) << "=>";
		for( size_t i = 0; i < rec.size(); i++ )
		{
			if ( rec[i] ) cout << setw(W_NAME) << rec[i] -> getNameShort();
			else cout << setw(W_NAME) << "--"; 
		}
		cout << endl;
	}
	else
	{
		for( size_t i = 0; i < _recPair_fanout.size() ; i++ )
		{
			cout << setw(W_NAME) << _recPair_fanout[i] -> getNameShort();
		}
		if ( _fanout.size() > 0 && _recCnt != _fanout.size() )
		{
			cout << setw(W_NAME) << "(keep)";
		}
	}

	cout << endl;
}

void
NtkGate::outputCutFunc( string f_name, unsigned numPi, unsigned &gateCnt )
{
	ofstream ouf(f_name);


	ouf << "module cutFunc( eco_o";
	for( unsigned i = 0; i < numPi; i++ )
		ouf << ", eco_i" << i;
	ouf << ");\n";
	ouf << "output eco_o;\n";
	ouf << "input eco_i0";
	for( unsigned i = 1; i < numPi; i++ )
		ouf << ", eco_i" << i;
	ouf << ";\n";

	gateCnt = 0;
	_cmg_list = 0;
	_cmg_head = 0;
	_cmg_next = 0;
	outputCutFuncGate( ouf, true, gateCnt );

	ouf << "endmodule\n";
}

void
NtkGate::outputCutFuncGate( ofstream &ouf, bool output, unsigned &gateCnt )
{
	if ( _flag >= _globalFlag - 1 ) return;
	_flag = _globalFlag;

	ouf << getTypeStr()
		<< " eco_g" << gateCnt++
		<< "(";

	if (output) ouf << "eco_o";
	else ouf << getNameShort();

	for( size_t i = 0; i < _fanin.size(); i++ )
	{
		if ( _fanin[i] -> _flag == _globalFlag-1 ) ouf << ", eco_i" << _fanin[i] -> _cutFuncPiId;
		else ouf << ", " << _fanin[i] -> getNameShort();
	}

	ouf << ");\n";

	for( size_t i = 0; i < _fanin.size(); i++ )
		_fanin[i] -> outputCutFuncGate( ouf, false, gateCnt );

}


/*=============== blackbox =================*/

void
NtkGate::checkXorStruct()
{
	CirGate *a, *b;
	bool neg;

	if ( _type == NTK_GATE_XOR || _type == NTK_GATE_XNOR  || _type == NTK_GATE_NOT ) return;


	if ( _aigNode.gate() -> checkXorStruct(a, b, neg) ) 
	{
		// cout << "xor found at " << getName() << endl;
	}
	else return;

	NGateList a_ntk, b_ntk ;

	_globalFlag ++ ;
	for( size_t i = 0; i < _fanin.size(); i++ ) _fanin[i] -> findXorInput( a, b, a_ntk, b_ntk, _xorInter );

	// TODO: check gates wiht inverted fanout (and/nand)

	// cout << inter.size() <<  " gates within xor" << endl;
	// cout << a_ntk.size() << " cands for a / " << b_ntk.size()  << " cands of b " 

	assert( a_ntk.size() == 1 && b_ntk.size() == 1 );
	_xorInputs.push_back( a_ntk[0] );
	_xorInputs.push_back( b_ntk[0] );

	// TODO: check XOR/XNOR
	_blackBoxFlag = 2;

	// if grouped, the inputs is also an internal net
	// if ( _xorInputs[0] -> _blackBoxFlag > 1 ) _xorInputs[0] -> _blackBoxFlag = 0;
	// if ( _xorInputs[1] -> _blackBoxFlag > 1 ) _xorInputs[1] -> _blackBoxFlag = 0;

	cout << _blackBoxFlag << " inputs XOR " << left << setw(W_NAME) << getName()  << right << "(" << _xorInputs[0]->getName() << ", " << _xorInputs[1]->getName() << ")" << endl;

}

void
NtkGate::findXorInput( const CirGate *a, const CirGate *b, NGateList &a_ntk, NGateList &b_ntk, NGateList &inter )
{
	// TODO: add a level thre of finding gates 

	if ( _flag == _globalFlag )  return;
	_flag = _globalFlag;

	if ( _aigNode.gate() == a && _type != NTK_GATE_NOT ) a_ntk.push_back(this);
	else if ( _aigNode.gate() == b && _type != NTK_GATE_NOT ) b_ntk.push_back(this);
	else
	{
		// search fanin
		inter.push_back( this );
		for( size_t i = 0; i < _fanin.size(); i++ )
		{
			_fanin[i] -> findXorInput( a, b, a_ntk, b_ntk, inter );
		}
	}

}

