#include "ntkGate.h"
#include "sat.h"

#include <iomanip>

using namespace std;

extern SatSolver *solver;


// // TODO: reuse sat model
// _flag == -1 / 0 : return
// _flag == -2 : not on cut
// 				but the variable has been declared
//				 (the gate was on previous cut)
// _flag == -3 : cut, the variable needs to be declared
//
// to extend and reuse the model
// 		check on the new cut
//			if _flag == 0: rebuild
//			if _flag == -1: set to -3
//			set to -3
// 		start from the last boundary 

void
NtkGate::addClauseRec( int level )
{
   	if ( _flag == _globalFlag ) return;

	_flag = _globalFlag;

	// create variable
	_satVar = solver->newVar();
	if ( _type == NTK_GATE_NAND ||
		 _type == NTK_GATE_OR ||
		 _type == NTK_GATE_XNOR ||
		 _type == NTK_GATE_NOT ) _satPhase = true;
	else _satPhase = false;

	if ( _level <= level ) 
	{
		//cout << setw(W_NAME) << _name << " add var" << endl;
		return;
	}

	// call fanin
	for( size_t i = 0; i < _fanin.size(); i ++ )
		_fanin[i] -> addClauseRec( level );

	// addclause
	addClause();

	//cout << setw(W_NAME) << _name << " add clause (" << getTypeStr() << ")" << endl;
}

void
NtkGate::addClauseRec()
{
   	if ( _flag == _globalFlag ) return;

	// create variable
	_satVar = solver->newVar();

	if ( _flag == _globalFlag - 1 ) 
	{
		_flag = _globalFlag;
		//cout << setw(W_NAME) << _name << " add var" << endl;
		return;
	}
	_flag = _globalFlag;

	// call fanin
	for( size_t i = 0; i < _fanin.size(); i ++ )
		_fanin[i] -> addClauseRec();

	// addclause
	addClause();
	//cout << setw(W_NAME) << _name << " add clause (" << getTypeStr() << ")" << endl;
}

void
NtkGateBuf::addClause()
{
	assert(_fanin.size() == 1);
	solver -> addEqCNF( _satVar, _fanin[0]->getSatVar(), _fanin[0]->getSatPhase() );
}
void
NtkGateNot::addClause()
{
	assert(_fanin.size() == 1);
	solver -> addEqCNF( _satVar, _fanin[0]->getSatVar(), _fanin[0]->getSatPhase() );
}
void
NtkGateAnd::addClause()
{
	assert(_fanin.size() > 1);

	Var v, v_prev = _fanin[0] -> getSatVar();
	bool phase_prev = _fanin[0] -> getSatPhase();

	for( size_t i = 1; i < _fanin.size(); i++ )
	{
		if ( i == _fanin.size() - 1 ) v = _satVar;
		else v = solver -> newVar();

		solver -> addAigCNF( v, v_prev, phase_prev,
		_fanin[i] -> getSatVar(), _fanin[i] -> getSatPhase() );

		v_prev = v;
		phase_prev = false;
	}
}
void
NtkGateNand::addClause()
{
	assert(_fanin.size() > 1);

	Var v, v_prev = _fanin[0] -> getSatVar();
	bool phase_prev = _fanin[0] -> getSatPhase();

	for( size_t i = 1; i < _fanin.size(); i++ )
	{
		if ( i == _fanin.size() - 1 ) v = _satVar;
		else v = solver -> newVar();

		solver -> addAigCNF( v, v_prev, phase_prev,
		_fanin[i] -> getSatVar(), _fanin[i] -> getSatPhase() );

		v_prev = v;
		phase_prev = false;
	}
}
void
NtkGateOr::addClause()
{
	assert(_fanin.size() > 1);

	Var v, v_prev = _fanin[0] -> getSatVar();
	bool phase_prev = !_fanin[0] -> getSatPhase();

	for( size_t i = 1; i < _fanin.size(); i++ )
	{
		if ( i == _fanin.size() - 1 ) v = _satVar;
		else v = solver -> newVar();

		solver -> addAigCNF( v, v_prev, phase_prev,
		_fanin[i] -> getSatVar(), !_fanin[i] -> getSatPhase() );

		v_prev = v;
		phase_prev = false;
	}
}
void
NtkGateNor::addClause()
{
	assert(_fanin.size() > 1);

	Var v, v_prev = _fanin[0] -> getSatVar();
	bool phase_prev = !_fanin[0] -> getSatPhase();

	for( size_t i = 1; i < _fanin.size(); i++ )
	{
		if ( i == _fanin.size() - 1 ) v = _satVar;
		else v = solver -> newVar();

		solver -> addAigCNF( v, v_prev, phase_prev,
		_fanin[i] -> getSatVar(), !_fanin[i] -> getSatPhase() );

		v_prev = v;
		phase_prev = false;
	}
}
void
NtkGateXor::addClause()
{
	assert(_fanin.size() > 1);

	Var v, v_prev = _fanin[0] -> getSatVar();
	bool phase_prev = _fanin[0] -> getSatPhase();

	for( size_t i = 1; i < _fanin.size(); i++ )
	{
		if ( i == _fanin.size() - 1 ) v = _satVar;
		else v = solver -> newVar();

		solver -> addXorCNF( v, v_prev, phase_prev,
		_fanin[i] -> getSatVar(), _fanin[i] -> getSatPhase() );

		v_prev = v;
		phase_prev = false;
	}
}
void
NtkGateXnor::addClause()
{
	assert(_fanin.size() > 1);

	Var v, v_prev = _fanin[0] -> getSatVar();
	bool phase_prev = _fanin[0] -> getSatPhase();

	for( size_t i = 1; i < _fanin.size(); i++ )
	{
		if ( i == _fanin.size() - 1 ) v = _satVar;
		else v = solver -> newVar();

		solver -> addXorCNF( v, v_prev, phase_prev,
		_fanin[i] -> getSatVar(), _fanin[i] -> getSatPhase() );

		v_prev = v;
		phase_prev = false;
	}
}


void
NtkGateConst::addClauseRec( int level )
{
   	if ( _flag == _globalFlag ) return;

	NtkGate::addClauseRec( level );

	if ( !_old ) 
	{
		solver -> assertProperty( _satVar, false );
	}

}

void
NtkGateConst::addClauseRec()
{
   	if ( _flag == _globalFlag ) return;

	NtkGate::addClauseRec();

	if ( !_old ) 
	{
		solver -> assertProperty( _satVar, false );
	}

}