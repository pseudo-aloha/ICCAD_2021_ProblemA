/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <unordered_map>
#include "../util/util.h"
#include "ntkMgr.h"
#include "ntkGate.h"
#include "cirMgr.h"

using namespace std;

extern NtkMgr*	ntkMgr;
extern CirMgr*	aigOld;
extern CirMgr*	aigGol;
extern CirMgr*	aigMiter;

extern bool debug_n;
extern bool debug_a;

extern int time_threshold;
extern int total_time_threshold;
extern Timer timer_findsimilar;
extern Timer timer_total;

extern CirMgr*	aigSel;

const char*			SUFFIX_OLD = "_O";
const char*			SUFFIX_GOL = "_G";

extern "C"
{
	typedef struct Abc_Frame_t_ Abc_Frame_t;
	int    Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand );
}
extern Abc_Frame_t * pAbc;
static char Command[1024];
size_t NtkMgr::_mg_count = 0;
bool abc_eq;
bool abc_read;

// different parameter array
int		_sim_num_arr[7] = {1, 2, 3, 4, 5, 7, 10};
float	_sim_thres_arr[6] = { 1.0, 0.9, 0.8, 1.1, 1.2, 0.6 };
int     _lv_arr[5] = {2, 4, 8, 11, 15};

int		_pre_sim_num_arr[4] = {1, 3, 5, 10 };
float	_pre_sim_thres_arr[2] = { 0.9, 1.0 };
int     _pre_lv_arr[3] = {2, 5, 12};

int NeqNum = 0;
int totalNum = 0;

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/

NtkMgr::NtkMgr()
{
	reset();
	
	sprintf( _fname_patch_tmp, ".patch.tmp.v" );
	sprintf( _fname_patched, ".patched.v" );
	sprintf( _fname_p, ".p.v" );
	sprintf( _fname_ped, ".ped.v" );

}
NtkMgr::~NtkMgr()
{
	reset();
}
NtkGate *NtkMgr::getGate(unsigned gid) const
{
	// TODO add this back after verilog read
	//if (gid > _numVar + _numPO || !_gateList[gid] || _gateList[gid]->getType() == NTK_GATE_UNDEF)
		//return 0;
	return _gateList[gid];
}
void
NtkMgr::createConst()
{
	NtkGate *g1, *g2;
	unsigned id;
	string name;

	name = "1'b0"; name += SUFFIX_OLD;
	id = findOrCreateIdByName( name );
	assert(getGate(id)==0);
	g1 = createGate( id, name, NTK_GATE_CONST );

	name = "1'b0"; name += SUFFIX_GOL;
	id = findOrCreateIdByName( name );
	g2 = createGate( id, name, NTK_GATE_CONST );


	name = "1'b1"; name += SUFFIX_OLD;
	id = findOrCreateIdByName( name );
	g1 = createGate( id, name, NTK_GATE_CONST );

	name = "1'b1"; name += SUFFIX_GOL;
	id = findOrCreateIdByName( name );
	g2 = createGate( id, name, NTK_GATE_CONST );
}
bool 
NtkMgr::constructCircuit()
{
	// construct gates:
	// for PIs and gates, create gate
	// for POs, add id to _POList
	// find and store fanin's id in fanin

	// after all gates are constructed
	// change the id in fanin to actual gate poiter
	// also handle the fanout of fanin gates

	// create constant gates
	createConst();

	// connect actual fanins
	for( size_t i = 0; i < _gateList.size(); i++ )
		_gateList[i] -> connectFanin();

	NtkGate *dummyPO;
	string dummyNamePre = "eco_dummy";
	for( size_t i = 0; i < _POList_old.size(); i++ )
	{
		string dummyName = dummyNamePre + to_string((long long unsigned)i) + "_O";
		unsigned dummyId = findOrCreateIdByName( dummyName );
		dummyPO = createGate( dummyId, dummyName, NTK_GATE_BUF );
		dummyPO -> _fanin.push_back( getGate(_POList_old[i]) );
		getGate(_POList_old[i]) -> _fanout.push_back( dummyPO );
		_dummyPOList.push_back( dummyPO );
	}


	_numPO = _POList_old.size();
	assert( _numPO == _POList_gol.size() );
	_numPI = _PIList_old.size();

	// generate topological order netlist
	genNetlist();
	for( size_t i = 0; i < _netList_old.size(); i++ )
		getGate(_netList_old[i]) -> _netId = i;
	for( size_t i = 0; i < _netList_gol.size(); i++ )
		getGate(_netList_gol[i]) -> _netId = i;

	// move level towards PO
	/*
	for( int i = _netList_old.size()-1; i >= 0; i-- )
		getGate( _netList_old[i]) -> adjustLevel();
	for( int i = _netList_gol.size()-1; i >= 0; i-- )
		getGate( _netList_gol[i]) -> adjustLevel();
	*/

	// debug
	cout << "Ntk construction done" << endl;

	return true;
}
void
NtkGate::adjustLevel()
{
	if ( _fanout.empty() ) return;
	int	level = INT_MAX;
	for( size_t i = 0; i < _fanout.size(); i++ )
	{
		if ( _fanout[i] -> _level - 1 < level ) level = _fanout[i] -> _level - 1;
	}
	_level = level;
}

void
NtkMgr::markMergeCone()
{
	NtkGate *g;
	for( int i = _netList_old.size()-1; i >= 0; i-- )
	{
		g = _gateList[_netList_old[i]];

		if ( g -> _mg_list ) 
		{
			g-> _mergeFlag = true;
			continue;
		}
		for( size_t j = 0; j < g->_fanout.size(); j++ )
			if ( g->_fanout[j] -> _mergeFlag ) 
			{
				g-> _mergeFlag = true;
				break;
			}
	}
	for( size_t i = 0; i < _netList_gol.size(); i++ )
	{
		if ( getGate(_netList_gol[i]) -> _mg_list )
		{
			getGate(_netList_gol[i]) -> _mergeFlag = true;
		}
	}
}
void 
NtkMgr::genNetlist()
{

	// init
	NtkGate::_globalFlag += 1;
	_netList_old.clear();
	_netList_gol.clear();
	NtkGate* g;
	string name;

	// const
	name = "1'b0"; name += SUFFIX_OLD;
	g = findGateByName( name );
	g-> postOrderVisit( _netList_old );
	name = "1'b1"; name += SUFFIX_OLD;
	g = findGateByName( name );
	g-> postOrderVisit( _netList_old );

	name = "1'b0"; name += SUFFIX_GOL;
	g = findGateByName( name );
	g-> postOrderVisit( _netList_gol );

	name = "1'b1"; name += SUFFIX_GOL;
	g = findGateByName( name );
	g-> postOrderVisit( _netList_gol );

	if ( _p_rev && _p_gol && _p_in)
	{
		_p_in -> postOrderVisit( _netList_old );
	}

	for( size_t i = 0; i < _PIList_old.size(); i++ )
	{
		getGate(_PIList_old[i]) -> postOrderVisit( _netList_old );
	}
	for( size_t i = 0; i < _PIList_gol.size(); i++ )
	{
		getGate(_PIList_gol[i]) -> postOrderVisit( _netList_gol );
	}

	// gates
	for (size_t i=0; i<_numPO; i++)
	{
		getGate(_POList_old[i]) -> postOrderVisit( _netList_old );
	}
	for (size_t i=0; i<_numPO; i++)
	{
		getGate(_POList_gol[i]) -> postOrderVisit( _netList_gol );
	}


}
void 
NtkMgr::reset()
{
	for (size_t i = 0; i < _gateList.size(); ++i)
	{
		if (_gateList[i])
		{
			delete _gateList[i];
		}
	}

	_numVar = 0;
	_numPI = 0;
	_numLatch = 0;
	_numPO = 0;
	_numOld = 0;
	_numGol = 0;

	_gateList.clear();
	_PIList_old.clear();
	_PIList_gol.clear();
	_POList_old.clear();
	_POList_gol.clear();
	_netList_old.clear();
	_netList_gol.clear();
	_merge_frontier.clear();
	_dummyPOList.clear();

	_recPairList.clear();
	_bwdPatchList.clear();

	_netCnt = 0;
	_numMux = 0;
	_cost = 0;

	_name2Id.clear();
	_mg_heads.clear();
	_cmg_heads.clear();

	NtkGate::_globalFlag = 0;
	NtkGate::_globalFlagB = 0;

	_bwdPatchMap.clear();

	_sim_count = 0;
	_read = false;

	_p_rev = 0;
	_p_gol = 0;
	_p_in = 0;


	if ( aigMiter ) 
	{
		delete aigMiter;
		aigMiter = 0;
	}
	if ( aigOld ) 
	{
		delete aigOld;
		aigOld = 0;
	}
	if ( aigGol )
	{
		delete aigGol;
		aigGol = 0;
	} 
	if ( aigSel ) 
	{
		delete aigSel;
		aigSel = 0;
	} 
}

unsigned
NtkMgr::findOrCreateIdByName( string &name )
{
	unsigned id;
	unordered_map<string, unsigned>::iterator it = _name2Id.find( name );

	if ( it == _name2Id.end() )
	{
		// create a new id
		id = _gateList.size();
		_name2Id[name] = id;

		// put a nullptr into the gatelist tempararily
		// the actual gate will be create later
		_gateList.push_back(0);

		return id;

	}
	else return it->second;
}
NtkGate*
NtkMgr::createGate( unsigned id, string &name, NtkGateType type )
{
	NtkGate* g;

	if ( _gateList.size() < id+1 ) _gateList.resize( id+1 );

	switch( type ) 
	{
		case NTK_GATE_AND: 		g = new NtkGateAnd( id, name );break;
		case NTK_GATE_NAND: 	g = new NtkGateNand( id, name );break;
		case NTK_GATE_OR: 		g = new NtkGateOr( id, name );break;
		case NTK_GATE_NOR: 		g = new NtkGateNor( id, name );break;
		case NTK_GATE_XOR: 		g = new NtkGateXor( id, name );break;
		case NTK_GATE_XNOR:		g = new NtkGateXnor( id, name );break;
		case NTK_GATE_NOT: 		g = new NtkGateNot( id, name );break;
		case NTK_GATE_BUF: 		g = new NtkGateBuf( id, name );break;
		case NTK_GATE_PI: 		g = new NtkGatePi( id, name );break;
		case NTK_GATE_CONST: 	g = new NtkGateConst( id, name );break;
		default: 				g = new NtkGateAnd( id, name );break;
	}

	_gateList[id] = g;

	string old = name.substr(name.size()-SUFFIX_LEN, SUFFIX_LEN);
	if ( !old.compare(SUFFIX_OLD) ) 
	{
		_numOld ++;
		g->_old = true;
	}
	else
	{
		_numGol ++;
		g->_old = false;
	}

	switch (type)
	{
	case NTK_GATE_PI :
		if (g->_old) _PIList_old.push_back( id );
		else _PIList_gol.push_back( id );
		break;
	default:
		break;
	}

	return g;
}
NtkGate*
NtkMgr::findGateByName( string name )
{
	unsigned id;
	unordered_map<string, unsigned>::iterator it = _name2Id.find( name );
	if ( it == _name2Id.end() ) 
	{
		//cout << "gate " << name << " not found!!" << endl;
		return 0;
	}
	id = it->second;
	return getGate(id);

}

extern "C"
{
void
eco_mergeGate( char* name, size_t driver, unsigned complement, int flag )
{
	if ( flag == -2 )	// net
	{
		string s = name;
		ntkMgr -> mapGate2Aig( name, (CirGate*)driver, complement, CIR_CF );
	}

	// for ntk construction and merging
	// currently use .miter.aag instead
	/*
	if ( flag == -2 )	// net
	{
		string s = name;
		ntkMgr->mapGate2Aig( name, (CirGate*)driver, complement, CIR_MITER );
	}
	else if ( flag == -1 )	// constant
	{
		// in ABC, the constant node is const1
		NtkGate *g;
		g = ntkMgr->findGateByName( "1'b0_O");
		ntkMgr -> mapGate2Aig( g->getName(), (CirGate*)driver, true, CIR_MITER );
		g = ntkMgr->findGateByName( "1'b1_O");
		ntkMgr -> mapGate2Aig( g->getName(), (CirGate*)driver, false, CIR_MITER );
		g = ntkMgr->findGateByName( "1'b0_G");
		ntkMgr -> mapGate2Aig( g->getName(), (CirGate*)driver, true, CIR_MITER );
		g = ntkMgr->findGateByName( "1'b1_G");
		ntkMgr -> mapGate2Aig( g->getName(), (CirGate*)driver, false, CIR_MITER );

	}
	else	// PI with flag as it's index
	{
		NtkGate *g;
		g = ntkMgr->getPI( flag, true );
		ntkMgr -> mapGate2Aig( g->getName(), (CirGate*)driver, false, CIR_MITER );
		g = ntkMgr->getPI( flag, false );
		ntkMgr -> mapGate2Aig( g->getName(), (CirGate*)driver, false, CIR_MITER );
	}
	*/
}
size_t 
eco_createGate( char* name  ,unsigned type, unsigned old )
{
	unsigned id;
	string s = name;
	NtkGate* g;

	if ( old ) s += SUFFIX_OLD;
	else s += SUFFIX_GOL;


	/*
	if ( NTK_GATE_PI == (NtkGateType)type )  // map both old and gold to old
	{
		id = ntkMgr->findOrCreatePIByName( s );
		if ( old ) g = ntkMgr -> createGate( id, s, (NtkGateType)type );
		else g = ntkMgr->getGate(id);
	}
	*/
	id = ntkMgr->findOrCreateIdByName( s );
	g = ntkMgr -> createGate( id, s, (NtkGateType)type );

// debug
//cout << "creating gate " << s << "\t(" << g->getTypeStr() << ")" << endl;

	return (size_t)g;
}
void eco_addFanin( size_t pEcoGate, char* fanin )
{

	string s = fanin;
	if ( ((NtkGate*)pEcoGate) -> isOld() ) s += SUFFIX_OLD;
	else s += SUFFIX_GOL;

// debug
//cout << ((NtkGate*)pEcoGate) -> getName() << " add fanin " << s << endl;

	((NtkGate*)pEcoGate) -> addFaninByName( s );

}
void eco_addPO( char* name, unsigned old )
{
	string s = name;
	if ( old ) s += SUFFIX_OLD;
	else s += SUFFIX_GOL;

	ntkMgr -> addPO( ntkMgr -> findOrCreateIdByName(s), old );

}
void eco_eq()
{
	abc_eq = true;
}
void eco_read()
{
	abc_read = true;
}
}
void NtkMgr::addPO( unsigned id, bool old )
{
	if ( old ) _POList_old.push_back( id );
	else _POList_gol.push_back(id);

// debug
// cout << "add PO\n" << id;
}

void
NtkMgr::mapGate2Aig( string name, CirGate* aigNode, bool complement, CirType cirType )
{

	NtkGate* g = findGateByName( name );
	if ( !g ) {
		//cout << "mpaGate2Aig: gate " << name << " not found\n";
		return;
	}


	size_t key = ( (size_t)aigNode | (complement?01:00) );
	bool old = ( name.substr(name.size()-SUFFIX_LEN, SUFFIX_LEN).compare(SUFFIX_GOL) );

	if ( cirType == CIR_MITER ) 
		g -> _aigMiterNode = CirGateV( aigNode, complement );
	else if ( cirType == CIR_GOL || cirType == CIR_OLD )
		g -> _aigNode = CirGateV( aigNode, complement );

	if ( cirType != CIR_CF && cirType != CIR_MITER ) return;

	bool global = (cirType == CIR_MITER);

	unordered_map<size_t, unsigned> &map = global ? _mg_heads : _cmg_heads;
	unordered_map<size_t, unsigned>::iterator it = map.find( key );

	// disable prepatch gate's merging
	if ( name.substr(0, 5) == "eco_w" ) return;

	if ( it == map.end()  )	// not found, create new group
	{

		// debug
		// cout << "new group from ";
		// cout << g->_name << " with key " << hex << key << dec << endl;

		map[key] =  g->getId();
		g -> mg_create( global );
		return;
	}
	else	// found a existing group
	{
		_mg_count++;
		getGate(it->second) -> merge(g, global);
	}

	return;
}

void 
NtkMgr::printSummary() const 
{
	cout << "Number of PIs:\t" << _numPI << endl;
	cout << "Number of POs:\t" << _numPO << endl;
	cout << "Number of gates in old:\t" << _numOld << endl;
	cout << "Number of gates in golden:\t" << _numGol << endl;
}

void
NtkMgr::printNetlist() const
{
	// TODO

	cout << "Net list" << endl;
	cout 
		<< "    id"
		<< setw(16) << "mg group"
		<< "   aig"
		<< "      lv"
		<< setw(W_NAME) << "name"
		<< "  type"
		<< " #mg"
		<< "    flag"
		<< " cut"
		<< " mg"
		<< endl;

	for( size_t i = 0; i < _netList_old.size(); i++ )
	{
		getGate( _netList_old[i] ) -> reportGateSimple();
	}
	for( size_t i = 0; i < _netList_gol.size(); i++ )
	{
		getGate( _netList_gol[i] ) -> reportGateSimple();
	}
}

void 
NtkMgr::printMgList() const
{

	// TODO
	// for debug ?
	
}

// write output patch
/*
NTK_GATE_AND,
NTK_GATE_NAND,
NTK_GATE_OR,
NTK_GATE_NOR,
NTK_GATE_XOR,
NTK_GATE_XNOR,
NTK_GATE_NOT,
NTK_GATE_BUF
*/
/*
void        
NtkMgr::GetInputPatch(NtkGate* g_gold, NGateList& input_name, NGateList& wire_name)
{
	if ((g_gold->_mg_list != 0) || (g_gold->_type == NTK_GATE_PI))
	{
		string _tmp_name = g_gold->_name;
		if ((g_gold->_mg_list != 0) && (g_gold->_rPair != 0)) { _tmp_name = g_gold->_name + "_in"; }
		input_name.push_back(_tmp_name);
		return;
	}
	else 
	{
		wire_name.push_back(g_gold->_name);
		for (size_t i = 0 ; i < g_gold->_fanin.size() ; ++i) { GetInputPatch(g_gold->_fanin[i], input_name, wire_name); }
	}
}
*/

// void
// NtkMgr::test()
// {
// 	NtkGate *g1, *g2;
	
// 	g1 = findGateByName("m_O");
// 	g1 -> reportGate();

// 	g2 = findGateByName("n_O");
// 	g2 -> reportGate();

// 	g1 = findGateByName("m_G");
// 	g1 -> reportGate();

// 	g1 = findGateByName("n_G");
// 	g1 -> reportGate();
// }

// added by HugoAloha:))
void
NtkMgr::MarkFaninConeTraverse(NtkGate* gate, int level_limit, size_t& mg_amount){
	if ((gate->_flagB == gate->_globalFlagB - 1) || (gate->_level < level_limit)) return;
	gate->_flagB = gate->_globalFlagB - 1;
	if(gate -> _mg_list) mg_amount++;
	for (size_t i = 0 ; i < gate->_fanin.size() ; ++i) {
		MarkFaninConeTraverse(gate->_fanin[i], level_limit, mg_amount);
	}
}

void		
NtkMgr::CountRepeatedMgGate(NtkGate* gate, int level_limit, int& same_merged, size_t& mg_amount){
	// gate->_mg_next != NULL
	if (gate->_flagB == gate->_globalFlagB) return;
	gate->_flagB = gate->_globalFlagB;
	if(gate -> _mg_list)
	{
		mg_amount++;
		NtkGate* tmpGate = gate->_mg_list;
		while( tmpGate )
		{

			//cout << tmpGate -> getName() << endl;

			if(tmpGate -> _flagB == tmpGate -> _globalFlagB) return;
			if(tmpGate -> _flagB == tmpGate -> _globalFlagB - 1)
			{
				same_merged++;
				tmpGate -> _flagB = tmpGate -> _globalFlagB;
				return;
			}
			tmpGate = tmpGate -> _mg_next;
		}
	}
	for (size_t i = 0 ; i < gate->_fanin.size() ; ++i) {
		CountRepeatedMgGate(gate->_fanin[i], level_limit, same_merged, mg_amount);
	}
}



void
NtkMgr::getMergeFrontier()
{
	bool flag;
	NtkGate* tmp;
	NtkGate::increaseGlobalFlag();
	// find merge frontier
	for(size_t i = 0; i < _POList_old.size(); ++i)
	{
		getGate(_POList_old[i]) -> _aboveMergeFrontier = true;
	}
	for(int i = _netList_old.size() - 1; i >= 0; --i)
	{
		if(getGate(_netList_old[i]) -> _aboveMergeFrontier == true) continue;
		for(size_t j = 0; j < getGate(_netList_old[i]) -> _fanout.size(); ++j)
		{
			if(getGate(_netList_old[i]) -> _fanout[j] -> _aboveMergeFrontier && !(getGate(_netList_old[i]) -> _fanout[j] -> _mg_list))
			{
				getGate(_netList_old[i]) -> _aboveMergeFrontier = true;
				break;
			}
		}
	}
	// for(size_t i = 0; i < _netList_old.size(); ++i)
	// {
	// 	if(!getGate(_netList_old[i]) -> _aboveMergeFrontier)
	// 	{
	// 		for(size_t j = 0; j < getGate(_netList_old[i]) -> getFanoutList().size(); ++j)
	// 		{
	// 			if(getGate(_netList_old[i]) -> getFanoutList()[j] -> _aboveMergeFrontier && getGate(_netList_old[i]) -> getFanoutList()[j] -> _flag != NtkGate::_globalFlag)
	// 			{
	// 				getGate(_netList_old[i]) -> getFanoutList()[j] -> _flag = NtkGate::_globalFlag;
	// 				getGate(_netList_old[i]) -> getFanoutList()[j] -> mergeFrontierNum = NtkGate::_globalFrontierSize;
	// 				getGate(_netList_old[i]) -> getFanoutList()[j] -> _isMergeFrontier = true;
	// 				++NtkGate::_globalFrontierSize;
	// 			}
	// 		}
	// 	}
	// }
	// for(size_t i = 0; i < _PIList_old.size(); ++i)
	// {
	// 	if(getGate(_PIList_old[i]) -> _aboveMergeFrontier && getGate(_PIList_old[i]) -> _flag != NtkGate::_globalFlag)
	// 	{
	// 		getGate(_PIList_old[i]) -> _flag = NtkGate::_globalFlag;
	// 		// cout << getGate(_PIList_old[i]) -> getName() << endl;
	// 		getGate(_PIList_old[i]) -> _isMergeFrontier = true;
	// 		getGate(_PIList_old[i]) -> mergeFrontierNum = NtkGate::_globalFrontierSize;
	// 		// cout << "mm : " << getGate(_PIList_old[i]) -> getName() << endl;
	// 		++NtkGate::_globalFrontierSize;
	// 	}
	// }
	// fing merge frontier in gold circuit
	for(size_t i = 0; i < _POList_gol.size(); ++i)
	{
		getGate(_POList_gol[i]) -> _aboveMergeFrontier = true;
	}
	for(int i = _netList_gol.size() - 1; i >= 0; --i)
	{
		if(getGate(_netList_gol[i]) -> _aboveMergeFrontier == true) continue;
		for(size_t j = 0; j < getGate(_netList_gol[i]) -> _fanout.size(); ++j)
		{
			if(getGate(_netList_gol[i]) -> _fanout[j] -> _aboveMergeFrontier && !(getGate(_netList_gol[i]) -> _fanout[j] -> _mg_list))
			{
				getGate(_netList_gol[i]) -> _aboveMergeFrontier = true;
				break;
			}
		}
	}
	for(size_t i = 0; i < _netList_gol.size(); ++i)
	{
		if(!getGate(_netList_gol[i]) -> _aboveMergeFrontier)
		{
			for(size_t j = 0; j < getGate(_netList_gol[i]) -> getFanoutList().size(); ++j)
			{
				flag = true;
				if(getGate(_netList_gol[i]) -> getFanoutList()[j] -> _aboveMergeFrontier)
				{	
					tmp = getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_list -> _mg_next;
					if(tmp)
					{
						while(tmp)
						{
							if(tmp -> _isMergeFrontier)
							{
								tmp -> _isMergeFrontier = true;
							}
							tmp = tmp -> _mg_next;
						}
					}
					if(getGate(_netList_gol[i]) -> getFanoutList()[j] -> _flag != NtkGate::_globalFlag)
					{
						getGate(_netList_gol[i]) -> getFanoutList()[j] -> _flag = NtkGate::_globalFlag;
						getGate(_netList_gol[i]) -> getFanoutList()[j] -> _isMergeFrontier = true;
						getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_list -> _isMergeFrontier = true;
						getGate(_netList_gol[i]) -> getFanoutList()[j] -> mergeFrontierNum = NtkGate::_globalFrontierSize;
						getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_list -> mergeFrontierNum = NtkGate::_globalFrontierSize;
						tmp = getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_list -> _mg_next;
						if(tmp)
						{
							while(tmp)
							{
								if(tmp -> _isMergeFrontier)
								{
									tmp -> _isMergeFrontier = true;
									tmp -> mergeFrontierNum = NtkGate::_globalFrontierSize;
								}
								tmp = tmp -> _mg_next;
							}
						}
						// cout << getGate(_netList_gol[i]) -> getFanoutList()[j] -> getName() << " " << NtkGate::_globalFrontierSize << endl;
						NtkGate::_globalFrontierSize++;
					}
					// cout << "yen-ju-Lee: " << getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_list -> _isMergeFrontier << " " << getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_list -> getName() << endl;
					// if(getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_list -> _isMergeFrontier)
					// {
					// 	getGate(_netList_gol[i]) -> getFanoutList()[j] -> mergeFrontierNum = getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_list -> mergeFrontierNum;
					// 	flag = false;
					// }
					// else
					// {
					// 	tmp = getGate(_netList_gol[i]) -> getFanoutList()[j] -> _mg_next;
					// 	if(tmp)
					// 	{
					// 		while(tmp)
					// 		{
					// 			// cout << "yen-ju-Lee: " << tmp -> _isMergeFrontier << " " << tmp -> getName() << endl;
					// 			if(tmp -> _isMergeFrontier)
					// 			{
					// 				getGate(_netList_gol[i]) -> getFanoutList()[j] -> mergeFrontierNum = tmp -> mergeFrontierNum;
					// 				flag = false;
					// 			}
					// 			tmp = tmp -> _mg_next;
					// 		}
					// 	}
					// }
				}
			}
		}
	}
	for(size_t i = 0; i < _PIList_gol.size(); ++i)
	{
		if(getGate(_PIList_gol[i]) -> _aboveMergeFrontier)
		{
			// if(getGate(_PIList_gol[i]) -> _mg_list -> _isMergeFrontier)
			// {
			// 	getGate(_PIList_gol[i]) -> mergeFrontierNum = getGate(_PIList_gol[i]) -> _mg_list -> mergeFrontierNum;
			// }
			// else
			// {
			if(getGate(_PIList_gol[i]) -> _flag != NtkGate::_globalFlag)
			{
				getGate(_PIList_gol[i]) -> _flag = NtkGate::_globalFlag;
				getGate(_PIList_gol[i]) -> _isMergeFrontier = true;
				getGate(_PIList_gol[i]) -> _mg_list -> _isMergeFrontier = true;
				getGate(_PIList_gol[i]) -> mergeFrontierNum = NtkGate::_globalFrontierSize;
				getGate(_PIList_gol[i]) -> _mg_list -> mergeFrontierNum = NtkGate::_globalFrontierSize;
				tmp = getGate(_PIList_gol[i]) -> _mg_list -> _mg_next;
				if(tmp)
				{
					while(tmp)
					{
						if(tmp -> _isMergeFrontier)
						{
							tmp -> _isMergeFrontier = true;
							tmp -> mergeFrontierNum = NtkGate::_globalFrontierSize;
						}
						tmp = tmp -> _mg_next;
					}
				}
				++NtkGate::_globalFrontierSize;
			}
			// }
		}
	}
}

void
NtkMgr::visitFaninCone(NtkGate* gate, NtkGate* original_gate)
{
	assert( gate -> _structuralVec.size() == NtkGate::_globalFrontierSize);
	if(gate -> _flag == NtkGate::_globalFlag) return;
	gate -> _flag = NtkGate::_globalFlag;
	if(gate != original_gate && gate -> _isMergeFrontier)
	{
		// cout << original_gate -> getName() << " " << gate -> mergeFrontierNum << endl;
		original_gate -> _structuralVec[gate -> mergeFrontierNum] = true;
	}
	if(gate -> _type == NTK_GATE_PI)
	{
		return;
	}
	for(size_t i = 0; i < gate -> _fanin.size(); ++i)
	{
		visitFaninCone(gate -> _fanin[i], original_gate);
	}
}

void
NtkMgr::setStructuralVector()
{
	for(size_t i = 0 ; i < _netList_old.size() ; ++i)
	{
		NtkGate::increaseGlobalFlag();
		// Initialize _structuralVec with (mergeFrontierNum) 0
		for(size_t j = 0 ; j < NtkGate::_globalFrontierSize ; ++j)
		{
			getGate(_netList_old[i])->_structuralVec.push_back(0);
		}
		// Set Structural Vector
		visitFaninCone(getGate(_netList_old[i]), getGate(_netList_old[i]));
	}
	for(size_t i = 0 ; i < _netList_gol.size() ; ++i)
	{
		NtkGate::increaseGlobalFlag();
		// Initialize _structuralVec with (mergeFrontierNum) 0
		for(size_t j = 0 ; j < NtkGate::_globalFrontierSize ; ++j)
		{
			getGate(_netList_gol[i])->_structuralVec.push_back(0);
		}
		// Set Structural Vector
		visitFaninCone(getGate(_netList_gol[i]), getGate(_netList_gol[i]));
	}
}

float
NtkMgr::getStructuralSimilarity(NtkGate* g1, NtkGate* g2)
{
	size_t counter = 0;
	for(size_t i = 0; i < NtkGate::_globalFrontierSize; ++i)
	{
		// cout << g1 -> _structuralVec[i] << " " << g2 -> _structuralVec[i] << endl;
		if( g1 -> _structuralVec[i] && g2 -> _structuralVec[i])	
		{
			++counter;
		}
	}
	// cout << "g1 = " << g1->getName() << " ; g2 = " << g2->getName() << " ; counter = " << counter << " ; structural_sim = " << float(counter)/float(NtkGate::_globalFrontierSize) << endl;
	return float(counter)/float(NtkGate::_globalFrontierSize);
}

// void
// NtkMgr::findSimilarPairs()
// {
// 	float sim = 0, sim_struct = 0;
// 	size_t count = 0;
// 	CutPair cutPairs;
// 	NtkGatePQ rPairs_found;
// 	cout << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
// 	for(size_t i = 0; i < _netList_old.size(); ++i)
// 	{
// 		for(size_t j = 0; j < _netList_gol.size(); ++j)
// 		{
// 			if(getGate(_netList_old[i]) -> _type != NTK_GATE_PI && getGate(_netList_gol[j]) -> _type != NTK_GATE_PI && getGate(_netList_old[i]) -> _type != NTK_GATE_CONST && getGate(_netList_gol[j]) -> _type != NTK_GATE_CONST && !(getGate(_netList_gol[j]) -> _mg_list))
// 			{
// 				computeSimilarity(getGate(_netList_old[i]), getGate(_netList_gol[j]), sim, sim_struct);
// 				if(sim > 0.95) 
// 				{
// 					cout << getGate(_netList_old[i]) -> getName() << " " << getGate(_netList_gol[j]) -> getName() << " sim : " << sim << endl;
// 					cutPairs = cutMatching(getGate(_netList_old[i]), getGate(_netList_gol[j]));
// 					if(cutPairs.goldCutIdList.size() > 0) cout << "hello : " <<  cutPairs.goldCutIdList.size() << endl;
// 					count++;
// 					IdList &oldList = cutPairs.oldCutIdList;
// 					IdList &golList = cutPairs.goldCutIdList;
			
// 					for( size_t i = 0; i < oldList.size(); i ++ )
// 					{
// 						getGate(oldList[i]) -> addRecPair( getGate(golList[i]), rPairs_found );
// 					}
// 				}
// 			}
// 		}
// 	}
// 	cout << "count = " << count << endl;
// 	cout << "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=" << endl;
// }



void
NtkMgr::setupRevAndGol( char *fname_rev, char *fname_gol )
{
	if ( _read ) return;

	sprintf( Command, "eco_setFlag -ps" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "miter %s %s", fname_rev, fname_gol ); Cmd_CommandExecute( pAbc, Command );
	constructCircuit();
	sprintf( Command, "fraig" ); Cmd_CommandExecute( pAbc, Command );

	// read miter
	aigMiter = new CirMgr();
	sprintf( Command, "write_aiger -s .miter.aig" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "eco_setFlag" ); Cmd_CommandExecute( pAbc, Command );
	assert( !system("./aigtoaig .miter.aig .miter.aag"));
	assert( !system("rm .miter.aig") );
	aigMiter -> readCircuit( ".miter.aag" , CIR_MITER);
	//aigMiter->printSummary();

	// read old aig
	sprintf( Command, "read_verilog %s", fname_rev ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "eco_net2po" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "strash" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "write_aiger -s .g1.aig" ); Cmd_CommandExecute( pAbc, Command );
	assert( !system("./aigtoaig .g1.aig .g1.aag") );
	assert( !system("rm .g1.aig") );
	// TODO: use reset
	aigOld = new CirMgr();
	aigOld -> readCircuit( ".g1.aag", CIR_OLD);


	// read gol aig
	sprintf( Command, "read_verilog %s", fname_gol ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "eco_net2po -g" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "strash" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "write_aiger -s .r2.aig" ); Cmd_CommandExecute( pAbc, Command );
	assert( !system("./aigtoaig .r2.aig .r2.aag") );
	assert( !system("rm .r2.aig") );
	aigGol = new CirMgr();
	aigGol -> readCircuit( ".r2.aag", CIR_GOL);

	// init ntk
	markMergeCone(); 	// mark fanin cone of merged gates
	getMergeFrontier();
	setStructuralVector();
	checkXorStruct();


	if ( debug_n ) printNetlist();
	
	_read = true;

}
bool compareCutPairLevel( CutPair a, CutPair b)
{
	return a.level > b.level;
}

void
NtkMgr::clearCFMergeInfo()
{
	_cmg_heads.clear();
	for( size_t k = 0; k < _netList_old.size(); k++ )
	{
		getGate( _netList_old[k] ) -> _cmg_head = 0;
		getGate( _netList_old[k] ) -> _cmg_list = 0;
		getGate( _netList_old[k] ) -> _cmg_next = 0;
	}
	for( size_t k = 0; k < _netList_gol.size(); k++ )
	{
		getGate( _netList_gol[k] ) -> _cmg_head = 0;
		getGate( _netList_gol[k] ) -> _cmg_list = 0;
		getGate( _netList_gol[k] ) -> _cmg_next = 0;
	}
}

void
NtkMgr::resetPatchRecord(patchRecord &record)
{
	record.patchArr.clear();
	record.patchRecFanout.clear();
	record.pAndPPrime.clear();
}

int
NtkMgr::existInPatchArr(patchRecord &record, NtkGate *g)
{
	for(size_t i = 0; i < record.patchArr.size(); ++i)
	{
		if(findGateByName(record.patchArr[i]) == g) return i;
	}
	return -1;
}



bool ComparepatchPairStr::operator() ( patchPairStr &a, patchPairStr &b )
{
	return a._mg_count < b._mg_count;
}
void
NtkMgr::findSimilarPairs(char* fname_old, char* fname_gol)
{
	float sim = 0, sim_struct = 0;
	bool flag = false;
	size_t count = 0, original_mg_count = 0, max_mg_count = 0;
	CutPair cutPairs;
	CutPairList cutPairList;
	NtkGatePQ rPairs_found;
	char Command[1000];
	patchPair tmpPatchPair;
	patchPairStr tmpPatchPairStr;
	vector<patchPair> patchPairVec;
	patchPairQ bestPatchPair;
	_mg_count = 0;
	sprintf( Command, "miter %s %s", fname_gol, fname_old ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "fraig" ); Cmd_CommandExecute( pAbc, Command );
	cout << "original _mg_count = " << _mg_count << endl;
	original_mg_count = _mg_count;
	timer_findsimilar.start();
	bool time_exceed = false;
	for(size_t i = 0; i < _netList_old.size(); ++i)
	{
		if(getGate(_netList_old[i]) -> _mergeFlag == 0)
		{
			for(size_t j = 0; j < _netList_gol.size(); ++j)
			{
				if(getGate(_netList_old[i]) -> _type != NTK_GATE_PI && getGate(_netList_gol[j]) -> _type != NTK_GATE_PI && getGate(_netList_old[i]) -> _type != NTK_GATE_CONST && getGate(_netList_gol[j]) -> _type != NTK_GATE_CONST && !(getGate(_netList_gol[j]) -> _mg_list))
				{
					computeSimilarity(getGate(_netList_old[i]), getGate(_netList_gol[j]), sim, sim_struct);
					if( sim > 0.9 && ( sim - sim_struct ) > 0.6 )
					// if( sim - sim_struct > 0.9  )	// test2 error
					{
						tmpPatchPair.oldGate = getGate(_netList_old[i]);
						tmpPatchPair.golGate = getGate(_netList_gol[j]);
						patchPairVec.push_back(tmpPatchPair);
						if(timer_findsimilar.checkCurrentTime() > 30)
						{
							time_exceed = true;
							break;
						}
					}
				}
			}
		}
		if( time_exceed ) break;
	}
	
	timer_findsimilar.reset();
	time_exceed = false;
	for(size_t i = 0; i < patchPairVec.size(); ++i)
	{
		if(patchPairVec[i].oldGate -> _mergeFlag == 0)
		{
			if(patchPairVec[i].oldGate -> _type != NTK_GATE_PI && patchPairVec[i].golGate -> _type != NTK_GATE_PI && patchPairVec[i].oldGate -> _type != NTK_GATE_CONST && patchPairVec[i].golGate -> _type != NTK_GATE_CONST && !(patchPairVec[i].golGate -> _mg_list))
			{
				cout << "[][][][[][][][][][][][][][][][][][][][][][]][[]" << patchPairVec[i].oldGate -> getName() << " " << patchPairVec[i].golGate -> getName() << " mergeflag = " << patchPairVec[i].oldGate -> _mergeFlag << endl;



				for(size_t k = 0; k < _netList_old.size(); ++k)
				{
					for(size_t m = 0; m < getGate(_netList_old[k]) -> _recPair_fanin.size(); ++m)
					{
						getGate(_netList_old[k]) -> _recPair_fanin[m] = 0;
					}
					getGate(_netList_old[k]) -> _recPair_fanin.clear();
					for(size_t m = 0; m < getGate(_netList_old[k]) -> _recPair_fanout.size(); ++m)
					{
						getGate(_netList_old[k]) -> _recPair_fanout[m] = 0;
					}
					getGate(_netList_old[k]) -> _recPair_fanout.clear();
					getGate(_netList_old[k]) -> _recCnt = 0;
				}
				_recPairList.clear();

				_mg_count = 0;
				NtkGate::increaseGlobalFlag();
				for(size_t k = 0; k < patchPairVec[i].oldGate -> _fanout.size(); ++k)
				{
					patchPairVec[i].oldGate -> _fanout[k] -> _flag = NtkGate::_globalFlag;
				}

				sprintf( Command, "eco_setFlag -sf" ); Cmd_CommandExecute( pAbc, Command );
				patchPairVec[i].oldGate -> addRecPair(patchPairVec[i].golGate, rPairs_found, _recPairList);
				markFloatingGates();
				outputPatchFile("p.v");
				applyPatch(fname_old, "p.v", "ped.v");
				sprintf( Command, "miter %s %s", fname_gol, "ped.v" ); Cmd_CommandExecute( pAbc, Command );
				clearCFMergeInfo();
				sprintf( Command, "fraig" ); Cmd_CommandExecute( pAbc, Command );
				tmpPatchPairStr.oldGate = patchPairVec[i].oldGate -> getName();
				tmpPatchPairStr.golGate = patchPairVec[i].golGate -> getName();
				tmpPatchPairStr._mg_count = _mg_count;
				bestPatchPair.push(tmpPatchPairStr);
			}
		}
		if( timer_findsimilar.checkCurrentTime() > 200 ) break;
	}
	size_t forNum = (bestPatchPair.size() < 10) ? bestPatchPair.size() : 10;
	for(size_t i = 0; i < forNum; ++i)
	{
		_patchPairStrVec.push_back(bestPatchPair.top());
		bestPatchPair.pop();
	}
}

void
NtkMgr::addPreRecPair( patchRecord &record )
{
	_p_eq = true;

	for(size_t k = 0; k < record.patchArr.size(); ++k)
	{
		NtkGate *g = findGateByName( record.patchArr[k] );
		cout << g -> getName() << endl;

		if ( g == _p_rev ) _p_eq = false;

		NtkGate *gRec;
		unordered_set<NtkGate*> gRecs;
		vector<string> &fanoutRecString = record.patchRecFanout[k];

		if ( g -> _recPair_fanout.empty() ) 
		{
			// cout << "empty" << endl;
			_recPairList.push_back(g);
		}

		NGateList &fanout = g -> _fanout;

		for( size_t i = 0; i < fanoutRecString.size(); i++ )
		{
			gRec = findGateByName(fanoutRecString[i]);
			if ( !gRec ) continue;
			gRecs.insert(gRec);

			NGateList &fanin = fanout[i] -> _fanin;
			NGateList &recFanin = fanout[i] -> _recPair_fanin;;
			if ( recFanin.empty() ) recFanin.resize(fanin.size());

			for( size_t j = 0; j < fanin.size(); j++ )
			{
				if ( fanin[j] == g && recFanin[j] == 0 ) 
				{
					cout << "rec: " << gRec -> getName() << " for fanout " << fanout[i] -> getName() << endl;
					recFanin[j] = gRec;
					g -> _recCnt ++ ;
					break;
				} 
			}
		}

		for( size_t i = fanoutRecString.size(); i < fanout.size(); i++ )
		{
			cout << "extra fanout: " << fanout[i] -> getName() << endl;
		}

		for( size_t i = 0; i < g->_recPair_fanout.size(); i++ )
		{
			gRecs.insert(g->_recPair_fanout[i]);
		}

		g->_recPair_fanout.clear();
		for( unordered_set<NtkGate*>::iterator it = gRecs.begin(); it != gRecs.end(); it ++ )
		{
			g->_recPair_fanout.push_back(*it);
		}
		
	 
	}
	sort( _recPairList.begin(), _recPairList.end(), compareByLevel );
}

void
NtkMgr::prePatch( char *fname_old, char *fname_gol, int sim_num, float sim_thre, int lv_thre , patchRecord &record )
{
	NtkGatePQ tmpPQ;
	for(size_t i = 0; i < _patchPairStrVec.size(); ++i)
	{
		for(size_t k = 0; k < _netList_old.size(); ++k)
		{
			for(size_t m = 0; m < getGate(_netList_old[k]) -> _recPair_fanin.size(); ++m)
			{
				getGate(_netList_old[k]) -> _recPair_fanin[m] = 0;
			}
			getGate(_netList_old[k]) -> _recPair_fanin.clear();
			for(size_t m = 0; m < getGate(_netList_old[k]) -> _recPair_fanout.size(); ++m)
			{
				getGate(_netList_old[k]) -> _recPair_fanout[m] = 0;
			}
			getGate(_netList_old[k]) -> _recPair_fanout.clear();
			getGate(_netList_old[k]) -> _recCnt = 0;
		}
		_recPairList.clear();
		resetPatchRecord(record);

		NtkGate::increaseGlobalFlag();
		for(size_t k = 0; k <  findGateByName(_patchPairStrVec[i].oldGate) -> _fanout.size(); ++k)
		{
			findGateByName(_patchPairStrVec[i].oldGate) -> _fanout[k] -> _flag = NtkGate::_globalFlag;
		}

		sprintf( Command, "eco_setFlag -sf" ); Cmd_CommandExecute( pAbc, Command );
		findGateByName(_patchPairStrVec[i].oldGate) -> addRecPair( findGateByName(_patchPairStrVec[i].golGate), tmpPQ, _recPairList);
		outputPatchFile("p.v");
		applyPatch(fname_old, "p.v", "ped.v");
		sprintf( Command, "miter %s %s", fname_gol, "ped.v" ); Cmd_CommandExecute( pAbc, Command );
		clearCFMergeInfo();
		sprintf( Command, "fraig" ); Cmd_CommandExecute( pAbc, Command );

		record.pAndPPrime.push_back( findGateByName(_patchPairStrVec[i].oldGate) -> getName());
		record.pAndPPrime.push_back( findGateByName(_patchPairStrVec[i].golGate)-> getName());
		identifyRecPairs( findGateByName(_patchPairStrVec[i].oldGate),  findGateByName(_patchPairStrVec[i].golGate));
		buildAndLoadSelector( findGateByName(_patchPairStrVec[i].oldGate),  findGateByName(_patchPairStrVec[i].golGate));
		selectPatch();
		NtkGate *g;
		for(size_t k = 0; k < _netList_old.size(); ++k)
		{
			g = getGate(_netList_old[k]);
			if ( g -> _recPair_fanout.empty() ) continue;
			record.patchRecFanout.resize( record.patchRecFanout.size() + 1 );
			record.patchArr.push_back( g->getName() );
			vector<string> &tmpStrVec = record.patchRecFanout.back();
			tmpStrVec.resize( g->_fanout.size() );
			for(size_t m = 0; m < g -> _fanout.size(); ++m)
			{
				if ( g->_fanout[m] -> getRecPairFanin(g) )
				{
					tmpStrVec[m] = g -> _fanout[m] -> getRecPairFanin(g) -> getName();
				}
			}
		}
	}
}




// ===  whole process ===

void
NtkMgr::attachFiles( char *fname_old, char *fname_gol, char *fname_patch )
{
	_fname_old = fname_old;
	_fname_gol = fname_gol;
	_fname_patch = fname_patch;
}


bool
NtkMgr::ecoPo( int &bestCost, int sim_num, float sim_thre, int lv_thre )
{
	try
	{
		// if not setup, setup
		if ( !_read )
		{
			setupRevAndGol( _fname_old, _fname_gol );
		}

		// === run eco ===

		_sim_num = sim_num;
		_sim_thre = sim_thre;
		_lv_thre = lv_thre;
		_minCS_init = 10;

		// simulate
		if ( _sim_count < _sim_num )
		{
			cout << "Sim for " << _sim_num << endl;
			aigMiter -> randomSim( _sim_num - _sim_count );
			_sim_count = _sim_num;
		}

		NtkGate* g_old;
		NtkGate* g_gol;
		NtkGatePQ rPairs_found;
		//add  Po's as recPair
		for( size_t i = 0; i < _numPO; i++ )
		{
			g_old = getGate(_POList_old[i]);
			g_gol = getGate(_POList_gol[i]);
			_dummyPOList[i] -> _flag = NtkGate::_globalFlag;
			g_old -> addRecPair( g_gol, rPairs_found, _recPairList );

		}
		
		buildAndLoadSelector();
		selectPatch();
		markFloatingGates();
		outputPatchFile(_fname_patch_tmp);
		// estimatePatchCost();


		// apply and check patch if smaller
		if ( _cost < bestCost )
		{
			applyPatch( _fname_old, _fname_patch_tmp, _fname_patched );

			if ( checkPatched( _fname_patch_tmp, _fname_patched, _fname_gol ) )
			{
				// replace patch file if eq
				// TODO: check time...

				sprintf( Command, "rm -rf %s", _fname_patch  );
				system(Command);
				sprintf( Command, "cp %s %s", _fname_patch_tmp, _fname_patch  );
				system(Command);

				cout << "A patch with cost " << _cost << " found." << endl;
				bestCost = _cost;
			}
			else
			{
				cout << "Better patch but not eq" << endl;
			}
			
		}

		// TODO
		// if crush or take too long, 
		// reset and return false;


		return true;
	}
	catch( EcoError &e )
	{
		reset();
		return false;
	}
}

bool
NtkMgr::eco( int &bestCost, int sim_num, float sim_thre, int lv_thre )
{
	try
	{
		bool time_exceed = false;
		// if not setup, setup
		if ( !_read )
		{
			setupRevAndGol( _fname_old, _fname_gol );
		}

		// === run eco ===

	// NtkGate* test = findGateByName("clk_G");
	// assert(test);
	// test -> reportGate();
		_sim_num = sim_num;
		_sim_thre = sim_thre;
		_lv_thre = lv_thre;
		_minCS_init = 10;

		// simulate
		if ( _sim_count < _sim_num )
		{
			cout << "Sim for " << _sim_num << endl;
			aigMiter -> randomSim( _sim_num - _sim_count );
			_sim_count = _sim_num;
		}

		time_exceed = identifyRecPairs();
		buildAndLoadSelector();
		selectPatch();
		markFloatingGates();
		outputPatchFile(_fname_patch_tmp);
		// estimatePatchCost();


		// apply and check patch if smaller
		if ( _cost < bestCost )
		{
			applyPatch( _fname_old, _fname_patch_tmp, _fname_patched );

			if ( checkPatched( _fname_patch_tmp, _fname_patched, _fname_gol ) )
			{
				// replace patch file if eq
				// TODO: check time...

				sprintf( Command, "rm -rf %s", _fname_patch  );
				system(Command);
				sprintf( Command, "cp %s %s", _fname_patch_tmp, _fname_patch  );
				system(Command);

				cout << "A patch with cost " << _cost << " found." << endl;
				bestCost = _cost;
				time_threshold = 2;
				total_time_threshold = 1200;
			}
			else
			{
				cout << "Better patch with cost " << _cost <<  " but not eq" << endl;
			}
			
		}

		// TODO
		// if crush or take too long, 
		// reset and return false;


		return time_exceed;
	}
	catch( EcoError &e )
	{
		resetRecPair();
	}
	
	
}

// prepatch (patch one gate first)
bool
NtkMgr::firstPrepatchEco( int sim_num, float sim_thre, int lv_thre , size_t i, patchRecord &record)
{
	try
	{
		NtkGatePQ tmpPQ;
		if ( !_read )
		{
			setupRevAndGol( _fname_old, _fname_gol );
		}

		// === run eco ===

		_sim_num = sim_num;
		_sim_thre = sim_thre;
		_lv_thre = lv_thre;
		_minCS_init = 4;

		// simulate
		if ( _sim_count < _sim_num )
		{
			aigMiter -> randomSim( _sim_num - _sim_count );
			_sim_count = _sim_num;
		}

		// findSimilarPairs( _fname_old, _fname_gol );

		resetPatchRecord(record);

		NtkGate::increaseGlobalFlag();
		for(size_t k = 0; k <  findGateByName(_patchPairStrVec[i].oldGate) -> _fanout.size(); ++k)
		{
			findGateByName(_patchPairStrVec[i].oldGate) -> _fanout[k] -> _flag = NtkGate::_globalFlag;
		}

		sprintf( Command, "eco_setFlag -sf" ); Cmd_CommandExecute( pAbc, Command );

		findGateByName(_patchPairStrVec[i].oldGate) -> addRecPair( findGateByName(_patchPairStrVec[i].golGate), tmpPQ, _recPairList);
		markFloatingGates();
		outputPatchFile(_fname_p, findGateByName(_patchPairStrVec[i].oldGate),  findGateByName(_patchPairStrVec[i].golGate ) );


		// apply patch

		applyPatch( _fname_old, _fname_p, _fname_ped );

		abc_read = false;
		sprintf( Command, "read_verilog %s", _fname_ped );
		Cmd_CommandExecute( pAbc, Command );
		if(! abc_read ) { throw ERR_RESET_MGR; }


		/*
		sprintf( Command, "miter %s %s", _fname_gol, _fname_ped ); Cmd_CommandExecute( pAbc, Command );
		clearCFMergeInfo();
		sprintf( Command, "fraig" ); Cmd_CommandExecute( pAbc, Command );
		*/

		resetPatchRecord(record);

		identifyRecPairs( findGateByName(_patchPairStrVec[i].oldGate),  findGateByName(_patchPairStrVec[i].golGate) );
		buildAndLoadSelector( findGateByName(_patchPairStrVec[i].oldGate),  findGateByName(_patchPairStrVec[i].golGate) );
		selectPatch();
		NtkGate *g;
		for(size_t k = 0; k < _netList_old.size(); ++k)
		{
			g = getGate(_netList_old[k]);
			if ( g -> _recPair_fanout.empty() ) continue;
			record.patchRecFanout.resize( record.patchRecFanout.size() + 1 );
			record.patchArr.push_back( g->getName() );
			vector<string> &tmpStrVec = record.patchRecFanout.back();
			tmpStrVec.resize( g->_fanout.size() );
			for(size_t m = 0; m < g -> _fanout.size(); ++m)
			{
				if ( g->_fanout[m] -> getRecPairFanin(g) )
				{
					tmpStrVec[m] = g -> _fanout[m] -> getRecPairFanin(g) -> getName();
				}
			}
		}
			

		// TODO
		// if crush or take too long, 
		// reset and return false;


		return true;
	}
	
	catch( EcoError &e )
	{
		switch( e )
		{
			case ERR_RESET_REC:
				resetRecPair();
				return false;
			case ERR_RESET_MGR:
				reset();
				restorePrePatchPair();
				return false;
		}
	}
}


// main eco for prepatch
bool
NtkMgr::SecondPrepatcheco( int &bestCost, int sim_num, float sim_thre, int lv_thre , size_t i, patchRecord &record)
{
	try
	{
		bool time_exceed = false;
		NtkGatePQ tmpPQ;
		// if not setup, setup
		if ( !_read )
		{
			setupRevAndGol( _fname_ped, _fname_gol );
			// record p
			_p_rev = findGateByName(_patchPairStrVec[i].oldGate);
			_p_gol = findGateByName(_patchPairStrVec[i].golGate);
			_p_in = findGateByName(_patchPairStrVec[i].oldGate.substr(0, _patchPairStrVec[i].oldGate.size()-2) + string("_in_O"));
			assert(_p_in);
			_p_in -> _name = _p_rev->_name;
		}

		// === run eco ===

		_sim_num = sim_num;
		_sim_thre = sim_thre;
		_lv_thre = lv_thre;
		_minCS_init = 4;

		// simulate
		if ( _sim_count < _sim_num )
		{
			aigMiter -> randomSim( _sim_num - _sim_count );
			_sim_count = _sim_num;
		}


		time_exceed = identifyRecPairs();

		// patch
		buildAndLoadSelector();
		selectPatch();
		// estimatePatchCost();

		// output patch 
		addPreRecPair( record );
		handlePrePatchPair( _p_rev, _p_gol );
		markFloatingGates( _p_rev, _p_gol );
		outputPatchFile(_fname_patch_tmp);


		// apply and check patch if smaller
		if ( _cost < bestCost )
		{
			applyPatch( _fname_old, _fname_patch_tmp, _fname_patched );

			if ( checkPatched( _fname_patch_tmp, _fname_patched, _fname_gol ) )
			{
				// replace patch file if eq
				// TODO: check time...

				sprintf( Command, "rm -rf %s", _fname_patch  );
				assert( !system(Command) );
				sprintf( Command, "cp %s %s", _fname_patch_tmp, _fname_patch  );
				assert( !system(Command) );

				cout << "A patch with cost " << _cost << " found." << endl;
				bestCost = _cost;
				time_threshold = 30;
			}
			else
			{
				cout << "hooBetter patch but not eq" << endl;
			}
			
		}

		// TODO
		// if crush or take too long, 
		// reset and return false;


		return time_exceed;
	}
	catch( EcoError &e )
	{
		switch( e )
		{
			case ERR_RESET_REC:
				resetRecPair();
				return false;
			case ERR_RESET_MGR:
				reset();
				return false;
		}
	}
}



void
NtkMgr::resetRecPair()
{
	for(size_t k = 0; k < _gateList.size(); ++k)
	{
		_gateList[k] -> _recPair_fanin.clear();
		_gateList[k] -> _recPair_fanout.clear();
		_gateList[k] -> _recCnt = 0;

	}
	_recPairList.clear();
}

void
NtkMgr::tryDifferentParas( int &bestCost , patchRecord &record)
{
	bool time_exceed = false;
	bool total_time_exceed = false;
	bool fisrtPrepatchSucceed = false;


	ecoPo( bestCost, 10, 1.0, 20 );
	resetRecPair();

	for(size_t i = 0; i < sizeof(_sim_num_arr)/sizeof(_sim_num_arr[0]); ++i)
	{
		for(size_t j = 0; j < sizeof(_sim_thres_arr)/sizeof(_sim_thres_arr[0]); ++j)
		{
			for(size_t k = 0; k < sizeof(_lv_arr)/sizeof(_lv_arr[0]); ++k)
			{
				time_exceed = false;
				time_exceed = eco( bestCost, _sim_num_arr[i], _sim_thres_arr[j],  _lv_arr[k] );
				resetRecPair();
				if( time_exceed )
				{
					cout << "time out" << endl;
					break;
				}
				if( timer_total.checkCurrentTime() > total_time_threshold )
				{
					total_time_exceed = true;
					break;
				}
			}
			if( total_time_exceed ) break;
		}
		if( total_time_exceed ) break;
	}


	cout << "NeqNum(total)1 = " << totalNum << endl;
	setupRevAndGol( _fname_old, _fname_gol );
	aigMiter -> randomSim(10);
	findSimilarPairs( _fname_old, _fname_gol );
	reset();

	cout << "find similar pairs done" << endl;

	time_threshold = 30;

	for(size_t l = 0; l < _patchPairStrVec.size(); ++l)
	{
		cout << "p and p prime " << _patchPairStrVec[l].oldGate << " " << _patchPairStrVec[l].golGate << endl;
		fisrtPrepatchSucceed = firstPrepatchEco( 1, 1.0,  3,  l, record);
		reset();
		cout << "pre patch done" << endl;
		if( !fisrtPrepatchSucceed ) continue;

		for(size_t i = 0; i < sizeof(_pre_sim_num_arr)/sizeof(_pre_sim_num_arr[0]); ++i)
		{
			for(size_t j = 0; j < sizeof(_pre_sim_thres_arr)/sizeof(_pre_sim_thres_arr[0]); ++j)
			{
				for(size_t k = 0; k < sizeof(_pre_lv_arr)/sizeof(_pre_lv_arr[0]); ++k)
				{
					time_exceed = false;
					time_exceed = SecondPrepatcheco( bestCost, _sim_num_arr[i], _sim_thres_arr[j],  _lv_arr[k],  l, record);
					resetRecPair();
					if( time_exceed )
					{
						cout << "time out" << endl;
						break;
					}
				}
			}
		}

		cout << l << " th pre patch done" << endl;
		reset();
	}
	cout << "NeqNum(total)2 = " << totalNum << endl;
	return;

}

bool
NtkMgr::checkPatched( char *fname_patch, char *fname_patched,  char *fname_gol )
{
	// TODO: check output

	abc_read = false;
	sprintf( Command, "read_verilog %s", fname_patch);
	Cmd_CommandExecute( pAbc, Command );
	// assert(abc_read);
	if ( !abc_read )  return false;

	abc_eq = false;
	sprintf( Command, "cec %s %s", fname_patched, fname_gol);
	Cmd_CommandExecute( pAbc, Command );
	// assert(abc_eq);

	return abc_eq;
}

void
NtkMgr::checkXorStruct()
{
	for( size_t i = 0; i < _netList_gol.size(); i++ )
	{
		getGate(_netList_gol[i]) -> checkXorStruct();
	}
}