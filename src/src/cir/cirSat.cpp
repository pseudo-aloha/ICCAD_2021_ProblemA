#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"

using namespace std;

extern SatSolver* solver;

void CirMgr::satReset()
{
}

// other: add clause
// _flag == -1 / 0 : return
// _flag == -2 : cut, set falg to -1 and return
// _flag == -3 : cut, nwe var, set flag to -1 and return
//
// to extend and reuse the model
// 		check on the new cut
//			if _flag == 0: rebuild
//			if _flag == -1: set to -3
//			set to -2
// 		start from the last boundary 
// 		
void AigGate::addClause() 
{
// debug
// cout << "adding aig clause " << _id << endl;
   solver -> addAigCNF( _var, _fanin[0].gate() -> getVar(), _fanin[0].phase(),
      _fanin[1].gate() -> getVar(), _fanin[1].phase() );
}
void AigGate::addClauseRec() 
{

   if ( _flag >= _globalFlag - 1 ) return;
   if ( _flag >= _globalFlag - 3 ) 
   {
	   _flag = _globalFlag - 1;
   		if ( _flag == _globalFlag -2 )  return;

//  cout  << _id << " add clause recursively" << endl;

		_var = solver -> newVar();
	   return;
   }

//  cout  << _id << " add clause recursively" << endl;

	_var = solver -> newVar();
   _flag = _globalFlag;
   _fanin[0].gate() -> addClauseRec();
   _fanin[1].gate() -> addClauseRec();
   addClause();
}
void POGate::addClause()
{
	solver -> addEqCNF( _var, _fanin[0].gate() -> getVar(), _fanin[0].phase() );
}
void POGate::addClauseRec()
{
   if ( _flag >= _globalFlag - 1 ) return;
   if ( _flag >= _globalFlag - 3 ) 
   {
	   _flag = _globalFlag - 1;
   		if ( _flag == _globalFlag -2 )  return;
		_var = solver -> newVar();

//  cout  << _id << " add clause recursively" << endl;

	   return;
   }

//  cout  << _id << " add clause recursively" << endl;

	_var = solver -> newVar();
   _flag = _globalFlag;
   _fanin[0].gate() -> addClauseRec();
   addClause();
}

void PIGate::addClauseRec()
{
   if ( _flag >= _globalFlag - 1 ) return;
   if ( _flag >= _globalFlag - 3 ) 
   {
	   _flag = _globalFlag - 1;
   		if ( _flag == _globalFlag -2 )  return;

//  cout  << _id << " add clause recursively" << endl;

		_var = solver -> newVar();
	   return;
   }

//  cout  << _id << " add clause recursively" << endl;
	_var = solver -> newVar();
	_flag = _globalFlag;
	
}

void ConstGate::addClauseRec()
{
   if ( _flag >= _globalFlag - 1 ) return;
   if ( _flag >= _globalFlag - 3 ) 
   {
	   _flag = _globalFlag - 1;
   		if ( _flag == _globalFlag -2 )  return;

//  cout  << _id << " add clause recursively" << endl;
		_var = solver -> newVar();

	   return;
   }

//  cout  << _id << " add clause recursively" << endl;

	_var = solver -> newVar();
	//solver -> assertProperty(_var, false);
	_flag = _globalFlag;
	
}