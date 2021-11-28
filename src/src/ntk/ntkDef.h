/****************************************************************************
  FileName     [ cirDef.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic data or var for cir package ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef NTK_DEF_H
#define NTK_DEF_H

#include <vector>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "myHashMap.h"
#include "cirDef.h"


#define SUFFIX_LEN 2
#define W_NAME 20
#define MAX_COST 2000

using namespace std;

// TODO: define your own typedef or enum
enum NtkGateType {
  NTK_GATE_PI,
  NTK_GATE_CONST,
  NTK_GATE_AND,
  NTK_GATE_NAND,
  NTK_GATE_OR,
  NTK_GATE_NOR,
  NTK_GATE_XOR,
  NTK_GATE_XNOR,
  NTK_GATE_NOT,
  NTK_GATE_BUF,
  NTK_GATE_UNDEF,
  NTK_GATE_TOTAL
};

// throw error
enum EcoError
{
    ERR_RESET_REC,
    ERR_RESET_MGR
};

class NtkGate;
class NtkMgr;
class SatSolver;

typedef vector<NtkGate*> NGateList;
typedef vector<NtkGate*> NPinList;

struct CompareByLevel
{
	bool operator()( NtkGate* g1, NtkGate* g2 );
};
typedef priority_queue<NtkGate*, vector<NtkGate*>, CompareByLevel> NtkGatePQ;
struct CompareByNetId
{
	bool operator()( NtkGate* g1, NtkGate* g2 );
};
typedef set<NtkGate*, CompareByNetId> GateSetByNet;
typedef unordered_set<NtkGate*> GateSet;


struct FSCEntry
{
	FSCEntry() {}
	FSCEntry( float s, float r, NtkGate* g ): sim_simul(s), sim_struct(r), gate(g){}
	float sim_simul;
	float sim_struct;
	NtkGate* gate;
};
bool compareFSCEntry( FSCEntry a, FSCEntry b );
// discarded
// For similarity checking
struct SimEntry 
{
	SimEntry(){}
	SimEntry( float s, float r, NtkGate* o, NtkGate* g ): sim(s), sim_struct(r), old(o), gol(g){}
	float sim;
	float sim_struct;
	NtkGate* old;
	NtkGate* gol;
};
bool compareSimEntry( SimEntry a, SimEntry b );
struct CompareSimEntry
{
	bool operator() ( SimEntry a, SimEntry b );
};
typedef priority_queue<SimEntry, vector<SimEntry>, CompareSimEntry> SimEntryQ;

// For storing cuts
struct CutPair
{
	IdList          oldCutIdList;
	IdList          goldCutIdList;
	size_t          checkSize;
	float			      score;
  size_t          level; // the level of the similar pair, used when try to fix a low level cut
  size_t          originalGateId;

	size_t 			size() { return oldCutIdList.size(); }
	bool			empty() { return oldCutIdList.empty(); }

};
struct Cut
{
	NGateList		gateList;
	float			score;		// TODO: don't need this inside the structure

	size_t			size() { return gateList.size(); }
	bool			empty() { return gateList.empty(); }
};

struct patchPair
{
	NtkGate* oldGate;
	NtkGate* golGate;
};

struct patchPairStr
{
	string oldGate;
	string golGate;
	size_t   _mg_count;
};

struct ComparepatchPairStr
{
	bool operator() (patchPairStr &a, patchPairStr &b );
};
typedef priority_queue<patchPairStr, vector<patchPairStr>, ComparepatchPairStr> patchPairQ;


typedef vector<CutPair> CutPairList;
typedef vector<vector<string>> NtkGateVecArrArr;
typedef vector<string> NtkGateVecArr;
struct patchRecord
{
	NtkGateVecArrArr	patchRecFanout;
	NtkGateVecArr		patchArr;
	NtkGateVecArr		pAndPPrime;
};

bool compareCutPairLevel( CutPair &a, CutPair &b);
bool compareByCheckSize( CutPair &a, CutPair &b );


// for merge back aig table

struct InputAigEntry
{
	InputAigEntry(): gate(0), phase(0), used(0), needInvert(0) {}
	NtkGate* 	gate;	// gate that used as input
	bool		phase;
	bool		used;
	bool		needInvert;
};
typedef unordered_map<CirGate*, InputAigEntry> InputAigTable;



#endif // CIR_DEF_H