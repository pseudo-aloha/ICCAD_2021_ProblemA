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
#include <queue>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

#define NEG 0x1

// CirGateV:
// fanin or fanout of a node.
// store the pointer to another node.
// LSB=1 denotes negation edge.
class CirGateV
{

public:
	CirGateV() { _gate = 0; }
	CirGateV(uint id) { _gate = id; }
	CirGateV(CirGate *g, bool phase) : _gate((size_t)g + phase) {}

	void set(CirGate *g, bool phase) { _gate = (size_t)g + phase; }
	void setGate(const CirGate *g, bool inverse = false) { _gate = (size_t)g + (phase() ^ inverse); }

	CirGate *gate() const { return (CirGate *)(_gate & (~NEG)); }

	// used when id are stored in CirGateV
	// should not be used after the the circuit are connected
	uint gateId() const { return (uint)_gate; }

	// wether it is a inverted edge
	bool phase() const { return _gate & NEG; }
	const bool isConst() const;

	bool operator==(const CirGateV &g) const { return gate() == g.gate(); }
	bool operator==(const CirGate *gptr) const { return ((CirGate *)(_gate & (~NEG))) == gptr; }

	// get simulation value
	size_t getSim( int index = -1 ) const;

private:

	// size_t to store gate and phase
	size_t _gate;

};

// CirGate:
// parent class of aig, pi, po, const
class CirGate
{
	friend class CirMgr;
	friend class CirGateV;

public:

	CirGate() {}
	CirGate(uint id) : 
	_id(id), _name(""), _var(0), _flag(0) {}
	virtual ~CirGate() {}
	void connect(vector<CirGate *> &);	// for each fanin add this gate to it's fanout
	void disconnect();
	void sweep();
	void replace(const CirGateV &);
	void optimize();
	void replace(CirGate *g);
	virtual void simulate(){};

	// sat
	virtual void addClauseRec() {};
	virtual void addClause() {};

	// building graph
	void addFanout(CirGate *fano, bool phase) { _fanout.push_back(CirGateV(fano, phase)); }
	void setSymbol(const string &s) { _name = s; }

	// blackbox
	virtual bool checkXorStruct( CirGate* &a, CirGate* &b, bool &neg ) { return false; }

	// Basic access methods
	virtual string getTypeStr() const = 0;
	virtual CirGateType getType() const = 0;
	uint getID() const { return _id; }
	uint getSignedID(bool inf = 0) const { return 2 * _id + inf; }
	string getSymbol() const { return _name; }
	virtual bool isAig() const { return false; }	
	CirGateV getFanin( unsigned i );

	// Printing functions
	void printGate_netList() const;
	void reportFECs() const;
	void reportGate() const;
	void reportFanin(int level) const;
	void reportFanout(int level) const;
	void preOrderPrint(int level, int indent, bool fanin);

	void postOrderVisit(IdList &, IdList &);



	// flag access
	uint getFlag() { return _flag; }
	// set flag to _globalFlag + i
	void setFlag(int i=0) { _flag =_globalFlag+i; }
	// increase global flag (of CirGate) by i
	static void increaseGlobalFlag( int i = 1 ) {_globalFlag+=i;}
  	static unsigned 	_globalFlag;			

	// fraig
	void 	setVar(Var var) { _var = var; }
	Var 	getVar() { return _var; }
	void 	pushPattern(SatSolver &);

protected:

	// index in gate list
  	uint 			_id;		
  	PinList 		_fanout;
  	PinList 		_fanin;

	// name
  	string 			_name;				// name

	// simulation value
  	//size_t 			_sim;
	vector<size_t>		_sim;

  	Var 			_var;

	// to mark visited
  	unsigned 			_flag;					// flag of the node

};

class AigGate : public CirGate {
public:
  AigGate() {}
  AigGate( uint id, uint fanin0, uint fanin1 ): CirGate(id) {
    _fanin.push_back( CirGateV( fanin0 ) );
    _fanin.push_back( CirGateV( fanin1 ) );
  }

  string getTypeStr() const { return "AIG"; }
  CirGateType getType() const { return AIG_GATE; }
  virtual bool isAig() const { return true; }

  void simulate();

  void	addClause();
  void	addClauseRec();

	bool checkXorStruct( CirGate* &a, CirGate* &b, bool &neg );

private:
};

class POGate : public CirGate {
public:
  POGate( uint id, uint fanin ): CirGate(id) {
    _fanin.push_back( CirGateV( fanin ) );
  }

  string getTypeStr() const { return "PO"; }
  CirGateType getType() const { return PO_GATE; }

  void simulate();
  void addClauseRec();
  void addClause();
};

class PIGate : public CirGate {
  friend class CirMgr;
public:
  PIGate( uint id ): CirGate(id) {}

  string getTypeStr() const { return "PI"; }
  CirGateType getType() const { return PI_GATE; }

private:

	void addClauseRec();
};

class ConstGate : public CirGate {
public:
  ConstGate(): CirGate(0) {}

  string getTypeStr() const { return "CONST"; }
  CirGateType getType() const { return CONST_GATE; }
  void addClauseRec();

};

class UndefGate : public CirGate {
public:
  UndefGate( uint id ): CirGate(id) {}

  string getTypeStr() const { return "UNDEF"; } 
  CirGateType getType() const { return UNDEF_GATE; }
};

#endif // CIR_GATE_H