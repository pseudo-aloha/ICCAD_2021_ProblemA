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
#include <vector>
#include "cirMgr.h"
#include "cirGate.h"
#include "../ntk/ntkMgr.h"
#include "../util/util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
extern NtkMgr*	ntkMgr;
extern CirMgr* 	aigOld;
extern const char*			SUFFIX_OLD;
extern const char*			SUFFIX_GOL;

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0; // in printint, lineNo needs to ++
static char buf[1024];
static string errMsg;

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
CirMgr::~CirMgr()
{
	reset();
}
CirGate *CirMgr::getGate(unsigned gid) const
{
	if (gid > _numVar + _numPO || !_gateList[gid] || _gateList[gid]->getType() == UNDEF_GATE)
	{
		cout << "wrong gate id" << endl;
		return 0;
	}
	return _gateList[gid];
}
CirGate	*CirMgr::getPO(unsigned po) const {
	return (po < _numPO) ? getGate(_numVar+po+1): 0; 
}
void CirMgr::reset()
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
	_numLatch - 0;
	_numPO = 0;
	_numAig = 0;
	_numNet = 0;


	_gateList.clear();
	_PIList.clear();
	_netList.clear();
	_aigNetlist.clear();
	_netAigIds.clear();


	_numGoodBit = 0;
	_numSim = 0;

}
bool CirMgr::readCircuit(const string &fileName, CirType type )
{
	reset();
	lineNo = 0;
	ifstream inf(fileName);
	_type = type;

	// in every read function
	// just use it's fanins' id as fanin to create the gate
	readHeader(inf);
	for (size_t i = 0; i < _numPI; ++i)
		readPI(inf);

	if ( _type == CIR_MITER ) 
	{
		_numNet = _numPO-1;
		_numPO = 1;
	} 
	else if ( _type != CIR_SEL )
	{
		_numNet = _numPO - ntkMgr->getNumPO();
		_numPO  = ntkMgr->getNumPO();
	}

	for (size_t i = 0; i < _numPO; ++i) 
		readPO(inf);
	for (size_t i = 0; i < _numNet; ++i )
		readNet(inf);

	for (size_t i = 0; i < _numAig; ++i)
		readAig(inf);

	// TODO: map constant
	linkConst();

	readSymbol(inf);
	inf.close(); 

	// to construct the circuit
	// store fanin's id in a gate first
	// (since the gate may not be constructed yet)
	// then call connect() to create actual fanin's pointer
	connect();

	// dfs back track on each po
	// to get a topological order id list (_netList)
	genNetlist();

	return true;
}
void CirMgr::linkConst()
{
	string name;

	if ( _type == CIR_MITER || _type == CIR_OLD )
	{
		name = "1'b0"; name += SUFFIX_OLD;
		ntkMgr -> mapGate2Aig( name, _gateList[0], 0, _type );
		name = "1'b1"; name += SUFFIX_OLD;
		ntkMgr -> mapGate2Aig( name, _gateList[0], 1, _type );
	}

	if ( _type == CIR_MITER || _type == CIR_GOL )
	{
		name = "1'b0"; name += SUFFIX_GOL;
		ntkMgr -> mapGate2Aig( name, _gateList[0], 0, _type );
		name = "1'b1"; name += SUFFIX_GOL;
		ntkMgr -> mapGate2Aig( name, _gateList[0], 1, _type );
	}

}
void CirMgr::readHeader(ifstream &inf)
{
	inf >> buf >> _numVar >> _numPI >> _numPO >> _numPO >> _numAig;
	_gateList.clear();
	_gateList.resize(_numVar + _numPO + 1, 0);
	_gateList[0] = new ConstGate();
	_PIList.reserve(_numPI);
	_numNet = 0;
	++lineNo;
}
void CirMgr::readPI(ifstream &inf)
{
	uint id;
	inf >> id;
	id /= 2;
	_gateList[id] = new PIGate(id);
	//debug
	//cout << "build pi with id " << id << endl;
	_PIList.push_back(id);
	++lineNo;
}
void CirMgr::readPO(ifstream &inf)
{
	uint fanin;
	inf >> fanin;
	uint id = _numVar + lineNo - _numPI;
	_gateList[id] = new POGate(id, fanin);
	//debug
	++lineNo;
}
void CirMgr::readNet(ifstream &inf)
{
	uint fanin;
	inf >> fanin;
	_netAigIds.push_back(fanin);
	++lineNo;
}
void CirMgr::readAig(ifstream &inf)
{
	uint id, fanin[2];
	inf >> id;
	id /= 2;
	inf >> fanin[0] >> fanin[1];
	_gateList[id] = new AigGate(id, fanin[0], fanin[1]);

// debug
//cout  << "create aig gate with id " << id  << endl;

	++lineNo;
}
void CirMgr::readSymbol(ifstream &inf )
{
	unsigned id;
	char type;
	string symbol;
	inf.get();
	while (inf.get(type))
	{
		if (type != 'i' && type != 'o') break;

		inf >> id;
		inf.ignore();
		getline(inf, symbol);

		if ( type == 'i') 
		{
			_gateList[_PIList[id]] -> _name = symbol;
			if ( _type == CIR_OLD || _type == CIR_MITER ) ntkMgr -> mapGate2Aig( symbol + SUFFIX_OLD, getGate(_PIList[id]), 0, _type );
			if ( _type == CIR_GOL || _type == CIR_MITER ) ntkMgr -> mapGate2Aig( symbol + SUFFIX_GOL, getGate(_PIList[id]), 0, _type );
			continue;
		}
		if ( id < _numPO ) {
			_gateList[_numVar+1] -> _name = symbol;
			continue;
		}
		id -= _numPO;
		ntkMgr -> mapGate2Aig( symbol, getGate(_netAigIds[id]/2), _netAigIds[id]%2, _type );
	}
}
void CirMgr::connect()
{
	for (size_t i = 0; i < _gateList.size(); ++i)
	{
		if (_gateList[i])
		{
			_gateList[i]->connect(_gateList);
		}
	}
	// find not used gate
	for (size_t i = 1; i <= _numVar; ++i)
	{
		if (_gateList[i] && _gateList[i]->_fanout.empty())
		{
			_unUsedList.push_back(i);
		}
	}
}
void CirMgr::genNetlist()
{
	CirGate::_globalFlag += 3;

	_netList.clear();
	_aigNetlist.clear();

/*
	for (size_t i = _numVar + 1; i <= _numVar + _numPO; ++i)
	{
		_gateList[i]->postOrderVisit(_netList, _aigNetlist);
	}
*/
	for ( size_t i = 0; i <= _numVar+_numPO; i++ )
	{
		if ( _gateList[i] ) _gateList[i] -> postOrderVisit( _netList, _aigNetlist );
	}
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void CirMgr::printSummary() const
{
	cout << "\nCircuit Statistics\n==================" << endl;
	cout << "  PI   " << right << setw(9) << _numPI << endl;
	cout << "  PO   " << right << setw(9) << _numPO << endl;
	cout << "  AIG  " << right << setw(9) << _numAig << endl;
	cout << "------------------" << endl;
	cout << "  Total" << right << setw(9) << _numPI + _numPO + _numAig << endl;
}

void CirMgr::printNetlist() const
{
	cout << endl;
	uint count = 0;
	for (size_t i = 0; i < _netList.size(); ++i)
	{
		if (_gateList[_netList[i]]->getType() != UNDEF_GATE)
		{
			cout << '[' << count << "] ";
			_gateList[_netList[i]]->printGate_netList();
			++count;
		}
	}
}

void CirMgr::printPIs() const
{
	cout << "PIs of the circuit:";
	for (size_t i = 0; i < _PIList.size(); ++i)
	{
		cout << ' ' << _PIList[i];
	}
	cout << endl;
}

void CirMgr::printPOs() const
{
	cout << "POs of the circuit:";
	for (size_t i = 0; i < _numPO; ++i)
	{
		cout << ' ' << _numVar + i + 1;
	}
	cout << endl;
}