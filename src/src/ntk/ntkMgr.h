/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef NTK_MGR_H
#define NTK_MGR_H

#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include "sat.h"
#include "cirGate.h"

using namespace std;

#include "ntkDef.h"

extern NtkMgr *ntkMgr;

class NtkMgr
{

	friend class NtkGate;

public:
	NtkMgr();
	~NtkMgr();

	/*========== whole process ===========*/

	void		attachFiles( char *fname_old, char *fname_gol, char *fname_patch );
	void		resetRecPair();
	void 		reset();
	bool 		eco	( int &bestCost, int sim_num, float sim_thre, int lv_thre );
	bool 		ecoPo	( int &bestCost, int sim_num, float sim_thre, int lv_thre );

	/*========== main functions ===========*/

	// read rev and gol
	// also read miter aig / single aig 
	// perform fraig to find merge gates
	void		setupRevAndGol			( char *fname_rev, char *fname_gol );	

	// build selector from recpair
	// output, fraig, read aig back
	void		buildAndLoadSelector	( NtkGate *g_rev = 0, NtkGate *g_gol = 0 );

	// for prepatch pair, remove merge
	// and add recPairFanin in golden
	void		handlePrePatchPair( NtkGate *g_rev, NtkGate *g_gol );
	void		restorePrePatchPair();


	// output patch
	void		markFloatingGates( NtkGate *p_rev = 0, NtkGate *p_gol = 0 );
	void        outputPatchFile(string fname, NtkGate *p_rev = 0, NtkGate *p_gol = 0 );

	// apply patch
	void		applyPatch		( string fname_old, string fname_patch, string fname_out );

	// check patch
	bool		checkPatched	( char *fname_patch, char *fname_patched, char *fname_gol );


	/*========== access  ==========*/
	// return '0' if "gid" corresponds to an undefined gate.
	NtkGate *	getGate( unsigned gid ) const;
	NtkGate * 	getPI( unsigned i, bool old ) const {
		if ( i >= _PIList_old.size() ) return 0;
		return  old ? getGate(_PIList_old[i] ) : getGate(_PIList_gol[i]);
	}

	/*========== ntk constructoin  ==========*/
	
	bool 		constructCircuit();
	void		createConst();
	void 		genNetlist();
	void		addPO( unsigned id, bool old );
	void		markMergeCone();

	//yen_ju hugo aloha added
	vector<CutPair*> cutCandidate;
	//end

	/*========== look up gate  ==========*/
	
	// create an id for the gate with specific name
	unsigned		findOrCreateIdByName( string &name );

	NtkGate*		findGateByName( string name );
	NtkGate*		createGate( unsigned id, string &name, NtkGateType type );

	/*========== aig mapping  ==========*/

	// assume that the gates in golden are mapped after
	// the gates in old
	void	mapGate2Aig( string name, CirGate* aigNode, bool complement, CirType cirType );

	/*========== sat solver  ==========*/

	// delete model
	// reset variables
	void	satReset();

	// add a miter and return the var of the xor
	Var		satAddMiter( NtkGate* g1, NtkGate* g2 );

	// add a miter and return the var of the xor
	Var		satAddMiterNtk( NtkGate* g1, NtkGate* g2 );

	// just a test
	void	satTest();

	/*========== printing ==========*/

	void 		printSummary() const;
	void		printNetlist() const;
	void 		printMgList() const;

	/*========== matching ==========*/

	bool 		identifyRecPairs	( NtkGate* g_old_pre = 0, NtkGate *g_gol_pre = 0 );
	void 		identifyRecPairs	(patchRecord &record);
	void		identifyRecPairsOf2Gates(NtkGate* g_old, NtkGate* g_gol, patchRecord &record);
	CutPair		cutMatching			( NtkGate* g_old, NtkGate* g_gol );
	void		findPossibleCuts( IdList &oldCands, IdList &golCands );

	/*========== recycle ==========*/
	void		recycle				();

	
	// compare and sort the similarity between two idlist 
	void		getSimPairs( IdList &oldList, IdList &golList, float threshold, vector<SimEntry> &res );
	void		checkCutFuncTest();

	// H-Lee
	void    	cutTest();

	// similarity
	void		computeSimilarity( NtkGate* g_old, NtkGate* g_gol, float &sim, float &sim_struct );

	// new
	void		getGuildedCutCands2( vector<Cut>& , vector<Cut>&, NtkGate* , NtkGate* , unsigned, NGateList &dupMerge, bool &dupMergeChange, vector<SimEntry> &, int minCheckSize );
	bool		checkCutFunc2( NtkGate* g_old, NtkGate* g_gol, NGateList &dupMerge, vector<SimEntry> &simPairList, vector<int> &numGateLeft, int gol_oldCutMaxId, bool revIsNewCut, int level, Cut &cut, CutPair &resCut, vector<Cut> &golCutList, vector<int> &golCutIdList );
	bool		legalizeCut( queue<NtkGate*> &checkFanin, GateSetByNet &activeGates, int &numNew, NGateList &removed );
	void		recoverCut( queue<NtkGate*> &checkFanout, GateSetByNet &activeGates, int &numNew, NGateList &removed );
	void		makeDecisionOnCut( Cut &taken, GateSetByNet &activeGates, queue<NtkGate*> &checkFanin, queue<NtkGate*> &checkFanout, int &numNew, vector<Cut> &candList, int minCheckSize );
	void		enumerateCuts( vector<Cut> &cutList, NGateList &dupMerge, GateSetByNet &activeGates, int numNew, int minCheckSize );
	bool		cutEqual( NGateList &c1, NGateList &c2 );
	void		updateSimPairList( vector<SimEntry> &simPairList, vector<Cut> &, vector<Cut> &, NGateList &, NGateList &, bool );
	void		checkNewGatesInCuts( vector<Cut> &, NGateList &, NGateList &, NGateList&, NGateList& );
	void		addPairwiseSimEntry( vector<SimEntry> &simPairList, NGateList &, NGateList & );
	bool		addPairToFormCut( vector<SimEntry>& simVec, unsigned index, int numGateLeftInCut, const int numGateInCut, vector<int> &numGateLeft, NGateList &dupMerge, NGateList &revCut, Var miter, int oldCutMaxId, bool revIsNewCut, vector<Cut> &golCutList, vector<int> &golCutIdList, int numConstMatch );
	bool		checkCutFuncEquiv( NGateList &revCut, Var miter );
	bool		checkCutFuncEquivWithOneGateLeft( NGateList &revCut, Var miter, NGateList &candList );
	void		reportMatchCut( NGateList &cut );

	// CF fraig
	CutPair		cutMatchingWithFraig( NtkGate* g_old, NtkGate* g_gol);
	bool		fraigCutFunc2( NtkGate* g_old, NtkGate* g_gol, NGateList &dupMerge, vector<SimEntry> &simPairList, vector<int> &numGateLeft, int gol_oldCutMaxId, bool revIsNewCut, int level, Cut &cut, CutPair &resCut, vector<Cut> &golCutList, vector<int> &golCutIdList );
	unsigned 	fraigCutFunc( NtkGate* g_old, NtkGate* g_gol, CutPair &cut );
	bool		fraigCutFunc( NGateList &revCut );
	bool		fraigCutFuncWithOneGateLeft( NGateList &revCut, NGateList &candList );
	void		fraigCF();
	bool		addPairToFormCutAndFraig( vector<SimEntry>& simVec, unsigned index, int numGateLeftInCut, const int numGateInCut, vector<int> &numGateLeft, NGateList &dupMerge, NGateList &revCut, int oldCutMaxId, bool revIsNewCut, vector<Cut> &golCutList, vector<int> &golCutIdList, int numConstMatch );

	/*========= replacing ===========*/

	void		buildSelector	( string fname, bool manual=false, NtkGate* g_rev = 0, NtkGate* g_gol = 0 );
	// get a new id for net in selector
	unsigned	newNet			() { return _netCnt++; }
	void		selectPatch		();
	void		estimatePatchCost ();
	unsigned 	getBwdPatchMux( NtkGate* gr, NtkGate* gg );



	/*========= black box ===========*/
	void checkXorStruct();

	/*========= other ===========*/

	size_t		getNumPO() {return _numPO; }
	void		simTest();
	void		test();
	void		clearCFMergeInfo();
	void		resetPatchRecord(patchRecord &record);

	/*========= write output patch ===========*/
	void        GetInputPatch(NtkGate*, NGateList&, NGateList&);
	//added by yen_ju for output the whole patched circuit
	void        OuputWholePatchFile(string fname);

	// added by Hugo
	void        MarkFaninConeTraverse(NtkGate*, int, size_t&);
	void		CountRepeatedMgGate(NtkGate*, int, int&, size_t&);

	/*=========get merge frontier============*/
	void 		getMergeFrontier();
	void 		setStructuralVector();
	void		visitFaninCone(NtkGate*, NtkGate*);
	float		getStructuralSimilarity(NtkGate*, NtkGate*);

	/*=========global simulation=========*/
	void 		findSimilarPairs(char* fname_old, char* fname_gol );
	static size_t _mg_count;

	/*=========prepatch==================*/
	void		addPreRecPair( patchRecord &record );
	void		checkBwdPatches();
	int			existInPatchArr( patchRecord &record , NtkGate* g);
	void		prePatch( char *fname_old, char *fname_gol, int sim_num, float sim_thre, int lv_thre , patchRecord &record );
	bool		firstPrepatchEco( int sim_num, float sim_thre, int lv_thre, size_t i, patchRecord &record );
	bool		SecondPrepatcheco( int &bestCost, int sim_num, float sim_thre, int lv_thre , size_t i, patchRecord &record);

	/*========try different parameters============*/
	void		tryDifferentParas( int &bestCost , patchRecord &record);

private:

	// private function
	void 		bfs_init(queue<NtkGate *> &);
	NtkGate *	bfs_travel(queue<NtkGate *> &);

	void 		simulate();		// call aig?

	size_t 		_numVar;
	size_t 		_numPI;
	size_t 		_numLatch;
	size_t 		_numPO;
	size_t 		_numOld;
	size_t 		_numGol;

	NGateList 	_gateList;

	IdList 	_PIList_old;
	IdList 	_PIList_gol;
	IdList	_POList_old;
	IdList	_POList_gol;
	IdList 	_netList_old;
	IdList 	_netList_gol;
	IdList  _merge_frontier;

	NGateList	_dummyPOList;

	// for matching
	NGateList	_recPairList;	// also hold backward patch gate in selector
	NGateList	_bwdPatchList;

	// for selector 
	unsigned 	_netCnt;
	unsigned	_numMux;

	//for cost
	unsigned _cost; 


	// a table that find gate by name
	unordered_map<string, unsigned>		_name2Id;

	// a table that find mg_heads by something (aig id or ptr in abc)
	// store the head of the list in old circuit
	unordered_map<size_t, unsigned>		_mg_heads;
	unordered_map<size_t, unsigned>		_cmg_heads;

	// for the mapping of bwdpatch to mux id
	unordered_map< NtkGate*, vector<NtkGate*> > _bwdPatchMap;

	int			_sim_count;		// sim to sim num before starting
	bool		_read;

	// for cut matching
	// these will be setup before matching

	int			_sim_num;
	float 		_sim_thre;
	int			_lv_thre;		// lv thre

	// initial min check size (not change currently)
	// TODO
	int			_minCS_init;	


	// dont reset these
	char 		*_fname_old;
	char		*_fname_gol;
	char		*_fname_patch;
	char		_fname_patch_tmp[128];
	char		_fname_patched[128];
	char		_fname_p[128] ;
	char		_fname_ped[128];

	// for opt
	bool		_enumerate_gol;

	// for patch
	NGateList 					_netList_patched;

	unordered_set<NtkGate*> 	_patchInputSet;
	NGateList 					_patchOutputList;
	NGateList					_patchWireList;
	bool						_useConst;
	InputAigTable				_in2Aig;

	// for fraig CF
	NGateList 	_CFGates_rev;
	NGateList 	_CFGates_gol;
	NtkGate*	_g_rev;
	NtkGate*  	_g_gol;
	NGateList 	_dupMerge;

	int 		_unmergedCnt;
	NGateList	_bwdCut;
	NGateList	_bwdCutMapping;
	NGateList	_matchCut;
	NGateList	_matchCutMapping;

	int 		_level;
	int			_level_res;

	// for prepatch
	vector<patchPairStr> 	_patchPairStrVec;
	vector<NtkGate*>		_mg_list_recover;
	NtkGate*				_p_rev;
	NtkGate*				_p_gol;
	NtkGate*				_p_in;
	bool					_p_eq;

};

#endif // CIR_MGR_H
