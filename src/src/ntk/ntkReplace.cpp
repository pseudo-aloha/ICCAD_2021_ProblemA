#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <queue>
#include <algorithm>
#include <iterator>


#include "ntkMgr.h"
#include "ntkGate.h"
#include "cirMgr.h"
#include "sat.h"

using namespace std;


extern NtkMgr* 	ntkMgr;
extern CirMgr*	aigSel;
extern SatSolver* solver;

extern bool debug_s;
extern bool debug_p;

extern "C"
{
	typedef struct Abc_Frame_t_ Abc_Frame_t;
	int    Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand );
}
extern Abc_Frame_t * pAbc;
static char Command[1024];

void
NtkMgr::buildAndLoadSelector( NtkGate *g_rev, NtkGate *g_gol )
{
	sprintf( Command, "eco_setFlag" ); Cmd_CommandExecute( pAbc, Command );
	// build selector
	buildSelector(".selector.v", false,  g_rev, g_gol );
	//ntkMgr -> buildSelector(".selector.manual.v", true);

	// convert selector to aag
	sprintf( Command, "read_verilog .selector.v" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "fraig" ); Cmd_CommandExecute( pAbc, Command );
	// TODO: fraig will result in a wrong patch ??
	// sprintf( Command, "print_stats" ); Cmd_CommandExecute( pAbc, Command );
	// sprintf( Command, "strash" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "print_stats" ); Cmd_CommandExecute( pAbc, Command );
	//sprintf( Command, "sat -v" ); Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "write_aiger -s .selector.aig"); Cmd_CommandExecute( pAbc, Command );
	if( system("./aigtoaig .selector.aig .selector.aag") ) { throw ERR_RESET_REC; };

	// read selector (aag)
	aigSel = new CirMgr();
	aigSel -> readCircuit(".selector.aag", CIR_SEL);
	// aigSel -> printSummary();
}


void 
NtkMgr::buildSelector( string fname, bool manual, NtkGate *g_rev, NtkGate *g_gol )
{

	ofstream ouf(fname);

	_netCnt = _gateList.size();

	unsigned muxBegin = _netCnt;

	// header (module)
	ouf << "module selector(miter";


	// create net for mux first
	// add sel signal as input
	NtkGate *g;
	for(size_t i=0; i<_recPairList.size(); i++)
	{
		g = _recPairList[i];
		if ( g->_recPair_fanout.empty() && 0==g->_numBwdPatch ) continue;
		g -> setMuxId( _netCnt );
		for( size_t j=0, n=g->_recPair_fanout.size()+g->_numBwdPatch; j<n; j++ )
		{
			if ( !manual ) ouf << ",n" << _netCnt+2*j+1;
		}
		_netCnt += 2 * (g->_recPair_fanout.size()+g->_numBwdPatch);
	}
	unsigned numMux = (_netCnt-muxBegin)/2;

	// add PIs to header
	for( size_t i = 0; i < _numPI; i++ )
		ouf << ",n" << getGate(_PIList_old[i])->getId(); 
	ouf << ");\n";

	// outputs
	ouf << "output miter;\n";

	if ( manual )
	{
		// original PIs
		for( size_t i = 0; i < _numPI; i++ )
			ouf << "input n" << getGate(_PIList_old[i])->getId() << ";\n";
	}

	// MUXs sel inputs
	for( size_t i = 0; i < numMux; i++ )
	{
		if ( !manual ) ouf  << "input n" << muxBegin + 2*i + 1 << ";\n";
		else ouf  << "wire n" << muxBegin + 2*i + 1 << ";\n";
	}

	if ( manual )
	{
		for( size_t i = 0; i < numMux; i++ )
			ouf  << "assign n" << muxBegin + 2*i + 1 << " = 1'b1;\n";
	}

	if ( !manual )
	{
		// original PIs
		for( size_t i = 0; i < _numPI; i++ )
			ouf << "input n" << getGate(_PIList_old[i])->getId() << ";\n";
	}
	
	// original gates
	unsigned gateCnt = 0;
	for( size_t i=0; i < _netList_old.size(); i++ )
		getGate(_netList_old[i])->selAddGate( ouf, gateCnt );
	// TODO: merge the bottom part of golden
	for( size_t i=0; i < _netList_gol.size(); i++ )
		getGate(_netList_gol[i])->selAddGate( ouf, gateCnt );


	// muxes (patch)
	queue<NtkGate*> patchGates;
	for( unsigned i = 0; i < _recPairList.size(); i++ )
	{
		NtkGate* g = _recPairList[i];

		for( size_t j = 0, n = g->_numBwdPatch; j < n; j++ )
		{
			unsigned mux = g->_muxId+2*j;
			if ( debug_s ) cout << "bwd patch n" << mux << " ( " << _bwdPatchMap[g][j] -> getName() << " -> " << g->getName() << " )" << endl;
			unsigned sel = mux+1;
			unsigned sel_inv = newNet();
			unsigned t1 = newNet(), t2 = newNet();
			ouf << "not g" << gateCnt++ << "(n" << sel_inv <<
				",n" << sel << ");\n";
			// 0: gate in rev (might be mux)
			ouf << "nand g" << gateCnt++ << "(n" << t1 <<
				"," << g->selGetNetForBwdPatch(_bwdPatchMap[g][j]) <<
				",n" <<  sel_inv << ");\n";
			// 1: gate in gol
			ouf << "nand g" << gateCnt++ << "(n" << t2 <<
				"," << _bwdPatchMap[g][j]->selGetNetCopy() <<
				",n" <<  sel << ");\n";
			ouf << "nand g" << gateCnt++ << "(n" << mux <<
				",n" << t1 <<
				",n" << t2 << ");\n";

		}
		for( size_t j = 0, n = g->_recPair_fanout.size(); j < n; j++ )
		{
			unsigned mux = g->_muxId+2*(j+g->_numBwdPatch);
			//if ( debug_s ) cout << "patch n" << mux << " ( " << g -> getName() << " -> " << g->_recPair_fanout[j]->getName() << " )" << endl;
			unsigned sel = mux+1;
			unsigned sel_inv = newNet();
			unsigned t1 = newNet(), t2 = newNet();
			ouf << "not g" << gateCnt++ << "(n" << sel_inv <<
				",n" << sel << ");\n";
			// 0
			ouf << "nand g" << gateCnt++ << "(n" << t1 <<
				"," << g->selGetNet() <<
				",n" <<  sel_inv << ");\n";
			// 1
			ouf << "nand g" << gateCnt++ << "(n" << t2 <<
				"," << g->_recPair_fanout[j]->selGetNetCopy() <<
				",n" <<  sel << ");\n";
			ouf << "nand g" << gateCnt++ << "(n" << mux <<
				",n" << t1 <<
				",n" << t2 << ");\n";

			patchGates.push(g->_recPair_fanout[j]);
		}
	}




	// patch gates
	NtkGate::increaseGlobalFlag(1);
	while( !patchGates.empty() )
	{
		patchGates.front() -> selAddGateCopy( ouf, gateCnt, true );
		patchGates.pop();
	}

	// miters
	unsigned miter;
	if ( g_rev == 0 || g_gol == 0 )
	{
		for ( unsigned i = 0; i < _numPO; i++ )
		{
			unsigned old =  getGate(_POList_old[i])->_id;

			if ( !_dummyPOList[i] -> _recPair_fanin.empty() )
			{
				NtkGate *rec = _dummyPOList[i] -> _recPair_fanin[0];
				if ( rec )
				{
					old = getGate(_POList_old[i])-> _muxId+getGate(_POList_old[i])->_numBwdPatch;
					NGateList &recFanout = getGate(_POList_old[i]) -> _recPair_fanout;
					for( size_t j = 0; j < recFanout.size(); j++  )
					{
						if ( recFanout[j] == rec ) 
						{
							old += j;
							break;
						}
					}
				}
			}

			unsigned gol = getGate(_POList_gol[i])->_id;
			unsigned netXor = newNet();

			ouf << "xor g" << gateCnt++ << "(n" << netXor <<
				",n" << old <<
				",n" << gol << ");\n";

			if ( i == 0 ) miter = netXor;
			else 
			{
				unsigned netOr = newNet();
				ouf << "or g" << gateCnt++ << "(n" << netOr <<
					",n" << netXor <<
					",n" << miter << ");\n";
				miter = netOr;
			}
		}
	}
	else
	{
		unsigned old = g_rev-> _recPair_fanout.empty() ? g_rev -> getId() :  g_rev -> _muxId + g_rev ->_numBwdPatch;
		unsigned gol = g_gol -> getId();
		unsigned netXor = newNet();

		ouf << "xor g" << gateCnt++ << "(n" << netXor <<
			",n" << old <<
			",n" << gol << ");\n";

		miter = netXor;
	}
	

	ouf << "assign miter = n" << miter << ";\n";
	ouf << "endmodule\n";

	ouf.close();
	cout << "Selector successfully built." << endl;

	_numMux = numMux;

}

void
NtkGate::selAddGate( ofstream &ouf, unsigned &gateCnt )
{
	if ( _type == NTK_GATE_PI || _type == NTK_GATE_CONST ) return;

	ouf << getTypeStr() << " g" << gateCnt++ << "(n" << _id;


	if ( !_old || _recPair_fanin.empty() )
	{
		for( size_t i = 0; i < _fanin.size() ; i++ )
			ouf << "," << _fanin[i] -> selGetNet();
	}
	else
	{
		for( size_t i = 0; i < _fanin.size() ; i++ )
			ouf << "," << _fanin[i] -> selGetNet( _recPair_fanin[i] );
	}
	ouf << ");\n";
}
void
NtkGate::selAddGateCopy( ofstream &ouf, unsigned &gateCnt, bool out )
{

	// TODO
	if ( _mg_list ) return;

	if ( _flag == _globalFlag ) return;
	_flag = _globalFlag;

	if ( _type == NTK_GATE_PI || _type == NTK_GATE_CONST ) return;

	ouf << getTypeStr() << " g" << gateCnt++ << "(c" << _id;


	if ( _recPair_fanin.empty() )
	{
		for( size_t i = 0; i < _fanin.size() ; i++ )
			ouf << "," << _fanin[i] -> selGetNetCopy();
	}
	else
	{
		for( size_t i = 0; i < _fanin.size() ; i++ )
			ouf << "," << _fanin[i] -> selGetNetCopy( _recPair_fanin[i] );

	}



	ouf << ");\n";

	for( size_t i = 0; i < _fanin.size(); i++ )
		_fanin[i] -> selAddGateCopy( ouf, gateCnt, false );

}

string
NtkGate::selGetNet( NtkGate* rec )
{

	if ( !rec )
	{
		NtkGate *g;
		if ( _mg_list && !_old) g = _mg_list;
		else g = this;

		if ( g -> _type == NTK_GATE_CONST ) 
		{
			return g -> getNameShort();
		}
		else return string("n") + to_string((long long)g->_id);
	}
	else
	{
		for ( size_t i = 0; i < _recPair_fanout.size(); i++ )
		{
			if ( _recPair_fanout[i] == rec )
			{
				return string("n") + to_string( (long long)_muxId + 2*(_numBwdPatch+i) );
			}
		}
		cout << "selGetNet() error!!" << endl;
		return string("nError");

	}

}
string
NtkGate::selGetNetCopy( NtkGate* rec )
{
	unsigned id;

	if ( _mg_list )
	{
		if( _mg_list -> _type == NTK_GATE_CONST ) return _mg_list -> getNameShort();
		else return string("n") + to_string( (long long) _mg_list -> _id);
	}

	if ( rec && !_recPair_fanout.empty() )
	{
		id = ntkMgr -> getBwdPatchMux( rec, this );
		return string("n") + to_string( (long long) id);
	}
	else
	{
		if ( _type == NTK_GATE_CONST ) return getNameShort();
		return string("c") + to_string( (long long) _id);
	}

}
unsigned
NtkMgr::getBwdPatchMux( NtkGate* gr, NtkGate* gg )
{
	vector<NtkGate*> &lst = _bwdPatchMap[gr];
	for( size_t i = 0; i < lst.size(); i++ )
	{
		if ( lst[i] == gg )
		{
			return gr -> _muxId + 2*i;
		}
	}
	// if not found
	return gr -> _id;
}

string
NtkGate::selGetNetForBwdPatch( NtkGate* gol )
{
	for( size_t i = 0; i < _recPair_fanout.size(); i++ )
	{
		if ( _recPair_fanout[i] == gol ) 
		{
			return string("n") + to_string( (long long) (_muxId + 2*(_numBwdPatch + i) ));
		}
	}
	return string("n") + to_string((long long)_id);

}

void
NtkMgr::selectPatch()
{
	vector<bool> res = aigSel -> selectPatch( _numMux );

	if ( debug_s )
	{
		cout << endl;
		cout << "patch selected: " << endl;
	}

	unsigned index = 0;
	NGateList newRecPairList;

	for ( size_t i = 0; i < _recPairList.size(); i++ )
	{
		if ( _recPairList[i] -> selectRecPair( res, index ) )
		newRecPairList.push_back( _recPairList[i] );
	}
	if ( debug_s ) cout << endl;

	_recPairList =  newRecPairList;


	/*
	for(size_t k = 0; k < _netList_old.size(); ++k)
	{
		cout << "checking " << getGate(_netList_old[k]) -> getName() << endl;;
		NGateList &recFanin = getGate(_netList_old[k]) -> _recPair_fanin;
		NGateList &fanin = getGate(_netList_old[k]) -> _fanin;
		if ( ! recFanin.empty() )
		{
			for( size_t m = 0; m < recFanin.size(); m++ )
			{
				if ( recFanin[m] ) cout << "rec pair fanin: " << recFanin[m] -> getName() << " for " << fanin[m] -> getName() << endl;
			}
		}
	}
	*/

}
bool
NtkGate::selectRecPair( vector<bool> &res, unsigned &index )
{

	NtkGate* rec;
	NGateList newRecPair_fanout;
	unsigned newNumBwdPatch = 0;

	for( size_t i = 0; i < _numBwdPatch; i++ )
	{
		if ( debug_s )
		{
			if ( res[index] ) cout << "   ";
			else cout << " < ";
			cout 
				<< setw(4) << getLevel()
				<< setw(W_NAME) << getNameShort() 
				<< setw(6) << getId()
				<< " <- " 
				<< setw(4) << ntkMgr -> _bwdPatchMap[this][i] -> getLevel()
				<< setw(W_NAME) << ntkMgr -> _bwdPatchMap[this][i] -> getNameShort() 
				<< setw(6) << ntkMgr -> _bwdPatchMap[this][i] -> getId()
				<< endl;

		}
		
		if ( !res[index++] ) 
		{
			newNumBwdPatch++;
			continue;
		}

		// if not taken
		// TODO
		NtkGate *gol = ntkMgr -> _bwdPatchMap[this][i];

		for( size_t j = 0; j < gol->_fanout.size(); j++ )
		{
			NGateList &recList=gol->_fanout[j]->_recPair_fanin;
			for( size_t k = 0; k < recList.size(); k++ )
			{
				if ( recList[k] == this ) 
				{
					recList[k] = 0;
					break;
				}
				// TODO: also clear the whole vector if it's empty
			}
		}

		for( size_t j = 0; j < gol->_recPair_fanout.size(); j++ )
			if ( gol->_recPair_fanout[j] == this )
			{
				gol -> _recPair_fanout.erase( gol -> _recPair_fanout.begin() + j );
				break;
			}
	}

	for( size_t i = 0; i < _recPair_fanout.size(); i++ )
	{
		// debug
		if ( debug_s )
		{
			if ( res[index] ) cout << " o ";
			else cout << "   ";
			cout 
				<< setw(4) << getLevel()
				<< setw(W_NAME) << getNameShort() 
				<< setw(6) << getId()
				<< " -> " 
				<< setw(4) << _recPair_fanout[i] -> getLevel()
				<< setw(W_NAME) << _recPair_fanout[i] -> getNameShort() 
				<< setw(6) << _recPair_fanout[i] -> getId();

		
			if ( _recPair_fanout[i] -> _mg_list )
			{
				cout  << " = " << setw(16) << _recPair_fanout[i] -> _mg_list -> getNameShort();
			}
			else
			{

				NtkGate::increaseGlobalFlag();
				unsigned numGate = 0;
				_recPair_fanout[i] -> estimatePatchCost( numGate, true );
				cout << setw(15) << "#" <<  setw(4) << numGate;
			}
			cout << endl;
		}
		


		if ( res[index++] ) 
		{
			newRecPair_fanout.push_back( _recPair_fanout[i] );
			continue;
		}

		// if not taken
		rec = _recPair_fanout[i];

		for( size_t j = 0; j < _fanout.size(); j++ )
		{
			NGateList &recList=_fanout[j]->_recPair_fanin;
			NGateList &fanin=_fanout[j]->_fanin;
			for( size_t k = 0; k < recList.size(); k++ )
			{
				if ( recList[k] == rec && fanin[k] == this ) 
				{
					recList[k] = 0;
					_recCnt--;
					break;
				}
				// TODO: also clear the whole vector if it's empty
			}
		}
	}

	_recPair_fanout = newRecPair_fanout;
	return !_recPair_fanout.empty();
}

vector<bool>
CirMgr::selectPatch( unsigned numPatch )
{
	solver -> reset();
	CirGate::increaseGlobalFlag(4);
	CirGate* miter = getPO(0);

	assert( miter );

	miter->addClauseRec();
	// solver -> printStats();

	solver -> assertProperty( miter->getVar(), true );
	vector<bool> res;
	res.clear();
	res.resize( numPatch, true );

	// pre proof with all patches (just for testing)
	for( size_t j = 0; j < res.size(); j++ )
		solver -> assumeProperty( _gateList[j+1]->getVar(), res[j] );
	bool sat = solver -> assumpSolve();
	if (sat) cout << "Illegal RP Set!!!" << endl;
	else cout << "valid rp set!!!" << endl;
	
	solver -> assumeRelease();

	// start removing
	for( size_t i = 0; i < res.size(); i++ )
	{
		res[i] = false;
		for( size_t j = i; j < res.size(); j++ )
			solver -> assumeProperty( _gateList[j+1]->getVar(), res[j] );

		bool sat = solver -> assumpSolve();
		if (sat) res[i] = true;			// patch is needed
		
		solver -> assumeRelease();
		solver -> assertProperty( _gateList[i+1]->getVar(), res[i]);

	}
	return res;
}

void
NtkMgr::estimatePatchCost()
{

	vector<unsigned>	patchSize;
	unsigned count;

	for( size_t i = 0; i < _recPairList.size(); i++ )
	{
		NGateList &recList = _recPairList[i]->_recPair_fanout;
		for( size_t j = 0; j < recList.size(); j++ )
		{
			count = 0;
			NtkGate::increaseGlobalFlag();
			recList[j] -> estimatePatchCost( count, true );
			patchSize.push_back(count);
		}
	}

	count = 0;
	unsigned count_prev = 0, index = 0;

	NtkGate::increaseGlobalFlag();
	for( size_t i = 0; i < _recPairList.size(); i++ )
	{
		NGateList &recList = _recPairList[i]->_recPair_fanout;
		for( size_t j = 0; j < recList.size(); j++ )
		{
			recList[j] -> estimatePatchCost( count, true );
			if ( debug_s ) 
			{
				cout << "#" << setw(3) << count-count_prev << " / " << setw(3) << patchSize[index] << " gates from" << setw(W_NAME) << _recPairList[i] -> getName()  << " -> " << setw(W_NAME) << recList[j] -> getNameShort() << endl;
				index++;
			}
			count_prev = count;
		}
	}
	cout << "# of gates in patch: " << count << endl;

	count = 0;
	NtkGate::increaseGlobalFlag();
	for( size_t i = 0; i < _POList_gol.size(); i++ )
	{
		getGate( _POList_gol[i] ) -> estimatePatchCost( count );
	}
	cout << "# of gates if patch from output: " << count << endl;

}
void
NtkGate::estimatePatchCost( unsigned &num, bool bwdPatch )
{
	if ( _flag == _globalFlag || _mg_list || _type == NTK_GATE_CONST ) return;
	_flag = _globalFlag;

	if ( _recPair_fanin.empty() || !bwdPatch )
	{
		for( size_t i = 0; i < _fanin.size(); i++ )
			_fanin[i] -> estimatePatchCost( num, bwdPatch );
	}
	else
	{
		for( size_t i = 0; i < _fanin.size(); i++ )
		{
			if ( !_recPair_fanin[i] ) _fanin[i] -> estimatePatchCost( num, bwdPatch );
		}
	}

	num++;
}


void
NtkGate::GetInputPatch( NGateList &input, NGateList &wire,  bool output )
{
	if (  _flag == _globalFlag ) return;
	_flag = _globalFlag;

	if (_mg_list && _mg_list -> _flag == _globalFlag) return;
	
	if ( (_type == NTK_GATE_PI || _mg_list) ) 
	{
		if(_mg_list -> _flag != _globalFlag && _type != NTK_GATE_CONST && _mg_list -> _type != NTK_GATE_CONST) input.push_back( _mg_list );
		_mg_list -> _flag = _globalFlag;
		// cout << "=== " << this -> _mg_list -> getName() << endl;
		return;
	}

	if ( _recPair_fanin.empty() )
	{
		for( size_t i = 0; i < _fanin.size(); i++ )
		{
			_fanin[i] -> GetInputPatch(input, wire, false);
		}

	}
	else
	{
		
		for(size_t i = 0; i < _fanin.size(); ++i)
		{
			if( _recPair_fanin[i] )
			{
				if(_recPair_fanin[i] -> _type != NTK_GATE_CONST && _recPair_fanin[i]->_flag != _globalFlag && !_recPair_fanin[i] -> IsAllPatched() ) input.push_back(_recPair_fanin[i]);
				_recPair_fanin[i] -> _flag = _globalFlag;
			}
			else
			{
				_fanin[i] -> GetInputPatch(input, wire, false);
			}
		}
	}
	
	// cout << "=== " << this -> getName() << endl;
	wire.push_back(this);
}

bool
NtkGate::IsAllPatched()
{
	if ( _type == NTK_GATE_CONST ) return false;
	else if(  _recPair_fanout.size() == 1 )
	{
		if ( _recCnt >= _fanout.size() ) return true;
		// else return false;
		else
		{
			for ( size_t i = 0; i < _fanout.size(); i++ )
			{
				if ( _fanout[i] -> getRecPairFanin( this ) == 0 )
				{
					// if not floating, return false
					if ( _fanout[i] -> _flagB == _globalFlagB ) return false;
				}
			}
			if ( debug_p) cout << "Patch Simplification: Ignore floating gates." << getName() << endl;
			return true;
		}
	}
	
	else return false;
}

//return the "index + 1" of the most occured gate, return 0 if the gate is not patch for more than half
unsigned
NtkGate::IsPatchMoreThanHalf()
{
	size_t count = 0;
	if((this -> IsAllPatched()) ||  _recCnt == 0 || _type == NTK_GATE_PI || _type == NTK_GATE_CONST) return 0;

	for(size_t i = 0; i < _recPair_fanout.size(); ++i)
	{
		count = 0;
		for(size_t j = 0; j < _fanout.size(); ++j)
		{
			for(size_t k = 0; k < _fanout[j] -> _recPair_fanin.size(); ++k)
			{
				if(_fanout[j] -> _recPair_fanin[k])
				{
					if(_fanout[j] -> _fanin[k] == this && _fanout[j] -> _recPair_fanin[k] == _recPair_fanout[i])
					{
						count++;
						break;
					}
				}
			}
		}
		if(count >= _fanout.size() / 2.0)
		{
			return i + 1;
		} 
	}
	return false;
}
// this is a function to find the gates that should not be patched orignially,
// but its fanin is changed to the siganl that has to be patch more than half of its fanin
bool
NtkGate::IsFaninChanged()
{
	unsigned tmp = 0;
	for(size_t i = 0; i < _fanin.size(); ++i)
	{
		tmp =  _fanin[i] -> IsPatchMoreThanHalf();
		if(tmp && _recPair_fanin.size() != 0 && _recPair_fanin[i] != NULL && _recPair_fanin[i] == _fanin[i] -> _recPair_fanout[tmp - 1])  return false;
		else if(tmp )
		{
			// cout << "alohaloha : " << this -> getName() << endl;
			return true;
		}
	}
	return false;
}

bool
NtkGate::IsPatchOutput()
{
	if( this -> IsAllPatched() ) return true;
	// unsigned tmp = 0;
	// if((IsPatchMoreThanHalf() != 0)) return true;
	// for(size_t i = 0; i < _fanin.size(); ++i)
	// {
	// 	tmp =  _fanin[i] -> IsPatchMoreThanHalf();
	// 	if(tmp && _recPair_fanin.size() != 0 && _recPair_fanin[i] != NULL && _recPair_fanin[i] == _fanin[i] -> _recPair_fanout[tmp - 1])  return false;
	// 	else if(tmp )
	// 	{
	// 		return true;
	// 	}
	// }
	if ( _recPair_fanin.empty() ) return false;
	for(size_t i = 0; i < _fanin.size(); ++i)
	{
		if( _recPair_fanin[i] && !(_fanin[i] -> IsAllPatched()))
		{
			return true;
		}
	}
	return false;
}

void        
NtkMgr::outputPatchFile(string fname, NtkGate *p_rev, NtkGate *p_gol )
{
	ofstream ouf(fname);

	if ( _p_rev && _p_gol )
	{
		cout << "P and P': " << _p_rev -> getName() << "  " << _p_gol -> getName() << endl;
	}

	unsigned gateCnt = 0; //record the number of gate
	unsigned counter = 0;
	unsigned gate_number = 0;


	_cost = 0; //record the cost

	NtkGate::increaseGlobalFlag(2);
	const size_t  ALL = NtkGate::_globalFlag;
	const size_t  PAR = NtkGate::_globalFlag - 1;
	NtkGate *g;


	// mark ALLs
	queue<NtkGate*> allPatchQueue;

	if ( p_rev && p_gol )
	{
		p_rev -> _flag = ALL;
		p_rev -> _allPatchTo = p_rev -> _recPair_fanout[0];
	}
	else
	{
		for( size_t i = 0; i < _dummyPOList.size(); i++ )
		{
			if ( !_dummyPOList[i] -> _recPair_fanin.empty() && _dummyPOList[i] -> _recPair_fanin[0] )
			{
				allPatchQueue.push( _dummyPOList[i] -> _fanin[0] );
				_dummyPOList[i] -> _fanin[0] -> _allPatchTo = _dummyPOList[i] -> _recPair_fanin[0];
			}
		}
		// induced ALLs
		while( !allPatchQueue.empty() )
		{
			g = allPatchQueue.front();
			allPatchQueue.pop();
			//TODO: 2 ALL will error if they don't have the same allpatchto
			//assert( g -> _flag != ALL );

			g -> _flag = ALL;
			// TODO: if p, check p_in

			if ( ! g-> _recPair_fanin.empty() )
			{
				for( size_t i = 0; i < g->_recPair_fanin.size(); i++ )
				{
					if ( g -> _recPair_fanin[i] ) allPatchQueue.push( g-> _fanin[i] );
					g -> _fanin[i] -> _allPatchTo = g -> _recPair_fanin[i];
				}
			}
		}
	}


	_patchInputSet.clear();
	_patchOutputList.clear();
	_patchWireList.clear();


	// start deciding patches
	// also get i/o list and wires
	for( size_t i = 0; i < _netList_patched.size(); i++ )
	{
		_netList_patched[i] -> decidePatchType();
	}

	// if ( _p_rev && _p_in )
	// {
	// 	assert ( _p_rev -> _flag < NtkGate::_globalFlag - 1 || _p_in -> _flag < NtkGate::_globalFlag - 1 );
	// }


	// print debug
	if ( debug_s )
	{
		for( size_t i = 0; i < _recPairList.size(); i++ )
		{
			_recPairList[i] -> reportFanoutRecPair(true);
		}
	}

	// move input to list
	NGateList 	inputList; // list to record the input signals of the patch
	NGateList	inputInvList;
	NGateList 	&outputList  = _patchOutputList;
	NGateList 	&wireList = _patchWireList;

	for( InputAigTable::iterator it = _in2Aig.begin(); it != _in2Aig.end(); it++ )
	{
		if ( ! it -> second.used  ) continue;
		_patchInputSet.insert( it -> second.gate );
		if ( it -> second.needInvert && it -> second.gate -> getType() != NTK_GATE_CONST ) inputInvList.push_back( it -> second.gate );
	}
	for( unordered_set<NtkGate*>::iterator it = _patchInputSet.begin(); it != _patchInputSet.end(); it++ )
	{
		if ( (*it) -> getType() != NTK_GATE_CONST ) inputList.push_back(*it);
	}

	counter = 0;
	ouf << "module top_eco(";
	for (size_t i = 0 ; i < inputList.size() ; ++i) {
		if(i != 0) ouf << ", ";
		counter++;
		if( counter % 20 == 0) ouf << "\n";
		ouf << inputList[i] -> getPatchFaninName2( 0, true );
	}
	// for (size_t i = 0 ; i < inputList_1.size() ; ++i) {
	// 	ouf << ", " << inputList_1[i] -> getPatchFaninName(2);
	// }
	for (size_t i = 0 ; i < outputList.size() ; ++i)
	{
		if (i != 0 || inputList.size() != 0) ouf << ", ";
		counter++;
		if( counter % 20 == 0) ouf << "\n";
		ouf << outputList[i] -> getPatchFaninName2(0);
	}
	ouf << ");\n";

	// input : inputs are stored in inputList, simply write them out : )
	// cout << "input" << endl;
	counter = 0;
	if(inputList.size() > 0) ouf << "input ";
	for (size_t i = 0 ; i < inputList.size() ; ++i) 
	{ 
		if(i != 0) ouf << ", ";
		counter++;
		if( counter % 20 == 0) ouf << "\n";
		ouf << inputList[i] -> getPatchFaninName2(0, true);
	}
	// for (size_t i = 0 ; i < inputList_1.size() ; ++i) {
	// 	ouf << ", " << inputList_1[i] -> getPatchFaninName(2);
	// }
	if(inputList.size() > 0) ouf << ";\n";

	// output : output include the ones in the old circuit and those in the gold circuit
	// print them out seperately(warning : the lists can be empty)
	// cout << "output" << endl;
	counter = 0;
	ouf << "output " ;
	for (size_t i = 0 ; i < outputList.size() ; ++i) 
	{ 
		if(i != 0) { ouf << ", "; }
		counter++;
		if( counter % 20 == 0) ouf << "\n";
		ouf << outputList[i] ->getPatchFaninName2(0); 
	}
	ouf << ";\n";

	// wire : inputs, outputs and the gates that we pass by in the gold circuit, print them
	// cout << "wire" <<endl;
	counter = 0;
	ouf << "wire ";
	for (size_t i = 0 ; i < inputList.size() ; ++i) 
	{ 
		if(i != 0) ouf << ", ";
		counter++;
		if( counter % 20 == 0) ouf << "\n";
		ouf << inputList[i] -> getPatchFaninName2(0, true);
	}
	// for (size_t i = 0 ; i < inputList_1.size() ; ++i) {
	// 	ouf << ", " << inputList_1[i] -> getPatchFaninName(2);
	// }
	for (size_t i = 0 ; i < outputList.size() ; ++i) 
	{
		if(i != 0 || inputList.size() != 0) ouf << ", ";
		counter++;
		if( counter % 20 == 0) ouf << "\n";
		ouf << outputList[i] -> getPatchFaninName2(0); 
	}
	for (size_t i = 0 ; i < wireList.size() ; ++i) 
	{ 
		ouf << ", ";
		counter++;
		if( counter % 20 == 0) ouf << "\n";
		ouf << wireList[i] -> getPatchFaninName2(0); 
	}
	for( size_t i = 0 ; i < inputInvList.size(); ++i )
	{
		ouf << ", ";
		counter++;
		if( counter % 20 == 0) ouf << "\n";
		ouf << inputInvList[i] -> getMappedFaninName( true );; 

	}
	ouf << ";\n";
	_cost += wireList.size() + outputList.size() + inputList.size() + inputInvList.size();

	// gate : we have to declare two types of gates as follows
	// 1 : the ones that are declared as outputs
	// 2 : the ones that is in wireList... 

	_useConst = false;

	for(size_t i = 0; i < outputList.size(); ++i)
	{

		if(outputList[i] -> _flag == ALL )
		{
			_cost--;

			if ( outputList[i] -> _allPatchTo -> getMappedFaninName() == "1'b1" )
			{
				ouf << "not eco_ga" << gateCnt ++ << "(" << outputList[i] -> getPatchFaninName2( 0 );
				ouf << ", 1'b0);\n";
				if ( debug_p ) cout << "use Const" << endl;
				_useConst = true;
			}
			else
			{
				ouf << "buf eco_ga" << gateCnt++ << "(" << outputList[i] -> getPatchFaninName2( 0 );
				ouf << ", " << outputList[i] -> _allPatchTo -> getMappedFaninName();
				ouf << ");\n";

				if ( outputList[i] -> _allPatchTo -> getMappedFaninName() == "1'b0" ) _useConst = true;

				if ( debug_p ) cout << "output: " <<  outputList[i] -> getName()  <<  " all patched to " << outputList[i] -> _allPatchTo -> getName() << endl;
			}

			// HERE


		}
		else // it's Partial
		{
			outputList[i] -> outputGateInPatch( ouf, gateCnt );
		}

	}


	for (size_t i = 0; i < wireList.size(); ++i )
	{
		// create gate
		// TODO: hand const simplification and const change
		/*
		_cost -= 2;
		ouf <<  wireList[i]->getTypeStr() << " eco_gc" << gateCnt++ << "(" << wireList[i] -> getPatchFaninName2( 0 );
		if ( wireList[i] -> _recPair_fanin.empty() )
		{

			for (size_t j = 0 ; j < wireList[i]->_fanin.size() ; ++j) 
			{
				_cost ++;
				ouf << ", " << wireList[i] -> _fanin[j] -> getPatchFaninName2( wireList[i] ); 
			}

		}
		else
		{
			for (size_t j = 0 ; j < wireList[i]->_fanin.size() ; ++j) 
			{
				_cost ++;
				// TODO: if partial patch and recPair_fanout includes fanin[j], just use fanin[j]
				if( wireList[i] -> _recPair_fanin[j] && !wireList[i] -> _recPair_fanin[j] -> IsAllPatched() ) ouf << ", " << wireList[i] -> _recPair_fanin[j] -> getNameShort(); 
				else ouf << ", " << wireList[i] -> _fanin[j] -> getPatchFaninName2( wireList[i] ); 
			}
		}
		ouf << ");\n";
		*/

		wireList[i] -> outputGateInPatch( ouf, gateCnt );

		gate_number++;
	}

	// inverters
	for( size_t i = 0; i < inputInvList.size(); ++i )
	{
		_cost--;
		ouf << "not eco_gi" << gateCnt++ << "(" << inputInvList[i] -> getMappedFaninName(true);
		ouf << ", " << inputInvList[i] ->  getPatchFaninName2( 0, true );
		ouf << ");\n";

		if ( debug_p ) cout << "output: " <<  inputInvList[i] -> getName() << " create invertor " << endl;

	}

	if ( _useConst ) _cost ++;
	cout << "useConst: " << _useConst << endl;

	// endmodule
	ouf << "endmodule\n";
	ouf << "// patch cost = " << _cost << endl;
	ouf.close();
	cout << endl << "Patch generated with cost: " << _cost << ", and gate_number = " << gate_number << endl << endl;

}

void
NtkGate::outputGateInPatch( ofstream &ouf, unsigned &gateCnt )
{
	// check const at fanin
	// if exist, simplify the gate
	vector<bool> fanin_isConst;

	int	numFanin = 0;
	bool zero = false, one = false;
	int oneCount = 0;
	string name;

	for( size_t j = 0; j < _fanin.size(); j++ )
	{
		if( _recPair_fanin.empty() ||  _recPair_fanin[j] == 0 )
		{
			if ( _old ) name =  _fanin[j] -> getPatchFaninName2( this );
			else name = _fanin[j] -> getMappedFaninName();
		}
		else
		{
			if ( _old ) name =  _recPair_fanin[j] -> getMappedFaninName();
			else name = _recPair_fanin[j] -> getPatchFaninName2( this );
		}

		if ( name == "1'b1" ) 
		{
			one = true;
			oneCount ++;
			fanin_isConst.push_back(true);
		}
		else if ( name == "1'b0" )
		{
			zero = true;
			fanin_isConst.push_back(true);
		}
		else
		{
			numFanin++;
			fanin_isConst.push_back(false);
		}
		
	}

	ntkMgr -> _cost -= 2;

	if ( numFanin == _fanin.size() )
	{
		ouf <<  getTypeStr() << " eco_gn" << "(" <<  getPatchFaninName2( 0 );
		if ( _blackBoxFlag == 2 )
		{
			cout << "A shrinkable XOR found!" << endl;
			for( size_t i = 0; i < _xorInter.size(); i++ )
			{
				if ( _xorInter[i] -> _mg_list )
				{
					cout << "But cannot be used due to mg_list ..." << endl;
					break;
				}
				else if ( !_xorInter[i] -> _recPair_fanin.empty() )
				{
					cout << "But cannot be used..." << endl;
					break;
				}
			}
		}
		for (size_t j = 0 ; j < _fanin.size() ; ++j)
		{ 
			ntkMgr -> _cost ++;
			if( _recPair_fanin.empty() ||  _recPair_fanin[j] == 0 )
			{
				if ( _old ) ouf << ", " <<  _fanin[j] -> getPatchFaninName2( this );
				else ouf << ", " << _fanin[j] -> getMappedFaninName();
			}
			else
			{
				if ( _old ) ouf << ", " <<  _recPair_fanin[j] -> getMappedFaninName();
				else ouf << ", " <<  _recPair_fanin[j] -> getPatchFaninName2( this );
			}
		}
		ouf << ");\n";

	}
	else
	{

		bool const0 = false, const1 = false;
		if ( debug_p ) cout << "Patch Simplification: constant logic simplification." << endl;
		string newType =  getTypeStr();

		bool xorInvert = false;
		xorInvert = oneCount % 2;

		// TODO: check correctness 
		switch ( _type )
		{
			case NTK_GATE_BUF:
				if ( zero ) const0 = true;
				else const1 = true;
				break;
			case NTK_GATE_NOT:
				if ( zero ) const1 = true;
				else const0 = true;
				break;
			case NTK_GATE_AND:
				if ( zero ) const0 = true;
				else if ( numFanin == 1 ) newType = "buf";
				else if ( numFanin == 0 ) const1 = true;
				break;
			case NTK_GATE_NAND:
				if ( zero ) const1 = true;
				else if ( numFanin == 1 ) newType = "not";
				else if ( numFanin == 0 ) const0 = true;
				break;
			case NTK_GATE_OR:
				if ( one ) const1 = true;
				else if ( numFanin == 1 ) newType = "buf";
				else if ( numFanin == 0 ) const0 = true;
				break;
			case NTK_GATE_NOR:
				if ( one ) const0 = true;
				else if ( numFanin == 1 ) newType = "not";
				else if ( numFanin == 0 ) const1 = true;
				break;
			case NTK_GATE_XOR:
				if ( xorInvert ) 
				{
					newType = "xnor";
					if ( numFanin == 1 ) newType = "not";
					else if ( numFanin == 0 ) const1 = true;
				}
				else
				{
					if ( numFanin == 1 ) newType = "buf";
					else if ( numFanin == 0 ) const0 = true;
				}
				break;
			case NTK_GATE_XNOR:
				if ( xorInvert ) 
				{
					newType = "xor";
					if ( numFanin == 1 ) newType = "buf";
					else if ( numFanin == 0 ) const0 = true;
				}
				else
				{
					if ( numFanin == 1 ) newType = "not";
					else if ( numFanin == 0 ) const1 = true;
				}
				break;
		}

		if ( const0 || const1 )
		{
			if ( const0 ) ouf << "buf";
			else ouf << "not";

			ntkMgr -> _cost++;
			ouf << " eco_gc" << "(" <<  getPatchFaninName2( 0 ) << ", 1'b0);\n";
			ntkMgr -> _useConst = true;
			cout << "constant simplification here " << _name << endl;

		}
		else
		{
			// ouf << newType << " eco_gb" << gateCnt++ << "(" <<  getPatchFaninName2( 0 );
			ouf << newType << " eco_gs" << "(" <<  getPatchFaninName2( 0 );

			for (size_t j = 0 ; j < _fanin.size() ; ++j)
			{ 
				if ( fanin_isConst[j] ) continue;

				ntkMgr -> _cost++;
				if ( _old )
				{
					if ( _recPair_fanin.empty() || _recPair_fanin[j] == 0 )
					{
						ouf << ", " << _fanin[j] -> getPatchFaninName2(this);
					}
					else
					{
						ouf << ", " << _recPair_fanin[j] -> getMappedFaninName();
					}
				}
				else 
				{
					if ( _recPair_fanin.empty() || _recPair_fanin[j] == 0 )
					{
						ouf << ", " << _fanin[j] -> getMappedFaninName();
					}
					else
					{
						ouf << ", " << _recPair_fanin[j] -> getPatchFaninName2(this);
					}

				}
			}
			ouf << ");\n";

		}
	}
}


string
checkSpecialChar( string str )
{
	if ( (str.find(']') != string::npos) && (str[0] != '\\') )
	{
		return "\\" + str + " ";
	}
	else return str;
}

int
NtkGate::getPartialCost()
{
	if ( _flag == _globalFlag ) return MAX_COST;
	if ( _flag == _globalFlag-1 ) return 0;

	int empty_rec = _fanin.size();
	if ( _recPair_fanin.size() )
	{
		for( size_t i = 0; i < _recPair_fanin.size(); i++ )
		{
			if ( _recPair_fanin[i] ) empty_rec--;
		}
	}
	return 1 + _fanin.size()-2 + empty_rec;

}

string
NtkGate::getPatchFaninName2( NtkGate* from, bool input )
{
	char buf[100];
	string a, b = "_in";

	if ( !from )
	{
		if ( _old )
		{
			// if ( _name == "n_532_O" ) cout << "n_532 Here: " << this <<  " flag " << _flag << " global " << _globalFlag << endl;
			// input
			if ( input && _flag >= _globalFlag - 1 )
			{
				a = getNameShort() + b;
			}
			// output
			else a = getNameShort();
		}
		else
		{
			// wire
			sprintf( buf, "eco_w%d", _id );
			a = buf;
		}
		
	}
	else if ( !_old )
	{
			if ( _mg_list ) 
			{
				a = _mg_list -> getNameShort();
				if ( _mg_list -> _flag >= _globalFlag - 1 ) a = a + b;
			}
			else 
			{
				sprintf( buf, "eco_w%d", _id );
				a = buf;
			}
	}
	else
	{
		if ( from -> _old )
		{
			if ( _flag  == _globalFlag ) a = getNameShort() + b;
			else a = getNameShort();
		}
		else
		{
			NtkGate *ref = 0;
			for( size_t i = 0; i < from -> _recPair_fanin.size(); i++ )
			{
				if ( from -> _recPair_fanin[i] == this )
				{
					ref = from -> _fanin[i];
				}
			}
			assert(ref);
			// repair fanin: 
			if ( _flag == _globalFlag-1 ) a = getNameShort();
			else if ( _flag == _globalFlag && _allPatchTo == ref ) a = getNameShort();
			else  a = getNameShort() + b;
		}
	}

	return checkSpecialChar(a);
}

string
NtkGate::getPatchFaninName( unsigned c )
{
	// TODO  don't return local char array WTF
	if ( c == 1) return (getNameShort());

	string a, b = "_in";
	if ( ( _old || _type == NTK_GATE_PI || _mg_list) && c == 0) 
	{
		// TODO handle "_in"
		NtkGate *g = this;
		if ( !_old ) g = _mg_list;
		a = g -> getNameShort();

		if( g -> IsAllPatched() ) 
		{
			return (a+b);
		}
		else return (a);
		// a = _mg_list ->_name.substr( 0, _mg_list ->_name.size() - SUFFIX_LEN);
		// if(_mg_list -> IsPatchOutput() && !_pseudoMerge) return a+b;
		// //if(_mg_list -> IsPatchOutput() ) return a+b;
		// else return a;

	}
	if(c == 2)
	{
		a = _name.substr( 0, _name.size() - SUFFIX_LEN);
		if(IsPatchMoreThanHalf()) return (a+b);
		return (a);
	}
	
	char buf[100];
	sprintf( buf, "eco_w%d", _id );

	return buf;
}



void myStrip( char* buf )
{
	string str;
	str = buf;

	if ( str[str.size()-1] == '\r' ) 
	{
		str = str.substr(0, str.size()-1);
	}


	unsigned pos=0, len=str.size();

	if ( str[0] == ' ' ) pos = str.find_first_not_of( ' ' );
	if ( str[len-1] == ' ' ) len = str.find_last_not_of( ' ' );
	sprintf(buf, "%s", str.substr(pos, len+1).c_str());

	// TODO some error here (a few character will disapper at the begining of the line)
}

void
NtkMgr::applyPatch( string fname_old, string fname_patch, string fname_out )
{
	ifstream f_old(fname_old);
	ifstream f_patch(fname_patch);
	ofstream f_out(fname_out);

	char buf[1024], buf2[1024];
	string str;
	string name;

	unordered_set<string> rec;
	stringstream ss;

	// find patch outputs
	// cout << "Patch outputs:\n";
	bool done = false;
	while( f_patch.getline( buf, 1024 ) )
	{

		if ( strlen(buf) > 6 &&  !strncmp( buf, "output", 6 ) )
		{
			ss >> str;

			do
			{
				ss.clear();
				ss << buf;
				while( ss >> str )
				{
					// delete '\'
					char* p = const_cast<char*>(str.c_str());

					if (p[0] == '\\') 
					{ 
						rec.insert( str.substr(1, str.size()-1) );
						ss >> str;
					}
					else
					{
						rec.insert( str.substr(0, str.size()-1) );
					}
					


					// cout << str.substr(0, str.size()-1) << endl;
				}

				if ( str[str.size()-1] == ';' ) break;

				f_patch.getline( buf, 1024 );

			} while ( true );

			done = true;
		}

		if ( done ) break;
	}
	f_patch.clear();
	f_patch.seekg(0);

	// input, output, gates
	while( f_old.getline( buf, 1024) )
	{
		if ( strlen(buf) < 8 ) continue;

		myStrip( buf );
		// cout << "buf : " << buf << endl;

		// Hugo TODO: store PI
		string text, token;
		string delimiter = " ";
		size_t pos = 0;	

		// // debug
		// for (const auto &str : PIs) {
		// 	cout << "PI : " << str << endl;
		// }	
		//
		// Hugo TODO: find whether PI in rec
		vector<string> PI_in_rec{}; // a
		vector<string> PIname_add_in{}; // a_in
		NtkGate *g;
		for( unordered_set<string>::iterator it = rec.begin(); it != rec.end(); it ++)
		{
			g = findGateByName( *it + string("_O") );
			if ( g && g->getType() == NTK_GATE_PI ) 
			{
				// cout << "innnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn" << endl;
				PI_in_rec.push_back( *it );
				PIname_add_in.push_back( *it + string("_in") );
			}
		}
		// debug
		// PI_in_rec.push_back("a");

		if ( !strncmp( buf, "input", 5 ) || !strncmp( buf, "output", 6 ) || !strncmp( buf, "module", 6 ) )
		{
			f_out << buf << endl;

			while( buf[strlen(buf)-1]!=';') {
				f_old.getline(buf,1024);
				myStrip( buf );
				f_out << buf << endl;
			}
		}
		else if ( !strncmp( buf, "endmodule", 9 ) )
		{
			// add the gates in patch into patched.v
			while( f_patch.getline( buf2, 1024) )
			{
				if ( 	// gate
					!strncmp( buf2, "or", 2 ) || 
					!strncmp( buf2, "nor", 3 ) || 
					!strncmp( buf2, "and", 3 ) || 
					!strncmp( buf2, "nand", 4 ) || 
					!strncmp( buf2, "xor", 3 ) || 
					!strncmp( buf2, "xnor", 4 ) || 
					!strncmp( buf2, "not", 3 ) || 
					!strncmp( buf2, "buf", 3 )
					)		
				{

					// cout << "inside gate : " << buf2 << endl;
					// Hugo: input = "or eco_gb0(n_616, wc52, eco_w286)"
					string each_name = "", final_buf = "";
					string parser = ",";
					bool meet_bracket = false, finish_extract = false, do_parsing = false, match = false;
					for (size_t i = 0 ; i < strlen(buf2) ; ++i)
					{
						string temp;
						temp.push_back(buf2[i]);
						if (meet_bracket)
						{
							if (finish_extract) { final_buf.append(temp); }
							else 
							{ 
								if (temp == ",") { do_parsing = true; }
								if (temp == ")") { do_parsing = true; finish_extract = true; }
								if (!do_parsing) 
								{
									each_name = each_name + buf2[i];
								}
								// extract token
								if (do_parsing)
								{
									// erase whitespace
									char* p = const_cast<char*>(each_name.c_str());
									if (p[0] == ' ') { each_name.erase(each_name.begin()); }
									else if (p[0] == '\\') { each_name.erase(each_name.begin()); }
									else if (p[strlen(p)-1] == ' ') { each_name.erase(each_name.end()-1); }
									// cout << "hhhhhhhhhhhhhhhh : " << each_name << endl;
									// cout << "do parsing ; " << do_parsing << endl;
									// cout << "finish extract : " << finish_extract << endl;
									// a -> a_eco
									for (size_t j = 0 ; j < PI_in_rec.size() ; ++j)
									{
										// cout << "PI in rec : " << PI_in_rec[j] << endl;
										if (each_name == PI_in_rec[j])
										{
											// cout << "uuuuuuuuuuuuuu : " << each_name << endl;
											if (finish_extract)
											{
												each_name.append("_eco)");
												final_buf.append(each_name);
												break;
											}
											else
											{
												each_name.append("_eco, ");
												// cout << "ggggggggggggggggg : " << each_name << endl;
												final_buf.append(each_name);
												// cout << "oooooooooooooooooo : " << final_buf << endl;
												match = true;
												break;
											}
										}
									}
									// a_in -> a
									for (size_t j = 0 ; j < PIname_add_in.size() ; ++j)
									{
										if (each_name == PIname_add_in[j])
										{
											if (finish_extract)
											{
												each_name.erase(strlen(each_name.c_str())-3);
												each_name.append(")");
												final_buf.append(each_name);
												break;
											}
											else 
											{
												each_name.erase(strlen(each_name.c_str())-3);
												each_name.append(", ");
												final_buf.append(each_name);
												match = true;
												break;
											}
										}
									}
									if ((!match) && (!finish_extract))
									{
										each_name.append(", ");
										final_buf.append(each_name);
									}
									if (finish_extract)
									{
										each_name.append(")");
										final_buf.append(each_name);
									}
									match = false;
									do_parsing = false;
									each_name = ""; 
								}
							}
						}
						else
						{
							final_buf.append(temp);
							if (buf2[i] == '(') { meet_bracket = true; }
						}
					}
					// output
					strcpy(buf2, final_buf.c_str());
					// cout << "buf2 : " << buf2 << endl;

					f_out << buf2 << endl;
				}

			}
			f_out << buf << endl;
			break;
		}
		else if ( !strncmp( buf, "assign", 6 ) )	// assign
		{
			ss.clear();
			ss << buf;

			ss.getline( buf2, 1024, ' ');
			ss.getline( buf2, 1024, '=');
			myStrip(buf2);
			if ( rec.find(buf2) != rec.end() ) f_out << "wire " << buf2 << "_in;\n";

			ss.getline( buf2, 1024 );
			ss.clear();
			ss << buf;

			ss.getline( buf2, 1024, ' ');
			f_out << buf2 << ' ';

			ss.getline( buf2, 1024, '=');
			myStrip(buf2);
			f_out << buf2;

			if ( rec.find(buf2) != rec.end() ) f_out << "_in";

			f_out << " =";
			ss.getline( buf2, 1024 );

			// cout << "last assign buf2 : " << buf2 << endl;
			// Hugo: a -> a_eco
			string check_buf2 = buf2;
			string final_buf2 = "";
			bool has_semicomma = false, match = false;
			char* p = const_cast<char*>(buf2);
			if (p[0] == ' ') 
			{ 
				final_buf2.append(" ");
				check_buf2.erase(0, 1); 
			}
			if (p[strlen(p)-1] == ';') 
			{ 
				check_buf2.erase(check_buf2.end()-1); 
				has_semicomma = true;
			}
			for (size_t i = 0 ; i < PI_in_rec.size() ; ++i)
			{
				if (check_buf2 == PI_in_rec[i])
				{
					check_buf2.append("_eco");
					final_buf2.append(check_buf2);
					match = true;
				}
			}
			if (!match) { final_buf2.append(check_buf2); }
			if (has_semicomma) { final_buf2.append(";"); }
			strcpy(buf2, final_buf2.c_str());
			// cout << "buf2 : " << buf2 << endl;

			f_out << buf2 << endl;


		}
		else if ( 	// gate
			!strncmp( buf, "or", 2 ) || 
			!strncmp( buf, "nor", 3 ) || 
			!strncmp( buf, "and", 3 ) || 
			!strncmp( buf, "nand", 4 ) || 
			!strncmp( buf, "xor", 3 ) || 
			!strncmp( buf, "xnor", 4 ) || 
			!strncmp( buf, "not", 3 ) || 
			!strncmp( buf, "buf", 3 )
			)		
		{

			// Hugo
			vector<string> words_g1{};
			//

			ss.clear();
			ss << buf;
			// Hugo: buf = and g1(o, a, b, c);
			// cout << "buf : " << buf << endl;

			ss.getline( buf2, 1024, '(');
			f_out << buf2 << '(';

			ss.getline( buf2, 1024, ',');

			myStrip(buf2);

			// Hugo: buf2 = o
			// cout << "buf2 second : " << buf2 << endl;
			string buf2_str = buf2;
			for (size_t i = 0 ; i < PI_in_rec.size() ; ++i)
			{
				if (buf2_str == PI_in_rec[i])
				{
					buf2_str.append("_eco");
				}
			}
			strcpy(buf2, buf2_str.c_str());
			//

			f_out << buf2;
			if ( rec.find(buf2) != rec.end() ) f_out << "_in";

			f_out << ',';
			ss.getline( buf2, 1024, ';' );

			// Hugo: buf2 = a, b, c)
			// has "comma" --> "wc_37, n_802)"
			// no "comma" --> "n_801)"
			// cout << "buf2 third : " << buf2 << endl;
			
			string each_name = "", final_buf = "";
			string parser = ",";
			bool meet_bracket = false, finish_extract = false, do_parsing = false, match = false;
			for (size_t i = 0 ; i < strlen(buf2) ; ++i)
			{
				string temp;
				temp.push_back(buf2[i]);
				if (finish_extract) { final_buf.append(temp); }
				else 
				{ 
					if (temp == ",") { do_parsing = true; }
					if (temp == ")") { do_parsing = true; finish_extract = true; }
					if (!do_parsing) 
					{
						each_name = each_name + buf2[i];
					}
					// extract token
					if (do_parsing)
					{
						// erase whitespace
						char* p = const_cast<char*>(each_name.c_str());
						if (p[0] == ' ') { each_name.erase(each_name.begin()); }
						else if (p[strlen(p)-1] == ' ') { each_name.erase(each_name.end()-1); }
						// a -> a_eco
						for (size_t j = 0 ; j < PI_in_rec.size() ; ++j)
						{
							if (each_name == PI_in_rec[j])
							{
								if (finish_extract)
								{
									each_name.append("_eco)");
									final_buf.append(each_name);
									break;
								}
								else
								{
									each_name.append("_eco, ");
									final_buf.append(each_name);
									match = true;
									break;
								}
							}
							cout << "Apply: " << buf << " " << each_name << endl;
						}
						if ((!match) && (!finish_extract))
						{
							each_name.append(", ");
							final_buf.append(each_name);
						}
						if (finish_extract)
						{
							each_name.append(")");
							final_buf.append(each_name);
						}
						match = false;
						do_parsing = false;
						each_name = ""; 
					}
				}
			}
			// output
			// cout << "buf2 g1.v : " << buf2 << endl;
			strcpy(buf2, final_buf.c_str());
			//

			f_out << buf2 << ';' << endl;
		}
	}
	f_old.close();

	f_patch.close();
	f_out.close();


	cout << "patched file generated." << endl;

}

void
NtkGate::postOrderMark()
{
	if ( _flagB == _globalFlagB )  return;

	_flagB = _globalFlagB;

	if ( _old )
	{
		if ( _recPair_fanin.empty() )
		{
			for( size_t i = 0; i < _fanin.size(); i++ )  _fanin[i] -> postOrderMark();
		}
		else
		{
			for( size_t i = 0; i < _recPair_fanin.size(); i++ )
			{
				if ( _recPair_fanin[i] ) 
				{
					_recPair_fanin[i] -> getMappedFanin() -> postOrderMark();
				}
				else _fanin[i] -> postOrderMark( );
			}
		}
		if ( _recPair_fanout.empty() )  ntkMgr -> _netList_patched.push_back(this);
		// cout << "patch netlist: add " << _name << endl;
	}
	else
	{
		if ( ! _recPair_fanout.empty() )
		{
			_recPair_fanout[0] -> postOrderMark();
		}
		else
		if ( _recPair_fanin.empty() )
		{
			for( size_t i = 0; i < _fanin.size(); i++ )  _fanin[i] -> getMappedFanin() -> postOrderMark();
			ntkMgr -> _netList_patched.push_back(this);
		}
		else
		{
			for( size_t i = 0; i < _recPair_fanin.size(); i++ )
			{
				if ( _recPair_fanin[i] ) _recPair_fanin[i] -> postOrderMark();
				else _fanin[i] -> getMappedFanin() -> postOrderMark();
			}
			ntkMgr -> _netList_patched.push_back(this);
		}
	}
	
}


void
NtkMgr::handlePrePatchPair( NtkGate *p_rev, NtkGate *p_gol )
{
	NtkGate *mergeInGol = p_rev -> _mg_list;

	if( !mergeInGol ) { throw ERR_RESET_MGR; }
	// assert( g_rev -> _mg_next == 0 );


	if ( _p_eq )
	{
		// if p and p' are eq;
		// add  recPair fanin & recPair fanout for all golden that merge with p
		while( mergeInGol )
		{
			cout << "adding bwd patch for " << mergeInGol -> getName() << endl;

			for( size_t i = 0; i < mergeInGol -> _fanout.size(); i++ )
			{
				NGateList &recFanin = mergeInGol -> _fanout[i] -> _recPair_fanin;
				NGateList &fanin = mergeInGol -> _fanout[i] -> _fanin;
				cout << "Fanout: " << mergeInGol -> _fanout[i] -> getName() << endl;
				if ( recFanin.empty() ) recFanin.resize( fanin.size() );
				for( size_t j = 0; j < fanin.size(); j++ )
				{
					if ( fanin[j] == mergeInGol ) recFanin[j] = _p_rev;
				}
			}
			mergeInGol -> _recPair_fanout.push_back( _p_rev );

			mergeInGol = mergeInGol -> _mg_next;
		}
	}
	else
	{
		/*
		// remove merge list
		_mg_list_recover.clear();
		while( mergeInGol )
		{
			cout << "remove merge info for " << mergeInGol -> getName() << endl;

			_mg_list_recover.push_back( mergeInGol );
			mergeInGol -> _mg_list = 0;
			mergeInGol = mergeInGol -> _mg_next;

		}
		*/

	}
	
}

void
NtkMgr::restorePrePatchPair()
{
	if ( !_p_eq )
	{
		for( size_t i = 0; i < _mg_list_recover.size(); i++ )
		{
			if ( _mg_list_recover[i] -> _mg_list == 0  ) 
				_mg_list_recover[i] -> _mg_list = _p_rev;
		}
	}
	_mg_list_recover.clear();
}

void
NtkMgr::markFloatingGates( NtkGate *p_rev, NtkGate *p_gol )
{
	_in2Aig.clear();

	CirGate* forbidden = 0;
	if ( p_rev && p_gol)
	{
		forbidden = p_rev -> _aigMiterNode.gate();
		for( size_t i = 0; i < _PIList_old.size(); i++ )
		{
			if ( getGate(_PIList_old[i]) -> _aigMiterNode.gate() == forbidden )
			{
				forbidden = 0;
				break;
			}
		}
	}

	// save all aig to _in2Aig
	for( size_t i = 0; i < _netList_old.size(); i++ )
	{
		NtkGate *g  = getGate(_netList_old[i]);

		if ( g->_name.substr(0, 5) == "eco_w" ) continue;
		if ( forbidden && g->_aigMiterNode.gate() == forbidden ) continue;
		if ( g -> _aigMiterNode.gate() )
		{
			InputAigEntry &entry = _in2Aig[g->_aigMiterNode.gate()];

			if ( !entry.gate || g -> getLevel() < entry.gate -> getLevel() ) 
			{
				entry.gate = g;
				entry.phase = g -> _aigMiterNode.phase();
			}
		}
		// if ( g-> _mg_list ) cout << g -> getName() << " merge with " << g->_mg_list -> getName() << endl;
	}

	// traverse patched
	_netList_patched.clear();
	NtkGate::_globalFlagB++;


	for( int i = _recPairList.size()-1; i >= 0; i-- )
	{
		_netList_patched.push_back(_recPairList[i]);
	}

	if ( p_rev && p_gol )
	{

		p_rev -> _flagB = NtkGate::_globalFlagB;
		string p_name = p_rev -> getName();

		p_name = p_name.substr( 0, p_name.size() - SUFFIX_LEN ) + string("_in_O");
		NtkGate *p_in = findGateByName( p_name );

		// mark original p
		// make the new p floating

		if(!p_in) { throw "no p_in"; }
		p_in -> postOrderMark();

		// if(!_p_eq) p_gol -> postOrderMark();
	}

	// mark gates that are not floating
	for( size_t i = 0; i < _dummyPOList.size(); i++ )
	{
		_dummyPOList[i] -> postOrderMark();
	}

}


void
NtkGate::decidePatchType()
{
	// cout << "decide patch type: " << _name << endl;

	if ( !_old )
	{
		ntkMgr -> _patchWireList.push_back( this );
		checkPatchFanin();
		return;
	}

	if ( _recPair_fanout.empty() )
	{
		if ( _flag == _globalFlag - 1 )  // PAR
		{
			checkPatchFanin();
			if ( debug_p ) cout << "Patch: " << _name << " will partial fix itself" << endl;

			if ( this == ntkMgr -> _p_in ) 
			{
				ntkMgr -> _p_rev -> _flag = _globalFlag - 1;
				if ( debug_p ) cout << "Patch: in signify original " << endl;
			}
			if ( this != ntkMgr -> _p_rev )
			{
				ntkMgr -> _patchOutputList.push_back( this );
			}
		}
		return;
	}

	// if is already PAR 
	// then it can only fix itself
	if ( _flag == _globalFlag -1 || _type == NTK_GATE_CONST )
	{
		for( size_t i = 0; i < _fanout.size(); i++ )
		{
			if ( _fanout[i] -> getRecPairFanin(this) != 0 )
			{
				_fanout[i] -> _flag = _globalFlag-1;
				if ( debug_p ) cout << "      " << _fanout[i] -> getName() << " is set to be PAR" << endl;
			}
			// TODO: check it's not ALL
		}
		if ( _type != NTK_GATE_CONST ) ntkMgr -> _patchOutputList.push_back( this );

		if ( this != ntkMgr -> _p_rev ) checkPatchFanin();

		if ( debug_p ) cout << "Patch: " << _name << " will be kept since it's already partial" << endl;
		return;
	}

	// if it's already ALL
	if ( _flag == _globalFlag )
	{
		// for those not _patchTo, set to P
		for( size_t i = 0; i < _fanout.size(); i++ )
		{
			if ( (_fanout[i] -> getRecPairFanin(this) != _allPatchTo) && (_fanout[i] -> _flagB == _globalFlagB) )
			{
				_fanout[i] -> _flag = _globalFlag - 1; // PAR
				if ( debug_p ) cout << "      " << _fanout[i] -> getName() << " is set to be PAR" << endl;
			}
		}
		ntkMgr -> _patchOutputList.push_back( this );
		if ( debug_p ) cout << "Patch: " << _name << " already have to be all patched to " << _allPatchTo -> getNameShort()  << endl;
		return;
	}


	int cost_keep = 1; 
	vector<int> cost_rec;
	cost_rec.resize( _recPair_fanout.size(), 0 );

	NtkGate *rec;
	int cost;


	// calcultate cost
	for( size_t i = 0; i < _fanout.size(); i++ )
	{
		if ( _fanout[i] -> _flagB != _globalFlagB ) continue;	// floating

		rec = _fanout[i] -> getRecPairFanin( this );
		cost = _fanout[i] -> getPartialCost();

		if ( !rec ) cost_keep += cost;
		else
		{
			for( size_t j = 0; j < _recPair_fanout.size(); j++ )
			{
				if ( _recPair_fanout[j] == rec ) cost_rec[j] += cost;
				break;
			}
		}
	}

	// choose one
	// TODO: check if there're more than one > 2000
	int max = cost_keep, max_index = -1;
	for( size_t i = 0; i < _recPair_fanout.size(); i++ )
	{
		if ( cost_rec[i] > max ) 
		{
			max_index = i;
			max = cost_rec[i];
		}
	}

	// set p for those not chosed
	if ( max_index == -1 )
	{
		for( size_t i = 0; i < _fanout.size(); i++ )
		{
			if ( _fanout[i] -> getRecPairFanin(this) != 0 )
			{
				_fanout[i] -> _flag = _globalFlag-1;
				if ( debug_p ) cout << "      " << _fanout[i] -> getName() << " is set to be PAR" << endl;
			}
		}
		if ( debug_p ) cout << "Patch: " << _name << " will be kept" << endl;
	}
	else
	{
		for( size_t i = 0; i < _fanout.size(); i++ )
		{
			if ( _fanout[i] -> getRecPairFanin(this) != _recPair_fanout[max_index]  && _fanout[i] -> _flagB )
			{
				_fanout[i] -> _flag = _globalFlag-1;
				if ( debug_p ) cout << "      " <<  _fanout[i] -> getName() << " is set to be PAR" << endl;
			}
		}

		_flag = _globalFlag; 	// ALL
		ntkMgr -> _patchOutputList.push_back( this );
		_allPatchTo = _recPair_fanout[max_index];
		if ( debug_p ) cout << "Patch: " << _name << " will be all patched to " << _allPatchTo -> getName() << endl;
	}

	return;
	
}

void
NtkGate::checkPatchFanin()
{
	if ( _old )	// PAR
	{
		// TODO: p?
		for( size_t i = 0; i < _fanin.size(); i++ )
		{
			// if not rec fanin
			if ( _recPair_fanin.empty() || _recPair_fanin[i] == 0 )
			{
				if ( _fanin[i] -> _flag != _globalFlag - 1  )
					ntkMgr -> _patchInputSet.insert( _fanin[i] );
			}
			else if ( _recPair_fanin[i] -> getMappedFanin() -> _old )
			{
				ntkMgr -> _patchInputSet.insert( _recPair_fanin[i] -> getMappedFanin() );
			}
			// else it use golden that is already in wirelist
		}
	}
	else
	{
		for( size_t i = 0; i < _fanin.size(); i++ )
		{
			// if not rec fanin
			if ( !_recPair_fanin.empty() && _recPair_fanin[i] )
			{
				// repair fanin: 
				if ( _recPair_fanin[i] -> _flag == _globalFlag-1 ) continue;
				else if ( _recPair_fanin[i] -> _flag == _globalFlag && _recPair_fanin[i] -> _allPatchTo == _fanin[i] ) continue;
				else ntkMgr -> _patchInputSet.insert( _recPair_fanin[i] );
			}
			/*
			else
			{
				if ( _fanin[i] -> _mg_list )
				{
					ntkMgr -> _patchInputSet.insert( _fanin[i] -> _mg_list );
				}
				// else golden
			}
			*/
			
		}
	}
}


// TODO: handle recPair fanout!!

// can only called with gold with no recpair fanin
// for traverse purpose
NtkGate*
NtkGate::getMappedFanin()
{
	if ( !_recPair_fanout.empty() )
	{
		return _recPair_fanout[0];
	}

	if ( !_aigMiterNode.gate() ) return this;
	InputAigEntry &e = ntkMgr -> _in2Aig[_aigMiterNode.gate()];

	if ( !e.gate )
	{
		return this;
	}
	else
	{
		e.used = true;

		if ( e.phase != _aigMiterNode.phase() )
		{
			e.needInvert = true;
			cout << "Patch: " << _name << " == !"  << e.gate -> getName() << endl;
		}

		return e.gate;
	}
}

// can only called with gold with no recpair fanin
string
NtkGate::getMappedFaninName( bool inv )
{


	string name;
	if ( inv )
	{
		if( !(ntkMgr->_in2Aig[_aigMiterNode.gate()].used && ntkMgr->_in2Aig[_aigMiterNode.gate()].needInvert) ) { throw ERR_RESET_REC; }
		return checkSpecialChar( getNameShort() + "_eco_inv" );
	}
	if ( !_recPair_fanout.empty() )
	{
		name = _recPair_fanout[0]->getNameShort();
		if ( _recPair_fanout[0] -> _flag  == _globalFlag )
		{
			if( !(ntkMgr -> _patchInputSet.find(_recPair_fanout[0]) != ntkMgr -> _patchInputSet.end() )) { throw ERR_RESET_REC; }
			name += "in";
		}
		return checkSpecialChar(name);
	}

	if ( !_aigMiterNode.gate() ) 
	{
		if( !(_flagB == _globalFlagB) ) { throw ERR_RESET_REC; }
		return checkSpecialChar( getNameShort() );
	}
	InputAigEntry &e = ntkMgr -> _in2Aig[_aigMiterNode.gate()];

	if ( !e.gate )
	{
		if( !(_flagB == _globalFlagB) ) { throw ERR_RESET_REC; }
		name = "eco_w" + to_string( (long long) _id );
	}
	else
	{
		string a = e.gate -> getNameShort(), in = "_in";
		if( !(ntkMgr -> _in2Aig[e.gate->_aigMiterNode.gate()] .used) ) { throw ERR_RESET_REC; }

		if ( e.phase != _aigMiterNode.phase() )
		{
			if ( e.gate -> _type == NTK_GATE_CONST ) name = "1'b1";
			else name = a + "_eco_inv";

			if(!(ntkMgr->_in2Aig[e.gate->_aigMiterNode.gate()].needInvert) ) { throw ERR_RESET_REC; }
		}
		else
		{
			if ( e.gate -> _flag >= _globalFlag - 1 )
			{
				name = a + in;
			}
			else name = a;
		}
	}

	// TODO: maybe handle special char here
	return checkSpecialChar( name );
}

