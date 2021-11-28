#ifndef NTK_GATE_H
#define NTK_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "ntkDef.h"
#include "cirGate.h"
#include "sat.h"

using namespace std;

enum CutEnumFlag 
{
	CE_EMPTY,
	CE_BELOW,
	CE_ABOVE,
	CE_DUP,
	CE_CUT,
	CE_TAKEN,
	CE_DISCARD,
};

enum BlackBoxFlag
{
	BB_NONE,
	BB_XOR,
	BB_XNOR,
	BB_INTER
};


bool compareByLevel( NtkGate* g1, NtkGate* g2 );

class NtkGate
{
    friend class NtkMgr;
	friend bool compareByLevel( NtkGate* g1, NtkGate* g2 );

public:

    NtkGate() {}
	NtkGate(uint id, string name, NtkGateType type );

    ~NtkGate() {}

    // === building ntk ===

	// take the id in fanin and change it to ptr.
	// also handle the fanout of fanins.
	void		connectFanin();
    void 		addFanout(NtkGate *g);
	void		addFaninByName( string &name );
	void		adjustLevel();

    // === Basic access methods ===

    string 		getTypeStr() const;
    NtkGateType	getType() const { return _type; }
    uint 		getId() const { return _id; }
    uint 		getNetId() const { return _netId; }
    string 		getName() const { return _name; }
    string 		getNameShort() const { return _name.substr(0, _name.size()-SUFFIX_LEN); }
	size_t 		getSim( int index ) const { return _aigMiterNode.getSim( index ); }
	unsigned 	getLevel() const { return _level; }
	bool		isOld() const { return _old; }
	bool 		isMergedWith( NtkGate* g ) const;

	NtkGate*    getMgList() const { return _mg_list; }
	NtkGate*    getMgNext() const { return _mg_next; }
	NPinList    getFanoutList() const { return _fanout;}
	NPinList    getFaninList() const {return _fanin;}
	NtkGate*	getRecPairFanin( NtkGate* fanin );

	/*========== printing ==========*/
	
	void		reportGateSimple() const;
	void		reportGate() const;
	void		postOrderPrint( int level, int indent );
	void		reportFanoutRecPair(bool detail);
	void		printUntilMerge( int indent );

	/*========== netlist ==========*/
	
    void 		postOrderVisit( IdList & netList  );
    void 		postOrderVisit( int level = 0 );		// used before enumerate
	void		postOrderMark( );
	void		postOrderVisitToCut();

	/*========== matching  ==========*/

	void		setRecPair( NtkGate* g ) { _rPair = g; }

	void		markDupMerge( int level, NGateList &dupMerge, GateSetByNet &activeGates, int &numNew, bool newGate );
	void		markGolBottomCut( int level, GateSetByNet &activeGates, int &numNew, bool newGate );

	bool		isValidCut();

	void		addRecPair( NtkGate* g, NtkGatePQ &found, NGateList &recPairList );
	bool		checkRecPair( NtkGate* g );
	void		addRecPairNRecord( NtkGate* g, NtkGatePQ &found , patchRecord &record);

	// cut function fraig
	void 		outputCutFunc( string f_name, unsigned numPi, unsigned &gateCnt );
	void		outputCutFuncGate( ofstream &ouf, bool output, unsigned &gateCnt );
	void		setCutFuncPi( unsigned i );
	void		preOrderVisitCutFunc();
	void		postOrderVisitCutFunc( unsigned &unmergedCnt );
	void		postOrderVisitCutFunc( NGateList &bwdCut, NGateList &bwdCutMapping );

	// new
	bool		legalize_checkFanin( bool begin, queue<NtkGate*> &checkFanin, GateSetByNet &activeGates, int &numNew, NGateList &removed );
	bool		legalize_popUp( queue<NtkGate*> &checkFanin, GateSetByNet &activeGates, int &numNew );
	bool		legalize_checkFanout( bool begin, queue<NtkGate*> &checkFanin, GateSetByNet &activeGates, int &numNew );
	void		legalize_popDown( queue<NtkGate*> &checkFanout, GateSetByNet &activeGates, int &numNew );
	void		finalizeFSC( bool discardOldCuts );
	void		clearFSC( int level );
	void		postOrderInit( int level );
	


	/*========== replacing  ==========*/

	void	selAddGate( ofstream &ouf, unsigned &gateCnt );
	void	selAddGateCopy( ofstream &ouf, unsigned &gateCnt, bool output );

	// get the net id for specific rec gate
	string	selGetNet( NtkGate* rec = 0 );
	string	selGetNetCopy( NtkGate* rec = 0 );
	string	selGetNetForBwdPatch( NtkGate* gol );

	void		setMuxId( unsigned id ) { _muxId = id; }
	void		GetInputPatch( NGateList &input, NGateList &wire, bool output);
	int			getPartialCost();
	string		getPatchFaninName( unsigned c = 0);
	string		getPatchFaninName2( NtkGate* from, bool input = false );
	string		getPatchFaninName(  string wireName, unsigned c = 0);
	void		decidePatchType();
	void		checkPatchFanin();
	void		outputGateInPatch( ofstream &ouf, unsigned &gateCnt);
	NtkGate*	getMappedFanin();
	string		getMappedFaninName( bool inv = false );
	
	// return true if there's any recpair left
	bool		selectRecPair( vector<bool> &res, unsigned &index );

	void		estimatePatchCost( unsigned &num, bool bwdPatch = false );

	/*========== sat solver ==========*/

	// recursively add fanin clause
	// until a flagged node is found
	void		satAddClauseRec();
	virtual void	addClauseRec();
	virtual void	addClauseRec( int level );
	virtual void	addClause() {}


	// flag an aig node (relative to gloal flag!!)
	void		setAigFlag(int i) { _aigNode.gate() -> setFlag(i); }

	// to be replaced
	Var			getVar() { return _aigNode.gate()->getVar(); }
	bool		getPhase() { return _aigNode.phase(); }


	// sat
	Var		getSatVar() { return _satVar; }
	bool	getSatPhase() { return _satPhase; }

	/*========== blackbox ==========*/

	void	checkXorStruct();
	void 	findXorInput( const CirGate *a, const CirGate *b, NGateList &a_ntk, NGateList &b_ntk, NGateList &inter );


	/*========== merged gates ==========*/
	
	void 			merge( NtkGate* g, bool global );
    NtkGate*		mg_getNext() const;
    unsigned		mg_len() const;

	// use current gate as head and create a new merged list
	void			mg_create( bool global );

	// add g to current list 
	// (same circuit)
	// added gate should have the same _mg_lsit
	void			mg_append( NtkGate* g, bool global );

	// add g to this gate's merged list
	// (different circuit)
	// also handle g's list
	void			mg_appendMerged( NtkGate* g, bool global );

	void			mg_update( NtkGate* g, bool global );

	/*========== flag  ==========*/
	
    uint getFlag() { return _flag; }

	// set flag to _globalFlag + i
    void setFlag( const int i ) { _flag = _globalFlag + i; }

	static void		increaseGlobalFlag( const int i=1 ) { _globalFlag += i; }
    static unsigned 	_globalFlag;		// for visiting gates
	static unsigned 	_globalFlagB;
	static unsigned     _globalFrontierSize;

	/*========== patch ===========*/
	bool IsPatchOutput();
	bool IsAllPatched();
	unsigned IsPatchMoreThanHalf();
	bool IsFaninChanged();

	bool checkLoopIfPatched( NtkGate *g_old );
	void checkWontReachWithPatch();

protected:

	// basic data
    uint            _id;	
	uint			_netId;
    string          _name;
    NtkGateType     _type;
	int 			_level;
    NPinList        _fanin;		
    NPinList        _fanout;

	vector<NtkGate*>	_recPair_fanin;
	vector<NtkGate*>	_recPair_fanout;

	unsigned 			_recCnt;

	NtkGate*		_mapping;

	// head of merged gates in another circuit; (old存gold, gold存old)
	NtkGate*		_mg_list;		// if not merged, points to "null"
    NtkGate*     	_mg_head;		// head of this list
    NtkGate*		_mg_next;		// next node in this list

	// merge list for cut function
	NtkGate* 		_cmg_list;
	NtkGate* 		_cmg_head;
	NtkGate*		_cmg_next;

	// rectification pair
	// only used in old gates to store the corresponding
	// rectification gate in gold
	NtkGate*		_rPair;		


	// aig nodes
	CirGateV		_aigMiterNode;  // simulate here
	CirGateV		_aigNode;       

	// similarity
	vector<bool>    _structuralVec;
	//vector<FSCEntry>	_fsc;
	vector<int>		_cuts;

	// for building selector.
	// start from this id
	// the ith rec pair in _recPair_fanout 
	// will use id+2*i as their mux output
	union
	{
		unsigned 		_cutFuncPiId;	// used in output cut func
		unsigned		_muxId;			// used in build selector
	};


	// flag
    uint 			_flag;		
	// added by Hugo
	uint			_flagB;			// used for floating mark in patch phase



	vector<bool>    structuralSim;
	uint            mergeFrontierNum;

	union 
	{
		//for cut finding purpose....
		struct{
			bool				_ce_new :8;
			bool				_ce_covered :8;
			bool				_ce_newOnCut :8;
			CutEnumFlag			_ce_flag: 8;
		};
		unsigned			_cut_flag;
		unsigned			_numBwdPatch;

	};

	NtkGate*		_allPatchTo;


	// for test
	// don't store in gate, use an additional table
	float			_maxScore;	

	// blackbox
	unsigned		_blackBoxFlag;
	NGateList		_xorInputs;
	NGateList		_xorInter;
	bool			_blackBoxInternal;


	// bool
	bool			_mergeFlag;

	// if the gate belongs to old circuit
    bool 			_old; 			// old or golden 
	bool			_pseudoMerge;
	bool            _aboveMergeFrontier;
	bool            _isMergeFrontier;

	// sat
	Var				_satVar;
	bool			_satPhase;



};


class NtkGateBuf : public NtkGate
{
public:
	NtkGateBuf( uint id, string name): NtkGate(id, name, NTK_GATE_BUF) {}
	void addClause();
};
class NtkGateNot : public NtkGate
{
public:
	NtkGateNot( uint id, string name): NtkGate(id, name, NTK_GATE_NOT) {}
	void addClause();
};
class NtkGateAnd : public NtkGate
{
public:
	NtkGateAnd( uint id, string name): NtkGate(id, name, NTK_GATE_AND) {}
	void addClause();
};
class NtkGateNand : public NtkGate
{
public:
	NtkGateNand( uint id, string name): NtkGate(id, name, NTK_GATE_NAND) {}
	void addClause();
};
class NtkGateOr : public NtkGate
{
public:
	NtkGateOr( uint id, string name): NtkGate(id, name, NTK_GATE_OR) {}
	void addClause();
};
class NtkGateNor : public NtkGate
{
public:
	NtkGateNor( uint id, string name): NtkGate(id, name, NTK_GATE_NOR) {}
	void addClause();
};
class NtkGateXor : public NtkGate
{
public:
	NtkGateXor( uint id, string name): NtkGate(id, name, NTK_GATE_XOR) {}
	void addClause();
};
class NtkGateXnor : public NtkGate
{
public:
	NtkGateXnor( uint id, string name): NtkGate(id, name, NTK_GATE_XNOR) {}
	void addClause();
};
class NtkGatePi : public NtkGate
{
public:
	NtkGatePi( uint id, string name): NtkGate(id, name, NTK_GATE_PI) {}
};
class NtkGateConst : public NtkGate
{
public:
	NtkGateConst( uint id, string name): NtkGate(id, name, NTK_GATE_CONST) {}
	// TODO: use addClauseRec to add golden constraint
	void	addClauseRec();
	void	addClauseRec( int level );
};



#endif // CIR_GATE_H