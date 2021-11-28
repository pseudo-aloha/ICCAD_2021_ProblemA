/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include "sat.h"
#include "ntkMgr.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
	friend class CirGate;

public:
	CirMgr() {}
	~CirMgr();

	// Access functions
	// return '0' if "gid" corresponds to an undefined gate.
	CirGate *getGate(unsigned gid) const;
	CirGate *getPO(unsigned po) const;

	

	// Member functions about circuit construction

	// if isMiter, the first PO is read as PO
	// all the other PO are used to map _aigMiterNode in each gate
	// else all PO are used to map _aigNode in each gate

	bool readCircuit(const string &, CirType type );	// parse circuit
	void linkConst();
	void readHeader(ifstream &);
	void readPI(ifstream &);
	void readPO(ifstream &);
	void readAig(ifstream &);
	void readNet(ifstream &);
	// since aig/aag define symbols at the end of the file
	void readSymbol(ifstream &);		
	void connect();						// turn fanin id into actual fanin
	void genNetlist();					// generate a idlist in topological order (for simulation)
	void reset();						// clear ntk

	// this function simply call every gate's simulate()
	void 	simulate();

	// this randomSim() generate random pattern at PI and simulate to
	// perform fraig multiple times. 
	// may have to write another function to 
	// calculate similarity or simply do one random simulation
	void randomSim( unsigned repeat = 1 );			

	vector<bool>	selectPatch( unsigned numPatch );

	// added by Hugo
	float cosineSimilarity(NtkGate* g1, NtkGate* g2);
	// added by Hugo
	float structuralSimilarity(NtkGate* g1, NtkGate* g2);

	size_t getMiterSim() { return (_numPO > 0) ? _gateList[_numVar+1]->_sim.back() : 0; }

	// set pi with given simulation patterns
	// and perform simulation
	void setPIAndSimulate(const vector<size_t> &);

	// create variables
	void satReset();

	// construct the lookup table of simulation, added by yen_ju
	void setGlobalSimLookupTable();

	// Member functions about circuit reporting
	void printSummary() const;
	void printNetlist() const;
	void printPIs() const;
	void printPOs() const;
	void printFloatGates() const;
	void printFECPairs() const;
	void writeAag(ostream &) const;
	void writeGate(ostream &, CirGate *) const;

	// create a queue of all PIs' fanout
	void 		bfs_init(queue<CirGate *> &); 	

	// bfs 1 step from the first gate in queue
	CirGate *	bfs_travel(queue<CirGate *> &);	

	// simulation 
	void fileSim(ifstream &);

private:

	CirType	_type;

	size_t _numVar;			// number of variable (pi and aig) in aig
	size_t _numPI;			
	size_t _numLatch;		// not used
	size_t _numPO;
	size_t _numAig;
	size_t _numNet;

	CirGateList _gateList;		// store the pointer of all gates

	IdList _PIList;			// pi's id

	// Note: aig of old and gold don't have netlist
	IdList _netList;		
	IdList _aigNetlist; 	// AigGates in netList (not in order)
	IdList _unUsedList;
	IdList	_netAigIds;

	// sim
	unsigned _numGoodBit;
	unsigned _numSim;

};

#endif // CIR_MGR_H
