#include <algorithm>
#include <iomanip>


#include "ntkDef.h"
#include "ntkMgr.h"
#include "ntkGate.h"
#include "cirMgr.h"
#include "sat.h"
#include "util.h"

using namespace std;

unsigned 		relatedCutGatesCnt = 0;

extern SatSolver* solver;
extern CirMgr*	aigOld;
extern CirMgr*	aigGol;
extern CirMgr* 	aigMiter;

extern bool debug_c;
extern bool debug_f;
extern bool debug_e;
extern bool debug_r;

extern int time_threshold;

extern "C"
{
	typedef struct Abc_Frame_t_ Abc_Frame_t;
	int    Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand );
}
extern Abc_Frame_t * pAbc;
static char Command[1024];


Timer timer_cutSearching("Cut Searching");
Timer timer_similarity("Similarity");
Timer timer_CF_proving("CF Proving");
Timer timer_CF_fraig("CF Fraig");
Timer timer_identify("Identify");
Timer timer_findsimilar("FindSimilar");


// for test
queue<NtkGate*> tmp_fakeRecPairsOld;
queue<NtkGate*> tmp_fakeRecPairsGol;

bool compareSimEntry( SimEntry a, SimEntry b ) { return a.sim > b.sim; }
bool CompareByLevel::operator()( NtkGate* g1, NtkGate* g2 )
{
	return g1->getLevel() < g2->getLevel();
}
bool CompareByNetId::operator()( NtkGate* g1, NtkGate* g2 )
{
	return g1->getNetId() < g2->getNetId();
}
bool CompareSimEntry::operator() ( SimEntry a, SimEntry b )
{
	return a.sim > b.sim;
}
bool
NtkMgr::identifyRecPairs( NtkGate* g_old_pre, NtkGate* g_gol_pre )
{
	bool time_exceed = false;
	sprintf( Command, "eco_setFlag -sf" ); Cmd_CommandExecute( pAbc, Command );

	NtkGatePQ rPairs_found;
	NtkGatePQ rPairs_notFound;

	NtkGate* g_old;
	NtkGate* g_gol;

	_recPairList.clear();

	if ( g_old_pre == 0 || g_gol_pre == 0 )
	{

		NtkGate::increaseGlobalFlag();
		
		for( size_t i = 0; i < _numPO; i++ )
		{
			g_old = getGate(_POList_old[i]);
			g_gol = getGate(_POList_gol[i]);

			// Warning: make sure they are in the same order
			// TODO: check the whole mg_list
			/*
			if ( ! g_old -> isMergedWith( g_gol ) )
			{
				// TODO: some PO use other PO as fanin
				//if ( g_old->getType() == NTK_GATE_BUF ) g_old = g_old -> _fanin[0];
				//if ( g_gol->getType() == NTK_GATE_BUF ) g_gol = g_gol -> _fanin[0];


				if ( g_gol -> _mg_list ) continue;
				rPairs_found.push( g_old );

			} else {

				// debug
				if ( debug_c ) cout << "Merged output: " << g_old->getNameShort() << endl;

			}
			*/

			_dummyPOList[i] -> _flag = NtkGate::_globalFlag;
			g_old -> addRecPair( g_gol, rPairs_found, _recPairList );

		}
	}
	else
	{
			g_old = g_old_pre;
			g_gol = g_gol_pre;

			if ( ! g_old -> isMergedWith( g_gol ) )
			{
				// TODO: some PO use other PO as fanin
				_recPairList.push_back( g_old );
				if ( !g_gol -> _mg_list ) rPairs_found.push( g_old );

			} else {
				// debug
				cout << "identify 2 gates: already merge!!" << endl;
			}
	}
	
	CutPair cutPairs;
	timer_identify.start();
	while( !rPairs_found.empty() )
	{
		g_old = rPairs_found.top();
		rPairs_found.pop();
		
		for( size_t k = 0, n = g_old -> _recPair_fanout.size(); k<n; k++ )
		{
			if ( g_old -> _mergeFlag ) continue;
			g_gol = g_old -> _recPair_fanout[k];
			if ( g_gol -> _mg_list ) continue;

			cutPairs = cutMatching(g_old, g_gol);
			bool valid = true;

			if ( !cutPairs.empty() )
			{
				IdList &oldList = cutPairs.oldCutIdList;
				IdList &golList = cutPairs.goldCutIdList;

				// mark cut function for rec net identification
				NtkGate::increaseGlobalFlag(2);
				for( size_t i = 0; i < oldList.size(); i++ )
				{
					getGate(oldList[i]) -> setFlag(-1);
				}
				g_old -> postOrderVisitToCut();
				
				// Optimize
				// To check overlap and only add when no different overlap gate
				// use this part of code
				for( size_t i = 0; i < oldList.size(); i++ )
				{
					if ( !getGate(oldList[i]) -> checkRecPair( getGate(golList[i]) ) ) valid = false;
				}
				// valid = true;
				if ( valid )

					for( size_t i = 0; i < oldList.size(); i ++ )
					{
						if ( !debug_r || getGate(oldList[i]) -> checkLoopIfPatched( getGate(golList[i])) )
							getGate(oldList[i]) -> addRecPair( getGate(golList[i]), rPairs_found, _recPairList );
					}

			}

			if ( debug_r && ( cutPairs.empty() || (!valid) ) )
			{
				cutPairs = cutMatchingWithFraig( g_old, g_gol );
				if ( !cutPairs.empty() )
				{
					IdList &oldList = cutPairs.oldCutIdList;
					IdList &golList = cutPairs.goldCutIdList;

					// mark cut function for rec net identification
					NtkGate::increaseGlobalFlag(2);
					for( size_t i = 0; i < oldList.size(); i++ )
					{
						getGate(oldList[i]) -> setFlag(-1);
					}
					g_old -> postOrderVisitToCut();
					
					for( size_t i = 0; i < oldList.size(); i ++ )
					{
						if ( getGate(oldList[i]) -> checkLoopIfPatched( getGate(golList[i])) )
							getGate(oldList[i]) -> addRecPair( getGate(golList[i]), rPairs_found, _recPairList );
					}

				}
			}

		}
		if( timer_identify.checkCurrentTime() > time_threshold )
		{
			time_exceed = true;
			break;
		}
	} 

	// cout << "# related gates on cut: " << relatedCutGatesCnt << endl;
	checkBwdPatches();

	// sort by levels
	sort( _recPairList.begin(), _recPairList.end(), compareByLevel );


	if ( debug_c )
	{
		cout  << endl << "rec pairs found: " << endl;
		for( size_t i = 0; i < _recPairList.size(); i++ )
		{
			_recPairList[i] -> reportFanoutRecPair(true);
		}
		cout << endl;

	}
	return time_exceed;
}
bool
NtkGate::checkRecPair( NtkGate* g )
{
	// if they are merged, continue
	if ( isMergedWith( g ) )
	{
		cout << "rec pair " << _name << " is already merged with " << g->getName() << endl;
		return true;
	}

	NtkGate *g2;

	for ( size_t i = 0; i < _fanout.size(); i++ )
	{
		if ( _fanout[i] -> _flag != _globalFlag || _fanout[i] -> _mergeFlag ) continue;

		g2 = _fanout[i];

		if ( g2 -> _recPair_fanin.empty() ) continue;

		for( size_t j = 0; j < g2->_fanin.size(); j++ )
		{
			// TODO: what if there are the same 2 fanins
			if ( g2->_fanin[j] == this )
			{
				// keep the original rec
				if ( g2->_recPair_fanin[j] != 0 && g2->_recPair_fanin[j] != g )
				{
					// cout << "invalid rec pair" << endl;
					return false;
				}
				break;
			}
		}
	}

	return true;
}



void
NtkGate::postOrderVisitToCut()
{
	if ( _flag >= _globalFlag-1 ) return;
	_flag = _globalFlag;
	for ( size_t i = 0; i < _fanin.size(); i++ )
	{
		_fanin[i] -> postOrderVisitToCut();
	}
}

bool compareByLevel( NtkGate* g1, NtkGate* g2 ) {return (g1->_level > g2->_level);}
bool compareCheckSize( CutPair &a, CutPair &b ) {
	return a.checkSize==b.checkSize ? a.score>b.score : a.checkSize<b.checkSize;
	}
bool compareCut( Cut &a, Cut &b ) {
	// TODO
	return a.size() == b.size() ? a.score>b.score : a.size()<b.size();
}

void
NtkGate::addRecPair( NtkGate* g, NtkGatePQ &found, NGateList &recPairList )
{
	// if they are merged, continue
	if ( isMergedWith( g ) )
	{
		cout << "rec pair " << _name << " is merged with " << g->getName() << endl;
		return;
	}


	NtkGate *g2;
	unsigned recCnt_prev = _recCnt;

	for ( size_t i = 0; i < _fanout.size(); i++ )
	{
		if ( _fanout[i] -> _flag != _globalFlag || _fanout[i] -> _mergeFlag ) continue;

		g2 = _fanout[i];

		if ( g2 -> _recPair_fanin.empty() ) g2 -> _recPair_fanin.resize( g2->_fanin.size(), 0 );

		for( size_t j = 0; j < g2->_fanin.size(); j++ )
		{
			// TODO: what if there are the same 2 fanins
			if ( g2->_fanin[j] == this )
			{
				// keep the original rec
				if ( g2->_recPair_fanin[j] == 0 )
				{
					g2 -> _recPair_fanin[j] = g;
					_recCnt ++;
				}
				break;
			}
		}
	}

	// no new rec pair is needed
	if ( recCnt_prev == _recCnt && _fanout.size() != 0 ) return;

	// check if the rec pair already exist
	for ( size_t i = 0; i < _recPair_fanout.size(); i++  )
	{
		if ( _recPair_fanout[i] == g ) return;
	}
	if ( _recPair_fanout.empty() ) 
	{
		found.push( this );
		recPairList.push_back(this);
	}
	_recPair_fanout.push_back(g);

}

vector<Cut> *cutListToSort;
bool compareCutIndexWithScore( int a, int b )
{
	return (*cutListToSort)[a].size() == (*cutListToSort)[b].size() ? (*cutListToSort)[a].score > (*cutListToSort)[b].score : (*cutListToSort)[a].size() < (*cutListToSort)[b].size();
}
bool compareCutIndex( int a, int b )
{
	return (*cutListToSort)[a].size() < (*cutListToSort)[b].size();
}
CutPair
NtkMgr::cutMatching( NtkGate* g_old, NtkGate* g_gol ) 
{
	unsigned level = 1;

	// debug
	if ( debug_c ) 
	{
		cout << "\n==============================\n\n";
		cout << "Matching: " << g_old -> _name << " " << g_gol -> _name << endl;
		cout << endl;
	}


	CutPair resCut;
	NGateList dupMerge;
	bool dupMergeChange;
	unsigned fraigUnmerged = 0;
	resCut.checkSize = INT_MAX;

	vector<Cut> revCutList, golCutList, revCutList_new, golCutList_new;
	vector<CutPair> cutList;
	vector<SimEntry> simPairList;
	NGateList revGates, golGates;
	vector<int> revCutIndex, golCutIndex;

	int	minCheckSize = 4;		
	int level_last = 0;

	golGates.push_back( getGate(_netList_gol[0]) );
	golGates.push_back( getGate(_netList_gol[1]) );

	// add g_gol as candidate (for const match)
	/*
	golGates.push_back( g_gol );
	Cut c;
	c.gateList.push_back( g_gol );
	golCutList.push_back( c );
	*/


	if ( debug_c )
	{
		cout << endl;
		// debug
		NtkGate::increaseGlobalFlag();
		g_old -> postOrderPrint( g_old -> getLevel() - _lv_thre, 0 );
		//g_gol -> postOrderPrint( g_gol -> getLevel() - LV_THRE, 0 );
		g_gol -> printUntilMerge( 0 );
		cout << endl;
	}

	// incremental add level
	while ( level <= _lv_thre ) 
	{
		// debug
		if ( debug_c ) cout << "level = " << level << endl;

		timer_cutSearching.start();
		getGuildedCutCands2( revCutList_new, golCutList_new, g_old, g_gol, level, dupMerge, dupMergeChange, simPairList, minCheckSize );
		timer_cutSearching.pause();

		getGate(_netList_gol[0]) -> _mapping = 0;
		getGate(_netList_gol[1]) -> _mapping = 0;

		// update similarity entry vector
		timer_similarity.start();
		updateSimPairList( simPairList, revCutList_new, golCutList_new, revGates, golGates, dupMergeChange );
		timer_similarity.pause();

		if ( dupMergeChange )
		{
			// remove previous cuts
			revCutList.clear();
			golCutList.clear();

			if ( debug_c )
			{
				cout << "New dup merge: "  << endl;
				for( size_t i = 0; i < dupMerge.size(); i++ )
					cout << setw(W_NAME) << dupMerge[i] -> getName();
				cout << endl;
				for( size_t i = 0; i < dupMerge.size(); i++ )
					cout << setw(W_NAME) << dupMerge[i] -> _mapping -> getName();
				cout << endl;
			}
		}

		vector<int> numGateLeft;
		int rev_oldCutMaxId = (int)revCutList.size() - 1;	// if index > this, it's a new cut
		int gol_oldCutMaxId = (int)golCutList.size() - 1;

		revCutList.insert( revCutList.end(), revCutList_new.begin(), revCutList_new.end() );
		golCutList.insert( golCutList.end(), golCutList_new.begin(), golCutList_new.end() );


		// TODO: trivial dup merge cut proving
		// since it didn't merge in fraig, some error might be in this CF
		if ( !dupMerge.empty() && revCutList.empty() && golCutList.empty() )
		{
			if ( debug_c ) cout << "Warning: trivial dup merge found." << endl;

			// break while loop since trival cut is found
			break;
		}
		else if ( !dupMerge.empty() && golCutList.empty() )
		{
			Cut empty;
			empty.score = 0;
			golCutList.push_back(empty);
			if ( debug_c ) cout << "insert empty cut in gol." << endl;
		}

		if ( debug_c ) cout << revCutList.size() << " and " << golCutList.size() << " cuts found" << endl;

		// compute rev cut score
		revCutIndex.clear();
		for( size_t i = 0; i < revCutList.size(); i++ )
		{
			revCutIndex.push_back(i);
			revCutList[i].score = 0;
			NGateList &l = revCutList[i].gateList;
			for( size_t j = 0; j < l.size(); j++ )
			{
				revCutList[i].score += l[j] -> _maxScore;
			}
		}
		cutListToSort = &revCutList;
		sort( revCutIndex.begin(), revCutIndex.end(), compareCutIndexWithScore );

		// sort gol cut by checksize
		golCutIndex.clear();
		for( size_t i = 0; i < golCutList.size(); i++ ) 
		{
			numGateLeft.push_back( golCutList[i].size() );
			//cout << "gold cut with size " << golCutList[i].size() << endl; 
			golCutIndex.push_back(i);
		}
		cutListToSort = &golCutList;
		sort( golCutIndex.begin(), golCutIndex.end(), compareCutIndex  );

		if ( debug_e ) cout << endl << revCutList.size() << " and " << golCutList.size() << " cuts found respectively." << endl << endl;

		// sort gol cuts by size
		size_t checkSize = 0;
		for( size_t k = 0; k < revCutList.size(); k++ )
		{

			if ( revCutList[k].size() > checkSize )
			{

				if ( revCutList[k].size() > minCheckSize ) break;

				// change gol cuts to current size
				checkSize = revCutList[k].size();

				// clear current gate's gol cut index
				for ( size_t i = 0; i < golGates.size(); i++ )
				{
					golGates[i] -> _cuts.clear();
				}
				for ( size_t i = 0; i < golCutIndex.size(); i++ )
				{
					NGateList &l = golCutList[golCutIndex[i]].gateList;
					if ( l.size() > checkSize ) break;
					for( size_t j = 0; j < l.size(); j++ )
					{
						l[j] -> _cuts.push_back( golCutIndex[i] );
					}
				}

			}

			// find pairing and prove
			if ( checkCutFunc2( g_old, g_gol, dupMerge,  simPairList, numGateLeft, gol_oldCutMaxId, (int)k > rev_oldCutMaxId, level, revCutList[k], resCut, golCutList, golCutIndex ) ) 
			{
				NGateList &l = revCutList[k].gateList;
				resCut = CutPair();
				for( size_t i = 0; i < l.size(); i++ )
				{
					resCut.oldCutIdList.push_back( l[i] -> getId() );
					resCut.goldCutIdList.push_back( l[i] -> _mapping -> getId() );
				}
				// TODO: bind pairing of the cut found
				// TODO: ignore gates above this cut
				// TODO: remove cuts with larger checksize

				level_last = level;
				minCheckSize = checkSize;
				break;
			}
		}
		timer_CF_proving.pause();

		level += 1;
	}


	if ( debug_c && level_last != 0 )
	{
		cout << "Last match cut found at level " << level_last << endl;
		cout << "Match cut with checksize " << resCut.size() << endl;
	}

	if ( !resCut.oldCutIdList.empty()  )
	{
		//cout << "Perform extra similarity check" << endl;
		// TODO: check if other similar pairing can get a higher strutural similarity
	}


	return resCut;
}

void
NtkMgr::fraigCF()
{
	// reference for CF fraig
	/*
	timer_CF_proving.pause();
	timer_CF_fraig.start();
	// TODO: think about the situation when
	// a fraig cut is found but a match cut is found later
	unsigned r = fraigCutFunc( g_old, g_gol, cutList[i] );
	if ( r > 0 && ( fraigUnmerged == 0 || r < fraigUnmerged ) ) 
	{
		cout << "found partial merged cutfunction" << endl;
		fraigUnmerged = r;
		resCut = cutList[i];
		level_last = level;

		NtkGate::increaseGlobalFlag(1);
		for( unsigned i = 0; i < resCut.oldCutIdList.size(); i++ )
		{
			getGate(resCut.goldCutIdList[i]) -> _cmg_list = getGate(resCut.oldCutIdList[i]);
			//getGate(resCut.goldCutIdList[i]) -> setFlag(0);
		}
		unsigned unmergedCnt = 0;
		g_gol -> postOrderVisitCutFunc();

		// TODO: store temp result and keep going deeper
		timer_CF_fraig.pause();
		return resCut;
	}
	timer_CF_fraig.pause();
	timer_CF_fraig.start();
	*/
}

void
NtkGate::clearFSC( int level )
{
}

void
NtkMgr::updateSimPairList( vector<SimEntry> &simPairList, vector<Cut> &revCutList, vector<Cut> &golCutList, 
	NGateList &revGates, NGateList &golGates, bool discardOldCuts )
{
	// TODO: try another structure of sim pair list

	NGateList rev_oldGatesInCuts, rev_oldGatesNotInCuts, rev_newGates;
	checkNewGatesInCuts( revCutList, revGates, rev_oldGatesInCuts, rev_oldGatesInCuts, rev_newGates );

	NGateList gol_oldGatesInCuts, gol_oldGatesNotInCuts, gol_newGates;
	checkNewGatesInCuts( golCutList, golGates, gol_oldGatesInCuts, gol_oldGatesInCuts, gol_newGates );


	// check discard and filter simpairlist
	if ( discardOldCuts )
	{
		vector<SimEntry> newSimPairList;

		revGates = rev_oldGatesInCuts;
		golGates = gol_oldGatesInCuts;
		golGates.push_back( getGate(_netList_gol[0]) );
		golGates.push_back( getGate(_netList_gol[1]) );

		// mark the old gates in new cuts
		// remove unmark gate in sim entry later
		for( size_t i = 0; i < rev_oldGatesInCuts.size(); i++ )
			rev_oldGatesInCuts[i] -> _ce_newOnCut = true;
		for( size_t i = 0; i < gol_oldGatesInCuts.size(); i++ )
			gol_oldGatesInCuts[i] -> _ce_newOnCut = true;

		getGate( _netList_gol[0] ) -> _ce_newOnCut = true;
		getGate( _netList_gol[1] ) -> _ce_newOnCut = true;
		
		// remove old pairs
		for( size_t i = 0; i < simPairList.size(); i++ )
		{
			if ( simPairList[i].old -> _ce_newOnCut && simPairList[i].gol -> _ce_newOnCut )
			{
				newSimPairList.push_back( simPairList[i] );
			}
		}

		// TODO: a filter function
		simPairList = newSimPairList;
	}
	
	if ( debug_e ) cout << "Simlarity: Rev(" << rev_newGates.size() << "/" << revGates.size() << ") Gol(" << gol_newGates.size() << "/" << golGates.size() << ")" << endl;

	// compute pairwise
	addPairwiseSimEntry( simPairList, rev_newGates, golGates );
	addPairwiseSimEntry( simPairList, revGates, gol_newGates );
	addPairwiseSimEntry( simPairList, rev_newGates, gol_newGates );

	revGates.insert( revGates.end(), rev_newGates.begin(), rev_newGates.end() );
	golGates.insert( golGates.end(), gol_newGates.begin(), gol_newGates.end() );

	sort( simPairList.begin(), simPairList.end(), compareSimEntry );

	for( size_t i = 0; i < revGates.size(); i++ )
	{
		revGates[i] -> _maxScore = 0;
	}
	for( size_t i = 0; i < simPairList.size(); i++ )
	{
		if ( simPairList[i].old -> _maxScore < simPairList[i].sim ) simPairList[i].old -> _maxScore = simPairList[i].sim;
	}

}

void
NtkMgr::checkNewGatesInCuts( vector<Cut> &cutList, NGateList &oldGates, 
	NGateList &oldGatesNotInCuts, NGateList &oldGatesInCuts, NGateList &newGatesInCuts )
{

	NGateList newGates_tmp;
	for( size_t i = 0; i < cutList.size(); i++ )
	{
		NGateList &l = cutList[i].gateList;
		for( size_t j = 0; j < l.size(); j++ )
		{
			// assert( l[j]->_old || l[j]->_type!=NTK_GATE_CONST );
			if( !l[j] -> _ce_newOnCut )
			{
				l[j] -> _ce_newOnCut = true;
				newGates_tmp.push_back(l[j]);
			}
		}
	}
	for( size_t i = 0; i < oldGates.size(); i++ )
	{
		if ( oldGates[i] -> _ce_newOnCut )
		{
			oldGates[i] -> _ce_newOnCut = false;
			oldGatesInCuts.push_back( oldGates[i] );
		}
		else
		{
			oldGatesNotInCuts.push_back( oldGates[i] );
		}
	}
	for( size_t i = 0; i < newGates_tmp.size(); i++ )
	{
		if ( newGates_tmp[i] -> _ce_newOnCut )
		{
			newGatesInCuts.push_back( newGates_tmp[i] );
		}
	}

}

void
NtkMgr::addPairwiseSimEntry( vector<SimEntry> &simPairList, NGateList &revGates, NGateList &golGates )
{

	float sim_simul, sim_struct;

	for( size_t i = 0; i < revGates.size(); i++ )
	{
		for( size_t j = 0; j < golGates.size(); j++ )
		{
			computeSimilarity( revGates[i], golGates[j], sim_simul, sim_struct );
			if ( ! (sim_simul < _sim_thre ) )
			{
				// TODO
				simPairList.emplace_back( sim_simul, sim_struct, revGates[i], golGates[j] );
				if ( sim_simul > revGates[i]->_maxScore ) revGates[i] -> _maxScore = sim_simul;

				/*
				// test
				if ( golGates[j] -> _type ==  NTK_GATE_CONST ) 
				{
					cout << "Similar: " << revGates[i] -> getName() << " -> " << golGates[j] -> getName() << " " << sim_simul << endl;
				}
				*/
				// cout << "Similar: " << revGates[i] -> getName() << " -> " << golGates[j] -> getName() << " " << sim_simul << endl;
				

			}
			

		}
	}
}

bool compareFSCEntry( FSCEntry &a, FSCEntry &b ) { return a.sim_simul < b.sim_simul; }
void
NtkGate::finalizeFSC( bool discardOldCuts )
{
	/*
	if ( discardOldCuts )
	{
		vector<FSCEntry> v;
		bool changed = false;
		for( size_t i = 0; i < _fsc.size(); i++ )
		{
			if ( _fsc[i].gate -> _ce_newOnCut ) v.push_back( _fsc[i] );
			else changed = true;
			// TODO: prevent unnecessary copy when nothing changed
		}
		if ( changed ) _fsc = v;
	}
	sort( _fsc.begin(), _fsc.end(), compareFSCEntry );
	cout << _fsc.size() << endl;
	*/
}

bool
NtkMgr::checkCutFunc2( NtkGate* g_old, NtkGate* g_gol, NGateList &dupMerge, vector<SimEntry> &simPairList, 
	vector<int> &numGateLeft, int gol_oldCutMaxId, bool revIsNewCut,
	int level, Cut &cut, CutPair &resCut, vector<Cut> &golCutList, vector<int> &golCutIdList )
{


	// just rebuild everytime first
	solver -> reset();

	// TODO: maybe use aig in gol?
	// TODO: add consant constraint in gol
	// mark gates in old
	NtkGate::increaseGlobalFlag(2);
	for( size_t i = 0; i < cut.size(); i++ )
		cut.gateList[i] -> setFlag(-1);
	for( size_t i = 0; i < dupMerge.size(); i++ )
		dupMerge[i] -> setFlag(-1);
	

	// create a list of cand to make decision
	//cout << "sim pair list with size " << simPairList.size() << endl;
	vector<SimEntry> candPairList;
	for( size_t i = 0; i < simPairList.size(); i++ )
	{
		if ( simPairList[i].old -> _flag == NtkGate::_globalFlag-1 ) 
			candPairList.push_back( simPairList[i] );

	}

	// add clause
	timer_CF_proving.start();
	g_old -> addClauseRec();
	g_gol -> addClauseRec( g_gol -> getLevel() - level );
	//g_gol -> addClauseRec( 0 );
	getGate(_netList_gol[0])->addClauseRec();
	getGate(_netList_gol[1])->addClauseRec();
	timer_CF_proving.pause();

	// assert miter
	Var v = satAddMiterNtk( g_old, g_gol );
	solver -> assertProperty( v, true );
	// assert dup merge
	for ( size_t i = 0; i < dupMerge.size(); i++  )
	{
		assert( dupMerge[i] -> _mapping );
		v = satAddMiterNtk( dupMerge[i], dupMerge[i] -> _mapping );
		solver -> assertProperty( v, false );
	}

	// start finding cut
	timer_cutSearching.start();
	bool match = addPairToFormCut( candPairList, 0, cut.size(), cut.size(), numGateLeft, dupMerge, cut.gateList, v, gol_oldCutMaxId, revIsNewCut, golCutList, golCutIdList, 0 );
	timer_cutSearching.pause();
	return match;

}

void
NtkGate::setCutFuncPi( unsigned i )
{
	_cutFuncPiId = i;
	_flag = _globalFlag-1;
}
void
NtkGate::preOrderVisitCutFunc()
{
	if ( _flag != _globalFlag ) return;

	_flag = _globalFlag-1;

	for( size_t i = 0; i < _fanout.size(); i++ )
		_fanout[i] -> preOrderVisitCutFunc();
}
void
NtkGate::postOrderVisitCutFunc( unsigned &unmergedCnt )
{
		if ( _flag == _globalFlag || _cmg_list ) return;
		unmergedCnt ++;
		_flag = _globalFlag;

		for( size_t i = 0; i < _fanin.size(); i++ )
			_fanin[i] -> postOrderVisitCutFunc( unmergedCnt );
}


void
NtkGate::postOrderVisit( int level )
{
	if ( _flag == _globalFlag ) return;

	_flag = _globalFlag;
	_ce_flag = CE_EMPTY;
	_ce_new = false;
	_ce_covered = false;
	_mapping = 0;

	// added for CF fraig
	ntkMgr->_CFGates_gol.push_back( this );

	if ( _level <= level || _mergeFlag ) return;
	// if ( _level <= level || _mergeFlag ) return;
	for( size_t i=0; i<_fanin.size(); i++)
		_fanin[i] -> postOrderVisit( level );
	
}

void 
NtkGate::markDupMerge( int level, NGateList &dupMerge, GateSetByNet &activeGates, int &numNew, bool newGate )
{

	if ( _flag == _globalFlag ) return;
	_flag = _globalFlag;

	_ce_flag = CE_ABOVE;
	_ce_covered = false;
	_ce_newOnCut = false;

	_ce_new = newGate;
	if ( newGate ) numNew++;

	_mapping = 0;

	// added for fraig CF
	ntkMgr -> _CFGates_rev.push_back( this);
	
	if ( _mg_list )
	{
		NtkGate *g = _mg_list, *mg = 0;
		while ( g ) 
		{
			if ( g -> _flag == _globalFlag )
			{
				if ( mg ) 
				{
					if ( g -> _level > mg -> _level )
					{
						mg = g;
						cout << "!! better dup merge found" << endl;
					}

				}
				else mg = g;
			}
			g = g->_mg_next;
		}

		// TODO: iterate through the list if not found
		if ( mg )
		{
			_ce_flag = CE_DUP;

			mg->_ce_flag = CE_DUP;
			_mapping = mg;

			dupMerge.push_back(this);
			// TODO: handle 1 to n map
			// TODO: handle invalid dup merge cut in gol
			return;
		}
	}

	if ( _level <= level || _mergeFlag )
	// if ( _level <= level || _mergeFlag ) 
	{
		_ce_flag = CE_CUT;
		activeGates.insert(this);
		return;
	}

	for( size_t i = 0; i < _fanin.size(); i ++ )
	{
		_fanin[i] -> markDupMerge( level, dupMerge, activeGates, numNew, _level == level+1 );
	}
}
void 
NtkGate::markGolBottomCut( int level, GateSetByNet &activeGates, int &numNew, bool newGate )
{
	// TODO: check if the dup merge part  have any overlap

	if ( _flag == _globalFlag ) return;
	_flag = _globalFlag;

	_ce_new = newGate;
	_ce_covered = false;
	_ce_newOnCut = false;

	if ( newGate ) numNew++;

	// if ( _level <= level || _mergeFlag ) 
	if ( _level <= level || _mergeFlag )
	{
		if ( _ce_flag != CE_DUP )
		{
			_ce_flag = CE_CUT;
			activeGates.insert(this);
		}
		return;
	}
	_ce_flag = CE_ABOVE;

	for( size_t i = 0; i < _fanin.size(); i ++ )
	{
		_fanin[i] -> markGolBottomCut( level, activeGates, numNew, _level == level+1 );
	}
}

void
NtkMgr::findPossibleCuts( IdList &oldCands, IdList &golCands )
{

}

Var
NtkMgr::satAddMiter( NtkGate* g1, NtkGate* g2 )
{
	Var v = solver -> newVar();
	//if ( g1->_aigNode.gate() == g2->_aigNode.gate() ) return -1;
	// TODO: handle when const is matching with a non-const gate
	g1->_aigNode.gate() -> addClauseRec();
	g2->_aigNode.gate() -> addClauseRec();
	solver -> addXorCNF(v, g1->getVar(), g1->getPhase(), g2->getVar(), g2->getPhase() );
	assert( g1->_aigNode.gate()->getFlag() >= CirGate::_globalFlag-1 );
	assert( g2->_aigNode.gate()->getFlag() >= CirGate::_globalFlag-1 );
	return v;
}
Var
NtkMgr::satAddMiterNtk( NtkGate* g1, NtkGate* g2 )
{
	//if ( g1->_aigNode.gate() == g2->_aigNode.gate() ) return -1;
	// TODO: handle when const is matching with a non-const gate
	/*
	assert( g1->getFlag() == NtkGate::_globalFlag );
	assert( g2->getFlag() == NtkGate::_globalFlag );
	*/
	if ( g1 -> getFlag() != NtkGate::_globalFlag || g2 -> getFlag() != NtkGate::_globalFlag )  return -1;
	Var v = solver -> newVar();
	solver -> addXorCNF(v, g1->getSatVar(), g1->getSatPhase(), g2->getSatVar(), g2->getSatPhase() );
	return v;
}


void
NtkGate::satAddClauseRec()
{
	_aigNode.gate() -> addClauseRec();
}


void
NtkMgr::computeSimilarity( NtkGate* g_old, NtkGate* g_gol, float &sim, float &sim_struct )
{
	sim = aigMiter -> cosineSimilarity( g_old, g_gol );
	sim_struct = getStructuralSimilarity( g_old, g_gol );
	// Optmize: add structual to simulation?
	sim += sim_struct;
}


/**********************************************************************
*                        New Cut Enumeration                         *
**********************************************************************/


void
NtkMgr::getGuildedCutCands2( vector<Cut>& revCutList, vector<Cut>& golCutList, NtkGate* g_old, NtkGate* g_gol, unsigned level, NGateList &dupMerge, bool &dupMergeChange, vector<SimEntry> &, int minCheckSize )
{

	int level_old = g_old -> getLevel() - level;
	int level_gol = g_gol -> getLevel() - level;
	
	revCutList.clear();
	golCutList.clear();

	// added for CF fraig
	_CFGates_rev.clear();
	_CFGates_gol.clear();

	GateSetByNet rec_activeGates, gol_activeGates;
	NGateList newDupMerge;

	int rec_numNew = 0;
	int gol_numNew = 0;

	// check dup & identify new gates in this level
	NtkGate::_globalFlag ++;
	g_gol -> postOrderVisit( level_gol );
	g_old -> markDupMerge( level_old, newDupMerge, rec_activeGates, rec_numNew, false );

	dupMergeChange = false;
	if ( !cutEqual( dupMerge, newDupMerge ) )
	{
		dupMergeChange = true;
		dupMerge = newDupMerge;
		rec_numNew = 1000;	// new gates are not necessary if dup merge is diff
		gol_numNew = 1000;
	}

	if ( debug_e ) cout << "New gates in this level: " << rec_numNew << endl;

	// TODO: verify the dup merge gates (make sure no overlap)
	// TODO: or at least remove overlapped dup
	// TODO: consider if new gate is a dup merge or a new dup merge is found


	_enumerate_gol = false;
	enumerateCuts( revCutList, dupMerge, rec_activeGates, rec_numNew, minCheckSize );

	// according to dupmerge, relocate the lowest cut & 
	// create the dupmerge in gol

	NtkGate::_globalFlag ++;
	NGateList gol_dupMerge;
	gol_dupMerge.reserve( dupMerge.size() );
	for( size_t i = 0; i < dupMerge.size(); i++ ) gol_dupMerge.push_back( dupMerge[i] -> _mapping );

	g_gol -> markGolBottomCut( level_gol, gol_activeGates, gol_numNew, false );
	_enumerate_gol = true;
	enumerateCuts( golCutList, gol_dupMerge, gol_activeGates, gol_numNew, minCheckSize );

}
void
NtkMgr::enumerateCuts( vector<Cut> &cutList, NGateList &dupMerge, GateSetByNet &activeGates, int numNew, int minCheckSize )
{

	Cut	taken;
	queue<NtkGate*>	checkFanin;
	queue<NtkGate*>	checkFanout;
	NGateList removed;
	GateSetByNet::iterator it;
	

	if ( activeGates.empty() ) 
	{
		// cout << "CE: trivial dup merge cut found." << endl;
		return;
	}

	// initial legalize:
		// for each gate
		// check fanout cone and pop again if needed
		// check fanin cone and enqueue any fanin gates on cut
	for( size_t i = 0; i < dupMerge.size(); i++ )
		checkFanin.push( dupMerge[i] );
	for( it = activeGates.begin(); it != activeGates.end(); it++ )
		checkFanin.push(*it);

	if ( legalizeCut( checkFanin, activeGates, numNew, removed ) )
	{

		if ( debug_e ) 
		{
			cout << "CE: Initial legalization success. Cut:";
			for( size_t i = 0, n = dupMerge.size(); i < n; i++ )
				cout << setw(W_NAME-1) << dupMerge[i] -> getName() << "*";
			for( it = activeGates.begin(); it != activeGates.end(); it++ )
				cout << setw(W_NAME) << (*it) -> getName();
			cout << endl;
		}

		makeDecisionOnCut( taken, activeGates, checkFanin, checkFanout, numNew, cutList, minCheckSize );
	}
	else
	{
		if ( debug_e ) cout << "CE: Initial legalization has failed." << endl;
	}

}
void
NtkMgr::makeDecisionOnCut( Cut &taken, GateSetByNet &activeGates, queue<NtkGate*> &checkFanin, queue<NtkGate*> &checkFanout, int &numNew, vector<Cut> &candList, int minChecksize )
{

		if ( taken.size() >= minChecksize ) return;

		NtkGate *g;
		assert( !activeGates.empty());
		g = *activeGates.begin();

		int before = activeGates.size();

		// take this
		activeGates.erase( g );
		g -> _ce_flag = CE_TAKEN;
		taken.gateList.push_back(g);
		// if ( ! ( _enumerate_gol && g -> getType() == NTK_GATE_CONST )  ) taken.gateList.push_back( g );

		//if ( debug_e ) cout << setw(W_NAME) << g->getName() << " (" << g->_netId <<  ") taken." << endl;

		if ( activeGates.empty() )
		{
			// push cut
			candList.push_back( taken );

			if ( debug_e )
			{
				cout << "Cut found:";
				// a cut is found
				for( size_t i = 0; i < taken.size(); i++ )
				{
					cout << setw(W_NAME) << taken.gateList[i] -> getName();
				}
				cout << endl;
			}
			
		}
		// make the next decision
		else makeDecisionOnCut( taken, activeGates, checkFanin, checkFanout, numNew, candList, minChecksize );

		// cout << setw(W_NAME) << g->getName() << " (" << g->_netId <<  ") pop up." << endl;
		activeGates.insert( g );
		// if ( ! ( _enumerate_gol && g -> getType() == NTK_GATE_CONST )  ) taken.gateList.pop_back();
		taken.gateList.pop_back();
		g -> _ce_flag = CE_CUT;

		NGateList removed;

		// pop this
		if ( g -> legalize_popUp( checkFanin, activeGates, numNew ) )
		{
			if ( legalizeCut( checkFanin, activeGates, numNew, removed ) )
			{
				if ( debug_e )
				{
					cout << setw(W_NAME) << g->getName() << " popped." << endl;
					cout << "Current cut:";
					// a cut is found
					for( size_t i = 0; i < taken.size(); i++ )
					{
						cout << setw(W_NAME) << taken.gateList[i] -> getName();
					}
					for( GateSetByNet::iterator it = activeGates.begin(); it != activeGates.end(); it++ )
					{
						cout << setw(W_NAME) << (*it) -> getName();
					}
					cout << endl;
					cout << "Removed gate:";
					for( size_t i = 0; i < removed.size(); i++ )
					{
						cout << setw(W_NAME) << removed[i] -> getName();
					}
					cout << endl;
				}
				// make the next decision

				makeDecisionOnCut( taken, activeGates, checkFanin, checkFanout, numNew, candList, minChecksize );

			}
			else if ( debug_e ) cout << setw(W_NAME) << g->getName() << " pop failed." << endl;
		}
		else if ( debug_e ) cout << setw(W_NAME) << g->getName() << " pop failed. (no new gates)" << endl;


		while( !checkFanin.empty() ) checkFanin.pop();
		while( !checkFanout.empty() ) checkFanout.pop();

		// recover
		g -> legalize_popDown( checkFanout, activeGates, numNew );
		recoverCut( checkFanout, activeGates, numNew, removed );


		//return
		if ( debug_e )
		{
			cout << "Recover to cut:";
			// a cut is found
			for( size_t i = 0; i < taken.size(); i++ )
			{
				cout << setw(W_NAME) << taken.gateList[i] -> getName();
			}
			for( GateSetByNet::iterator it = activeGates.begin(); it != activeGates.end(); it++ )
			{
				cout << setw(W_NAME) << (*it) -> getName();
			}
			cout << endl;
		}

		assert( activeGates.size() == before );
}
void
NtkMgr::recoverCut( queue<NtkGate*> &checkFanout, GateSetByNet &activeGates, int &numNew, NGateList &removed )
{
	if ( !removed.empty() )
	{

		// cout << "Removed: ";
		for( size_t i = 0; i < removed.size(); i++ )
		{
			removed[i] -> legalize_popDown( checkFanout, activeGates, numNew );
			//cout << setw(W_NAME) << removed[i] -> getName();
		}
		// cout << endl;
		removed.clear();
	}

	while( !checkFanout.empty() )
	{
		//cout << "checking fanout: " << checkFanout.front() -> getName() << endl;
		assert ( checkFanout.front() -> legalize_checkFanout( true, checkFanout, activeGates, numNew ) );
		checkFanout.pop();
	}

}
bool	
NtkMgr::legalizeCut( queue<NtkGate*> &checkFanin, GateSetByNet &activeGates, int &numNew, NGateList& removed )
{
	if ( numNew == 0 )  return false;

	while( !checkFanin.empty() )
	{
		//cout << "checking fanin: " << checkFanin.front() -> getName() << endl;
		if ( !checkFanin.front() -> legalize_checkFanin( true, checkFanin, activeGates, numNew, removed ) ) return false;
		checkFanin.pop();
	}
	return true;
}
bool
NtkGate::legalize_checkFanout( bool begin, queue<NtkGate*> &checkFanout, GateSetByNet &activeGates, int &numNew )
{
	if ( _flag != _globalFlag ) return true;
	if ( _ce_flag == CE_ABOVE ) return true;

	if ( !begin )
	{
		// if trying to remove a taken/dup gate, the legalization fail
		if ( _ce_flag == CE_TAKEN || _ce_flag == CE_DUP ) 
		{
			if ( debug_e ) cout << " check fanout fail while trying to remove " << getName() << endl;
			return false;
		}

		// remove gate from cut
		if ( _ce_flag == CE_CUT )
		{
			//cout << "remove " << getName() << " from cut" << endl;
			_ce_flag = CE_ABOVE;
			_ce_covered = true;
			activeGates.erase( this );

			// add fanins
			NtkGate* g;
			queue<NtkGate*> q;
			for( size_t i = 0; i < _fanin.size(); i++ )
			{
				q.push(_fanin[i]);
			}

			while( ! q.empty() )
			{
				g = q.front(); q.pop();

				if ( g -> _flag != _globalFlag ) continue; // out of CF

				if ( g -> _ce_flag == CE_ABOVE && !g-> _ce_covered )	// still above cut and not covered
				{
					g -> _ce_covered = true;
					for( size_t i = 0, n = g->_fanin.size(); i < n; i++ )
						q.push(g->_fanin[i]);
				}
				else if ( g -> _ce_flag == CE_EMPTY || g -> _ce_flag == CE_BELOW )
				{
					g -> _ce_flag = CE_CUT;
					g -> _ce_covered = false;
					activeGates.insert(g);
					checkFanout.push(g);
					if ( g -> _ce_new ) numNew ++;
				}
			}
		}
		else
		{
			if ( _ce_flag != CE_ABOVE ) _ce_covered = false;
			_ce_flag = CE_ABOVE;
		}
		
	}
	// else cout << "checkfanout begin from " << getName() <<  " " << (int)_ce_flag << endl;
	
	for( size_t i = 0; i < _fanout.size(); i++ )
	{
		if ( !_fanout[i] -> legalize_checkFanout( false, checkFanout, activeGates, numNew ) ) return false;
	}

	return true;

}
bool
NtkGate::legalize_checkFanin( bool begin, queue<NtkGate*> &checkFanin, GateSetByNet &activeGates, int &numNew, NGateList &removed )
{
	if ( _flag != _globalFlag ) return true;
	if ( _ce_flag == CE_BELOW ) return true;

	if ( !begin )
	{

		//cout << getName() <<  " " << (int)_ce_flag << " is begin check" << endl;

		// if trying to remove a taken/dup gate, the legalization fail
		if ( _ce_flag == CE_TAKEN || _ce_flag == CE_DUP ) 
		{
			if ( debug_e ) cout << "CE Failed while trying to remove " << getName() << endl;
			removed.push_back( this );
			return false;
		}

		// remove gate from cut
		if ( _ce_flag == CE_CUT )
		{
			removed.push_back( this );
			if ( ! legalize_popUp( checkFanin, activeGates, numNew ) ) return false;
		}
		else
		{
			if ( _ce_flag != CE_BELOW ) _ce_covered = false;
			_ce_flag = CE_BELOW;
		}
		
	}
	// else cout << "checkfanin begin from " << getName() <<  " " << (int)_ce_flag << endl;
	
	for( size_t i = 0; i < _fanin.size(); i++ )
	{
		if ( !_fanin[i] -> legalize_checkFanin( false, checkFanin, activeGates, numNew, removed ) ) return false;
	}

	return true;

}
bool
NtkGate::legalize_popUp( queue<NtkGate*> &checkFanin, GateSetByNet &activeGates, int &numNew )
{
	if ( _ce_new ) 
	{
		numNew--;
		if ( numNew == 0 )
		{
			numNew++;
			return false;
		}
	}

	_ce_flag = CE_BELOW;
	_ce_covered = true;
	activeGates.erase( this );

	// add fanouts
	NtkGate* g;
	queue<NtkGate*> q;
	for( size_t i = 0; i < _fanout.size(); i++ )
	{
		q.push(_fanout[i]);
	}

	while( ! q.empty() )
	{
		g = q.front(); q.pop();
		//cout << "poping up: " <<  g-> getName() << ": "  << g->_ce_covered << " " << g->_ce_flag  << endl;

		if ( g -> _flag != _globalFlag ) continue; // out of CF

		if ( g -> _ce_flag == CE_BELOW && !g-> _ce_covered )	// still below cut and not covered
		{
			g -> _ce_covered = true;
			for( size_t i = 0, n = g->_fanout.size(); i < n; i++ )
			{
				q.push(g->_fanout[i]);
			}
		}
		else if ( g -> _ce_flag == CE_EMPTY || g -> _ce_flag == CE_ABOVE )
		{
			g -> _ce_flag = CE_CUT;
			g -> _ce_covered = false;
			activeGates.insert( g );
			checkFanin.push(g);
		}
	}

	return true;
}
void
NtkGate::legalize_popDown( queue<NtkGate*> &checkFanout, GateSetByNet &activeGates, int &numNew )
{

	checkFanout.push( this );

	if ( _ce_flag == CE_CUT || _ce_flag == CE_TAKEN || _ce_flag == CE_DUP ) return;

	activeGates.insert( this );
	_ce_flag = CE_CUT;
	_ce_covered = false;

	if ( _ce_new ) numNew ++ ;
}

// return true if match
bool
NtkMgr::addPairToFormCut( vector<SimEntry>& simVec, unsigned index, int numGateLeftInCut, const int numGateInCut, vector<int> &numGateLeft, NGateList &dupMerge, NGateList &revCut, Var miter, int oldCutMaxId, bool revIsNewCut, vector<Cut> &golCutList, vector<int> &golCutIdList, int numConstMatch )
{
	// use this part to enable the random pairing on the last gate
	if ( numGateLeftInCut == 1 )
	{
		// cout << "selected: " << numGateInCut - numGateInCut << endl;
		vector<int> candCutList;
		NGateList candList;

		// find all candidates cut with one gate left and make sure it's new
		for( size_t i = revIsNewCut ? 0 : oldCutMaxId + 1; i < golCutList.size(); i++ )
		{
			if ( golCutList[i].size() == numGateInCut - numConstMatch  && numGateLeft[i] == 1 ) 
			{
				NGateList &l = golCutList[i].gateList;
				bool found = false;
				for( size_t j = 0; j < l.size(); j++ )
				{
					if ( ! l[j] -> _mapping )
					{
						candList.push_back( l[j] );
						found = true;
						break;
					}
				}
				assert( found );
			}
			/*
			else if ( golCutList[i].size() != numGateInCut - numConstMatch )
			{
				cout << "cut " << i << " is not used due to different size" << endl;
			}
			else
			{
				cout << "cut " << i << " is not used due to no enough pairing" << endl;
			}
			*/
			// TODO:
			// include the cut with the same checksize with a constant in it
		}
		if ( numConstMatch == 0 )
		{
			// include the cut with checksize-1 if no constant is paired currently
			// and pair the last gate with a constant
			for( size_t i = revIsNewCut ? 0 : oldCutMaxId + 1; i < golCutList.size(); i++ )
			{
				if ( numGateLeft[i] == 0 ) 
				{
					candList.push_back( getGate( _netList_gol[0] ) );
					candList.push_back( getGate( _netList_gol[1] ) );
					break;
				}
			}
		}

		// cout << "cand list size: " <<  candCutList.size() << endl;

		bool match = false;

		if ( !candList.empty() )
		{
			// if equiv, return this
			timer_cutSearching.pause();
			timer_CF_proving.start();
			match = checkCutFuncEquivWithOneGateLeft( revCut, miter, candList );
			timer_CF_proving.pause();
			timer_cutSearching.start();
		}

		return match;
	}

	if (index >= simVec.size() ) return false;

	NtkGate *go = simVec[index].old;
	NtkGate *gg = simVec[index].gol;

	if ( go -> _mapping || gg ->  _mapping ) 
		return addPairToFormCut( simVec, index+1, numGateLeftInCut, numGateInCut, numGateLeft, dupMerge, revCut, miter, oldCutMaxId, revIsNewCut, golCutList, golCutIdList, numConstMatch );

	// cout << "select " << go -> getName() << " -> " << gg -> getName() << endl;
	go -> _mapping = gg;
	gg -> _mapping = go;

	// cout << "select " << go -> getName() << " and " << gg -> getName() << endl;

	// check gg's mapping
	vector<int> &c = gg->_cuts;
	bool done = false, found = false, possible = false;

	for( size_t i = 0; i < c.size(); i++ )
	{
		// note that we can check it like this only without constant insertion
		numGateLeft[c[i]]--;

		// use this part instead to disable the random pairing on the last gate
		/*
		if ( numGateLeftInCut == 1 && numGateLeft[c[i]] == 0 )
		{
			// a paring found, start proving
			found = true;

			// check if its a new cut
			if ( !revIsNewCut && c[i] <= oldCutMaxId ) continue;

			// cout << "Pairing found with size " << revCut.size() <<  ": ";
			// for( size_t i = 0; i < revCut.size(); i++ ) cout << setw(W_NAME) << revCut[i] -> getName();
			// cout << endl;

			// if equiv, return this
			timer_cutSearching.pause();
			timer_CF_proving.start();
			done = checkCutFuncEquiv( revCut, miter );
			timer_CF_proving.pause();
			timer_cutSearching.start();
		} else
		*/

		if ( numGateLeft[c[i]] <= numGateLeftInCut - 1 )
		{
			if ( revIsNewCut || c[i] > oldCutMaxId ) possible = true;
		}
		
	}

	// if no pairing found, continue the decision process
	// also, if it's impossible to find a cut, return;
	{
		if ( gg -> _type == NTK_GATE_CONST )
		{
			// cout << "constant pair selected!!" << endl;
			done = addPairToFormCut( simVec, index+1, numGateLeftInCut-1, numGateInCut, numGateLeft, dupMerge, revCut, miter, oldCutMaxId, revIsNewCut, golCutList, golCutIdList, numConstMatch + 1 );
		}
		else 
		{
			if ( !found  && possible ) done = addPairToFormCut( simVec, index+1, numGateLeftInCut-1, numGateInCut, numGateLeft, dupMerge, revCut, miter, oldCutMaxId, revIsNewCut, golCutList, golCutIdList, numConstMatch );
		}

		// TODO:
		// what if the cut contains const gate?
	}


	// unselect and pick other
	// if ( debug_m ) cout << "cut_o: unselect" << setw(W_NAME) << go -> getName() << "..." << endl;
	// if ( debug_m ) cout << "cut_g: unselect" << setw(W_NAME) << gg -> getName() << "..." << endl;


	for( size_t i = 0; i < c.size(); i++ ) numGateLeft[c[i]]++;

	// cout << "unselect " << go -> getName() << " and " << gg -> getName() << endl;

	if ( done ) return true;

	go -> _mapping = 0;
	gg -> _mapping = 0;
	// cout << "unselect " << go -> getName() << " -> " << gg -> getName() << endl;

	return addPairToFormCut( simVec, index+1, numGateLeftInCut, numGateInCut, numGateLeft, dupMerge, revCut, miter, oldCutMaxId, revIsNewCut, golCutList, golCutIdList, numConstMatch );

}

bool
NtkMgr::checkCutFuncEquivWithOneGateLeft( NGateList &revCut, Var miter, NGateList &candList )
{

	bool sat;
	Var v;
	NtkGate* gLeft = 0;
	vector<Var> vars; 

	solver -> assumeRelease();

	// debug
	/*
	cout << "Rev Cut: " << endl;
	for( size_t i = 0; i < revCut.size(); i++ )
	{
		cout << setw(W_NAME) << revCut[i] -> getName();
	}
	cout << endl;
	*/

	for ( size_t i = 0; i < revCut.size(); i++  )
	{
		if ( ! revCut[i] -> _mapping )
		{
			gLeft = revCut[i];
			// cout << setw(W_NAME) << " ";
			continue;
		}
		// cout << setw(W_NAME) << revCut[i]->_mapping ->getName() ;
		v = satAddMiterNtk( revCut[i], revCut[i] -> _mapping );
		assert( v != -1 ); 
		vars.push_back(v);
	}
	// cout << endl;

	// try sorting the candList
	/*
	vector<FSCEntry> sortedCandList;
	for( size_t i = 0; i < candList.size(); i++ )
	{
		FSCEntry e;
		computeSimilarity( gLeft, candList[i], e.sim_simul, e.sim_struct );
		e.gate = candList[i];
		sortedCandList.push_back(e);
	}
	sort( sortedCandList.begin(), sortedCandList.end(), compareFSCEntry );
	for( size_t i = 0; i < sortedCandList.size(); i++ )
	{
		candList[i] = sortedCandList[i].gate;
	}
	*/

	// cout << candList.size() << " candidates to be paired with the last gate." << endl;
	for( size_t i = 0; i < candList.size(); i++ )
	{
		// cout << "Random Pair: trying " << candList[i]->getName() << endl;

		gLeft -> _mapping = candList[i];
		v = satAddMiterNtk( gLeft, candList[i] );
		assert( v != -1 );
		solver -> assumeProperty( v, false );

		for( size_t j = 0; j < vars.size(); j++ )
		{
			solver -> assumeProperty( vars[j], false );
		}

		sat = solver->assumpSolve();
		solver -> assumeRelease();

		if ( !sat ) 
		{
			if ( debug_c ) reportMatchCut( revCut );
			break;
		}

	}
	
	if (sat) gLeft -> _mapping = 0;

	return !sat;
}
bool
NtkMgr::checkCutFuncEquiv( NGateList &revCut, Var miter )
{

	bool sat;
	Var v;

	for ( size_t i = 0; i < revCut.size(); i++  )
	{
		assert( revCut[i] -> _mapping );
		v = satAddMiterNtk( revCut[i], revCut[i] -> _mapping );

		if ( (int)v == -1 ) 
		{
			cout << "Warning: gate "  <<  revCut[i] -> getName() << " on the cut are not visited." << endl;
			continue; 
		}
		solver -> assumeProperty( v, false );
	}

	sat = solver->assumpSolve();
	solver -> assumeRelease();

	if ( !sat  & debug_c ) reportMatchCut( revCut );

	return !sat;
}
void
NtkMgr::reportMatchCut( NGateList &cut )
{
	cout << "Match cut: ";
	for( size_t i = 0; i < cut.size(); i++ )
		cout << setw(W_NAME) << cut[i] -> getName();
	cout << endl << setw(11) << "";
	for( size_t i = 0; i < cut.size(); i++ )
		cout << setw(W_NAME) << cut[i] -> _mapping -> getName();
	cout << endl << setw(11) << "";
	float sim, sim_struct;
	for( size_t i = 0; i < cut.size(); i++ )
	{
		computeSimilarity( cut[i], cut[i]->_mapping, sim, sim_struct );
		cout << setw(W_NAME) << sim;
	}
	cout << endl << setw(11) << "";
	for( size_t i = 0; i < cut.size(); i++ )
	{
		computeSimilarity( cut[i], cut[i]->_mapping, sim, sim_struct );
		cout << setw(W_NAME) << sim_struct;
	}
	cout << endl;
}
bool
NtkMgr::cutEqual( NGateList &c1, NGateList &c2 )
{

	// consider order?

	if ( c1.size() != c2.size() ) return false;
	for( size_t i = 0; i < c1.size(); i++ )
	{
		if ( c1[i] != c2[i] ) return false;
	}
	return true;
}


// CF Fraig
CutPair
NtkMgr::cutMatchingWithFraig( NtkGate* g_old, NtkGate* g_gol )
{
	// modified
	unsigned level = 1;
	_g_rev = g_old;
	_g_gol = g_gol;
	_unmergedCnt = INT_MAX;
	_bwdCut.clear();
	_bwdCutMapping.clear();
	_matchCut.clear();
	_matchCutMapping.clear();

	// debug
	if ( debug_f ) 
	{
		cout << "\n==============================\n\n";
		cout << "Matching with fraig: " << g_old -> _name << " " << g_gol -> _name << endl;
		cout << endl;
	}


	CutPair resCut;
	NGateList dupMerge;
	bool dupMergeChange;
	resCut.checkSize = INT_MAX;

	vector<Cut> revCutList, golCutList, revCutList_new, golCutList_new;
	vector<CutPair> cutList;
	vector<SimEntry> simPairList;
	NGateList revGates, golGates;
	vector<int> revCutIndex, golCutIndex;

	int	minCheckSize = _minCS_init;		
	int level_last = 0;

	golGates.push_back( getGate(_netList_gol[0]) );
	golGates.push_back( getGate(_netList_gol[1]) );


	if ( debug_f )
	{
		cout << endl;
		// debug
		NtkGate::increaseGlobalFlag();
		g_old -> postOrderPrint( g_old -> getLevel() - _lv_thre, 0 );
		//g_gol -> postOrderPrint( g_gol -> getLevel() - LV_THRE, 0 );
		g_gol -> printUntilMerge( 0 );
		cout << endl;
	}

	// incremental add level
	while ( level <= _lv_thre ) 
	{
		_level = level;
		// debug
		if ( debug_f ) cout << "level = " << level << endl;

		timer_cutSearching.start();
		getGuildedCutCands2( revCutList_new, golCutList_new, g_old, g_gol, level, dupMerge, dupMergeChange, simPairList, minCheckSize );
		timer_cutSearching.pause();

		getGate(_netList_gol[0]) -> _mapping = 0;
		getGate(_netList_gol[1]) -> _mapping = 0;

		// update similarity entry vector
		timer_similarity.start();
		updateSimPairList( simPairList, revCutList_new, golCutList_new, revGates, golGates, dupMergeChange );
		timer_similarity.pause();


		if ( dupMergeChange )
		{
			// remove previous cuts
			revCutList.clear();
			golCutList.clear();

			if ( debug_f )
			{
				cout << "New dup merge: "  << endl;
				for( size_t i = 0; i < dupMerge.size(); i++ )
					cout << setw(W_NAME) << dupMerge[i] -> getName();
				cout << endl;
				for( size_t i = 0; i < dupMerge.size(); i++ )
					cout << setw(W_NAME) << dupMerge[i] -> _mapping -> getName();
				cout << endl;
			}

		}

		// mod
		_dupMerge = dupMerge;

		vector<int> numGateLeft;
		int rev_oldCutMaxId = revCutList.size() - 1;	// if index > this, it's a new cut
		int gol_oldCutMaxId = golCutList.size() - 1;


		revCutList.insert( revCutList.end(), revCutList_new.begin(), revCutList_new.end() );
		golCutList.insert( golCutList.end(), golCutList_new.begin(), golCutList_new.end() );

		// TODO: trivial dup merge cut proving
		// since it didn't merge in fraig, some error might be in this CF
		if ( !dupMerge.empty() && revCutList.empty() && golCutList.empty() )
		{
			if ( debug_f ) cout << "Warning: trivial dup merge found." << endl;

			// break while loop since trival cut is found
			break;
		}

		// compute rev cut score
		revCutIndex.clear();
		for( size_t i = 0; i < revCutList.size(); i++ )
		{
			revCutIndex.push_back(i);
			revCutList[i].score = 0;
			NGateList &l = revCutList[i].gateList;
			for( size_t j = 0; j < l.size(); j++ )
			{
				revCutList[i].score += l[j] -> _maxScore;
			}
		}
		cutListToSort = &revCutList;
		sort( revCutIndex.begin(), revCutIndex.end(), compareCutIndexWithScore );


		// sort gol cut by checksize
		golCutIndex.clear();
		for( size_t i = 0; i < golCutList.size(); i++ ) 
		{
			numGateLeft.push_back( golCutList[i].size() );
			//cout << "gold cut with size " << golCutList[i].size() << endl; 
			golCutIndex.push_back(i);
		}
		cutListToSort = &golCutList;
		sort( golCutIndex.begin(), golCutIndex.end(), compareCutIndex  );

		if ( debug_e ) cout << endl << revCutList.size() << " and " << golCutList.size() << " cuts found respectively." << endl << endl;

		// sort gol cuts by size
		size_t checkSize = 0;
		for( size_t k = 0; k < revCutList.size(); k++ )
		{


			if ( revCutList[k].size() > checkSize )
			{

				if ( revCutList[k].size() > minCheckSize ) break;

				// change gol cuts to current size
				checkSize = revCutList[k].size();

				// clear current gate's gol cut index
				for ( size_t i = 0; i < golGates.size(); i++ )
				{
					golGates[i] -> _cuts.clear();
				}
				for ( size_t i = 0; i < golCutIndex.size(); i++ )
				{
					NGateList &l = golCutList[golCutIndex[i]].gateList;
					if ( l.size() > checkSize ) break;
					for( size_t j = 0; j < l.size(); j++ )
					{
						l[j] -> _cuts.push_back( golCutIndex[i] );
					}
				}

			}

			// find pairing and prove
			// mod
			// TODO: try more cut at higher level, pick the one with best result?

			if ( fraigCutFunc2( g_old, g_gol, dupMerge,  simPairList, numGateLeft, gol_oldCutMaxId, k > rev_oldCutMaxId, level, revCutList[k], resCut, golCutList, golCutIndex ) )
			{
				break;
			}

		}
		timer_CF_proving.pause();

		level += 1;

	}

	if ( !_bwdCut.empty() )
	{
		// try to add bwd patch. if success, return match cut
		NtkGate::increaseGlobalFlag(2);
		for( size_t i = 0; i < _bwdCut.size(); i++ )
		{
			_bwdCut[i] -> setFlag(-1);
		}
		g_gol -> postOrderVisitToCut();
		NtkGatePQ q;

		for( size_t i = 0; i < _bwdCut.size(); i++ )
		{
			// TODO: check loop
			_bwdCut[i] -> addRecPair( _bwdCutMapping[i], q, _bwdPatchList );
		}

		resCut.checkSize = _matchCut.size();
		for( size_t i = 0; i < _matchCut.size(); i++ )
		{
			resCut.oldCutIdList.push_back( _matchCut[i] -> getId() );
			resCut.goldCutIdList.push_back( _matchCutMapping[i] -> getId() );
		}

	}


	return resCut;
	
}
bool
NtkMgr::fraigCutFunc2( NtkGate* g_old, NtkGate* g_gol, NGateList &dupMerge, vector<SimEntry> &simPairList, 
	vector<int> &numGateLeft, int gol_oldCutMaxId, bool revIsNewCut,
	int level, Cut &cut, CutPair &resCut, vector<Cut> &golCutList, vector<int> &golCutIdList )
{

	unsigned numPi = _dupMerge.size() + cut.size();
	if ( debug_f ) 
	{
		cout << "Fraig with rev cut: " << endl;
		for( size_t i = 0; i < dupMerge.size(); i++ )
			cout << setw(W_NAME) << dupMerge[i] -> getName();
		for( unsigned i = 0; i < cut.size(); i++ )
			cout << setw(W_NAME) << cut.gateList[i] -> getName();
		cout << endl;
	}

	// build rev CF
	timer_cutSearching.pause();
	timer_CF_fraig.start();
	NtkGate::increaseGlobalFlag(2);
	for( size_t i = 0; i < dupMerge.size(); i++ )
		dupMerge[i] -> setCutFuncPi(i);
	for( unsigned i = 0; i < cut.size(); i++ )
		cut.gateList[i] -> setCutFuncPi(i+dupMerge.size());

	unsigned gateCnt = 0;
	g_old -> outputCutFunc( ".cf_rev.v", numPi, gateCnt);
	timer_CF_fraig.pause();
	timer_cutSearching.start();


	// create a list of cand to make decision
	//cout << "sim pair list with size " << simPairList.size() << endl;
	vector<SimEntry> candPairList;
	for( size_t i = 0; i < simPairList.size(); i++ )
	{
		if ( simPairList[i].old -> _flag == NtkGate::_globalFlag-1 ) 
			candPairList.push_back( simPairList[i] );

	}


	// start finding cut
	timer_cutSearching.start();
	bool match = addPairToFormCutAndFraig( candPairList, 0, cut.size(), cut.size(), numGateLeft, dupMerge, cut.gateList, gol_oldCutMaxId, revIsNewCut, golCutList, golCutIdList, 0 );
	timer_cutSearching.pause();



	return match;

}

bool
NtkMgr::addPairToFormCutAndFraig( vector<SimEntry>& simVec, unsigned index, int numGateLeftInCut, const int numGateInCut, vector<int> &numGateLeft, NGateList &dupMerge, NGateList &revCut, int oldCutMaxId, bool revIsNewCut, vector<Cut> &golCutList, vector<int> &golCutIdList, int numConstMatch )
{
	// use this part to enable the random pairing on the last gate
	/*
	if ( numGateLeftInCut == 1 )
	{
		// cout << "selected: " << numGateInCut - numGateInCut << endl;
		vector<int> candCutList;
		NGateList candList;

		// find all candidates cut with one gate left and make sure it's new
		for( size_t i = revIsNewCut ? 0 : oldCutMaxId + 1; i < golCutList.size(); i++ )
		{
			if ( golCutList[i].size() == numGateInCut - numConstMatch  && numGateLeft[i] == 1 ) 
			{
				NGateList &l = golCutList[i].gateList;
				bool found = false;
				for( size_t j = 0; j < l.size(); j++ )
				{
					if ( ! l[j] -> _mapping )
					{
						candList.push_back( l[j] );
						found = true;
						break;
					}
				}
				assert( found );
			}
			// TODO:
			// include the cut with the same checksize with a constant in it
		}
		if ( numConstMatch == 0 )
		{
			// include the cut with checksize-1 if no constant is paired currently
			// and pair the last gate with a constant
			for( size_t i = revIsNewCut ? 0 : oldCutMaxId + 1; i < golCutList.size(); i++ )
			{
				if ( numGateLeft[i] == 0 ) 
				{
					candList.push_back( getGate( _netList_gol[0] ) );
					candList.push_back( getGate( _netList_gol[1] ) );
					break;
				}
			}
		}

		// cout << "cand list size: " <<  candCutList.size() << endl;

		bool match = false;

		if ( !candList.empty() )
		{
			// if equiv, return this
			timer_cutSearching.pause();
			timer_CF_fraig.start();
			match = fraigCutFuncWithOneGateLeft( revCut, candList );
			timer_CF_fraig.pause();
			timer_cutSearching.start();
		}

		return match;
	}
	*/

	if (index >= simVec.size() ) return false;

	NtkGate *go = simVec[index].old;
	NtkGate *gg = simVec[index].gol;

	if ( go -> _mapping || gg ->  _mapping ) 
		return addPairToFormCutAndFraig( simVec, index+1, numGateLeftInCut, numGateInCut, numGateLeft, dupMerge, revCut, oldCutMaxId, revIsNewCut, golCutList, golCutIdList, numConstMatch );

	go -> _mapping = gg;
	gg -> _mapping = go;

	// cout << "select " << go -> getName() << " and " << gg -> getName() << endl;

	// check gg's mapping
	vector<int> &c = gg->_cuts;
	bool done = false, found = false, possible = false;

	for( size_t i = 0; i < c.size(); i++ )
	{
		// note that we can check it like this only without constant insertion
		numGateLeft[c[i]]--;

		// use this part instead to disable the random pairing on the last gate
		if ( numGateLeftInCut == 1 && numGateLeft[c[i]] == 0 )
		{
			// a paring found, start proving
			found = true;

			// check if its a new cut
			if ( !revIsNewCut && c[i] <= oldCutMaxId ) continue;

			// cout << "Pairing found with size " << revCut.size() <<  ": ";
			// for( size_t i = 0; i < revCut.size(); i++ ) cout << setw(W_NAME) << revCut[i] -> getName();
			// cout << endl;

			// if equiv, return this
			timer_cutSearching.pause();
			timer_CF_fraig.start();
			done = fraigCutFunc( revCut );
			timer_CF_fraig.pause();
			timer_cutSearching.start();
		} else

		if ( numGateLeft[c[i]] <= numGateLeftInCut - 1 )
		{
			if ( revIsNewCut || c[i] > oldCutMaxId ) possible = true;
		}
		
	}

	// if no pairing found, continue the decision process
	// also, if it's impossible to find a cut, return;
	{
		if ( gg -> _type == NTK_GATE_CONST )
		{
			// cout << "constant pair selected!!" << endl;
			done = addPairToFormCutAndFraig( simVec, index+1, numGateLeftInCut-1, numGateInCut, numGateLeft, dupMerge, revCut, oldCutMaxId, revIsNewCut, golCutList, golCutIdList, numConstMatch + 1 );
		}
		else 
		{
			if ( !found  && possible ) done = addPairToFormCutAndFraig( simVec, index+1, numGateLeftInCut-1, numGateInCut, numGateLeft, dupMerge, revCut, oldCutMaxId, revIsNewCut, golCutList, golCutIdList, numConstMatch );
		}

		// TODO:
		// what if the cut contains const gate?
	}


	// unselect and pick other
	// if ( debug_m ) cout << "cut_o: unselect" << setw(W_NAME) << go -> getName() << "..." << endl;
	// if ( debug_m ) cout << "cut_g: unselect" << setw(W_NAME) << gg -> getName() << "..." << endl;


	for( size_t i = 0; i < c.size(); i++ ) numGateLeft[c[i]]++;

	// cout << "unselect " << go -> getName() << " and " << gg -> getName() << endl;

	if ( done ) return true;

	go -> _mapping = 0;
	gg -> _mapping = 0;

	return addPairToFormCutAndFraig( simVec, index+1, numGateLeftInCut, numGateInCut, numGateLeft, dupMerge, revCut, oldCutMaxId, revIsNewCut, golCutList, golCutIdList, numConstMatch );

}
unsigned
NtkMgr::fraigCutFunc( NtkGate* g_old, NtkGate* g_gol, CutPair &cut )
{

	// TODO: threshold condition
	/*
	if ( cut.score / cut.checkSize < 0.5 ) 
	{
		//cout << "Cut average score " << cut.score/cut.checkSize << "< 0.8, won't perform fraig" << endl;
		return false;
	}
	*/

	// if ( debug_f ) cout << "Cut function fraig ... ";

	unsigned numPi = cut.oldCutIdList.size();
	_cmg_heads.clear();
	NtkGate::increaseGlobalFlag(2);

	for( unsigned i = 0; i < numPi; i++ )
	{
		getGate(cut.oldCutIdList[i]) -> setCutFuncPi(i);
		getGate(cut.goldCutIdList[i]) -> setCutFuncPi(i);
	}

	unsigned gateCnt;
	gateCnt = 0;
	g_gol -> outputCutFunc(".cf_g.v", numPi, gateCnt);

	char Command[1024];
	NtkGate::increaseGlobalFlag(1);
	sprintf( Command, "miter .cf_o.v .cf_g.v" ); 
	Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "fraig" ); 
	Cmd_CommandExecute( pAbc, Command );

	// TODO check merge frontier
	// currently, we take the result
	// if no PI can reach PO
	for( unsigned i = 0; i < numPi; i++ )
		getGate(cut.goldCutIdList[i]) -> setFlag(0);
	unsigned unmergedCnt = 0;
	g_gol -> postOrderVisitCutFunc( unmergedCnt );

	if ( debug_f && unmergedCnt < gateCnt )
	{
		cout << "CF Fraig Result: " << unmergedCnt << " / " << gateCnt << " unmerged under " << setw(W_NAME) << g_gol->getName() << endl;
		/*
		for ( auto &id : cut.oldCutIdList )
			cout << setw(W_NAME) << getGate(id) -> _name;
		cout << endl;
		for ( auto &id : cut.goldCutIdList )
			cout << setw(W_NAME) << getGate(id) -> _name;
		*/
	}
	if ( debug_f ) cout << endl;

	if ( unmergedCnt < gateCnt ) 
	// more than 50% merge
	{
		return unmergedCnt;
	}
	else
	{
		return 0;
	}

}
bool
NtkMgr::fraigCutFunc( NGateList &revCut )
{

	// if ( debug_f ) cout << "Cut function fraig ... ";
	unsigned numPi = revCut.size()+_dupMerge.size();

	// clear up
	_cmg_heads.clear();
	for( size_t i = 0; i < _CFGates_rev.size(); i++ )
	{
		_CFGates_rev[i] -> _cmg_head = 0;
		_CFGates_rev[i] -> _cmg_list = 0;
		_CFGates_rev[i] -> _cmg_next = 0;
	}

	NtkGate::increaseGlobalFlag(2);

	for( size_t i = 0; i < _dupMerge.size(); i++ )
		_dupMerge[i] -> _mapping -> setCutFuncPi(i);
	for( size_t i = 0; i < revCut.size(); i++ )
		revCut[i] -> _mapping -> setCutFuncPi(i+_dupMerge.size());

	unsigned gateCnt = 0;
	_g_gol -> outputCutFunc(".cf_gol.v", numPi, gateCnt);

	NtkGate::increaseGlobalFlag(1);
	sprintf( Command, "miter .cf_rev.v .cf_gol.v" ); 
	Cmd_CommandExecute( pAbc, Command );
	sprintf( Command, "fraig" ); 
	Cmd_CommandExecute( pAbc, Command );

	for( size_t i = 0; i < _dupMerge.size(); i++ )
		_dupMerge[i] -> _mapping -> setFlag(0);
	for( size_t i = 0; i < revCut.size(); i++ )
		revCut[i] -> _mapping -> setFlag(0);
	unsigned unmergedCnt = 0;
	_g_gol -> postOrderVisitCutFunc( unmergedCnt );

	if ( debug_f ) cout << endl;

	if ( unmergedCnt < gateCnt )
	{
		// compare and record the resulting cut
		// TODO: only take equal checksize cut when level is diff
		if ( unmergedCnt <= _unmergedCnt && ( _matchCut.empty() || _matchCut.size() >= revCut.size() ) )
		{

			if ( debug_f && unmergedCnt < gateCnt )
			{
				cout << "Result: " << unmergedCnt << " / " << gateCnt << " unmerged under " << setw(W_NAME) << _g_gol->getName() << " (level = " << _level << ")" << endl;
				for( size_t i = 0; i < revCut.size(); i++ )
					cout << setw(W_NAME) << revCut[i] -> getName();
				cout << endl;
				for( size_t i = 0; i < revCut.size(); i++ )
					cout << setw(W_NAME) << revCut[i] -> _mapping -> getName();
			}
			NGateList newBwdCut, newBwdCutMapping;

			bool safe = true;

			NtkGate::increaseGlobalFlag(1);
			for( unsigned i = 0; i < revCut.size(); i++ )
			{
				revCut[i] -> _mapping -> _cmg_list = revCut[i];
			}
			_g_gol -> postOrderVisitCutFunc( newBwdCut, newBwdCutMapping );

			for( size_t i = 0; i < newBwdCut.size(); i++ )
			{
				if ( ! newBwdCut[i] -> checkLoopIfPatched( newBwdCutMapping[i]) ) safe = false;
			}

			if ( safe )
			{
				cout << "Safe bwd patch found" << endl;
				_level_res = _level;
				_matchCut = revCut;
				_matchCutMapping.clear();
				for( unsigned i = 0; i < revCut.size(); i++ )
				{
					_matchCutMapping.push_back(revCut[i]->_mapping);
				}
				_bwdCut = newBwdCut;
				_bwdCutMapping = newBwdCutMapping;
			}


		}
	}

	return false;

}
bool
NtkMgr::fraigCutFuncWithOneGateLeft( NGateList &revCut, NGateList &candList )
{

	NtkGate *gLeft, *gMapToConst = 0;

	// debug
	cout << "Rev Cut: " << endl;
	for( size_t i = 0; i < revCut.size(); i++ )
	{
		cout << setw(W_NAME) << revCut[i] -> getName();
	}
	cout << endl;

	for ( size_t i = 0; i < revCut.size(); i++  )
	{
		if ( ! revCut[i] -> _mapping )
		{
			gLeft = revCut[i];
		}
		else if ( revCut[i] -> _mapping -> getType() == NTK_GATE_CONST )
		{
			// currently, skip the ones with constant
			gMapToConst = revCut[i];
			return false;
		}
	}

	for( size_t i = 0; i < candList.size(); i++ )
	{
		// cout << "Random Pair: trying " << candList[i]->getName() << endl;
		if ( candList[i] -> getType() == NTK_GATE_CONST ) continue;

		gLeft -> _mapping = candList[i];
		fraigCutFunc( revCut );
	}

	gLeft -> _mapping = 0;
	return false;

}
void
NtkGate::postOrderVisitCutFunc( NGateList &bwdCut, NGateList &bwdCutMapping )
{

	if ( _flag == _globalFlag ) return;
	_flag = _globalFlag;

	if ( _mg_list ) return;

	if ( _cmg_list )
	{
		bwdCut.push_back(this);
		bwdCutMapping.push_back(_cmg_list);
		return;
	}
	else
	{
		for( size_t i = 0; i < _fanin.size(); i++ )
		{
			_fanin[i] -> postOrderVisitCutFunc( bwdCut, bwdCutMapping );
		}
	}
}

bool
NtkGate::checkLoopIfPatched( NtkGate *g_old )
{
	_globalFlagB += 1;
	for( size_t i = 0; i < _fanout.size(); i++ )
	{
		if ( _fanout[i] -> _flag != _globalFlag ) continue;
		_fanout[i] -> _flagB = _globalFlagB - 1;
	}
	g_old -> checkWontReachWithPatch();
	cout << "checking loop: done" << endl;
	for( size_t i = 0; i < _fanout.size(); i++ )
	{
		if ( _fanout[i] -> _flag != _globalFlag ) continue;
		if ( _fanout[i] -> _flagB == _globalFlagB ) return false;
	}
	return true;
}
void
NtkGate::checkWontReachWithPatch()
{
	if ( _flagB == _globalFlagB ) return;
	_flagB = _globalFlagB;
	cout << "checking loop: " << getId() << getName() << endl;
	for( size_t i = 0; i < _fanin.size(); i++ )
	{
		_fanin[i] -> checkWontReachWithPatch();
	}
	for( size_t i = 0; i < _recPair_fanin.size(); i++ )
	{
		if ( _recPair_fanin[i] ) _recPair_fanin[i] -> checkWontReachWithPatch();
	}
}
void
NtkMgr::identifyRecPairsOf2Gates(NtkGate* g_old, NtkGate* g_gol, patchRecord &record)
{
	NtkGatePQ rPairs_found;
	vector<string> tmp;


	// Warning: make sure they are in the same order
	// TODO: check the whole mg_list
	if ( ! g_old -> isMergedWith( g_gol ) )
	{
		// TODO: some PO use other PO as fanin
		//if ( g_old->getType() == NTK_GATE_BUF ) g_old = g_old -> _fanin[0];
		//if ( g_gol->getType() == NTK_GATE_BUF ) g_gol = g_gol -> _fanin[0];

		g_old -> _recPair_fanout.push_back(g_gol);
		_recPairList.push_back( g_old );

		if ( !(g_gol -> _mg_list) ) rPairs_found.push( g_old );

	} else {

		// debug
		if ( debug_c ) cout << "Merged output: " << g_old->getNameShort() << endl;

	}

	CutPair cutPairs;
	while( !rPairs_found.empty() )
	{
		g_old = rPairs_found.top();
		rPairs_found.pop();
		
		for( size_t k = 0, n = g_old -> _recPair_fanout.size(); k<n; k++ )
		{
			if ( g_old -> _mergeFlag ) continue;
			g_gol = g_old -> _recPair_fanout[k];
			if ( g_gol -> _mg_list ) continue;
			cout << "cut matching " << g_old -> getName() << " " << g_gol -> getName() << endl;
			cutPairs = cutMatching(g_old, g_gol);
			cout << "cut matching end " << endl;

			/*
			if ( g_old->getNameShort() == "n_753" )
			{
				cout << "test: change the paring of n_753\n";
				cout << getGate(cutPairs.goldCutIdList[0])->getNameShort() << " " << getGate(cutPairs.goldCutIdList[1])->getNameShort() << endl;
				swap( cutPairs.goldCutIdList[0], cutPairs.goldCutIdList[1] );
				cout << getGate(cutPairs.goldCutIdList[0])->getNameShort() << " " << getGate(cutPairs.goldCutIdList[1])->getNameShort();
			}
			*/

			IdList &oldList = cutPairs.oldCutIdList;
			IdList &golList = cutPairs.goldCutIdList;

			// mark cut function for rec net identification
			// WTF
			NtkGate::increaseGlobalFlag(2);
			for( size_t i = 0; i < oldList.size(); i++ )
			{
				getGate(oldList[i]) -> setFlag(-1);
			}
			cout << "postordertocut "<< endl;
			g_old -> postOrderVisitToCut();
			cout << "post end" << endl;
			
			for( size_t i = 0; i < oldList.size(); i ++ )
			{
				getGate(oldList[i]) -> addRecPairNRecord( getGate(golList[i]), rPairs_found, record);
			}

		}

	} 

	// cout << "# related gates on cut: " << relatedCutGatesCnt << endl;



	// sort by levels
	sort( _recPairList.begin(), _recPairList.end(), compareByLevel );
	cout << "sortbylevel end" << endl;

	cout  << endl << "rec pairs found: " << endl;
	for( size_t i = 0; i < _recPairList.size(); i++ )
	{
		if ( debug_c ) _recPairList[i] -> reportFanoutRecPair(true);
		else _recPairList[i] -> reportFanoutRecPair(false);
	}
	cout << endl;

}


void
NtkGate::addRecPairNRecord( NtkGate* g, NtkGatePQ &found , patchRecord &record)
{
	bool exists = false; 
	// if they are merged, continue
	if ( isMergedWith( g ) )
	{
		cout << "rec pair " << _name << " is merged with " << g->getName() << endl;
		return;
	}
	// check if the gate is already in the patchArr
	for(size_t i = 0; i < record.patchArr.size(); ++i)
	{
		if(record.patchArr[i] == getName())
		{
			exists = true;
			break;
		}
	}
	if( !exists )
	record.patchArr.push_back(getName());
	NtkGate *g2;
	unsigned recCnt_prev = _recCnt;
	vector<string> tmp;
	tmp.resize(_fanout.size());

	for ( size_t i = 0; i < _fanout.size(); i++ )
	{
		if ( _fanout[i] -> _flag != _globalFlag || _fanout[i] -> _mergeFlag ) continue;

		g2 = _fanout[i];
		
		tmp[i] = g -> getName();

		if ( g2 -> _recPair_fanin.empty() ) g2 -> _recPair_fanin.resize( g2->_fanin.size(), 0 );

		for( size_t j = 0; j < g2->_fanin.size(); j++ )
		{
			// TODO: what if there are the same 2 fanins
			if ( g2->_fanin[j] == this )
			{
				// keep the original rec
				if ( g2->_recPair_fanin[j] == 0 )
				{
					g2 -> _recPair_fanin[j] = g;
					_recCnt ++;
				}
				break;
			}
		}
	}
	if(!exists) record.patchRecFanout.push_back(tmp);
	

	// no new rec pair is needed
	if ( recCnt_prev == _recCnt && _fanout.size() != 0 ) return;

	// check if the rec pair already exist
	for ( size_t i = 0; i < _recPair_fanout.size(); i++  )
	{
		if ( _recPair_fanout[i] == g ) return;
	}
	if ( _recPair_fanout.empty() ) 
	{
		found.push( this );
		ntkMgr -> _recPairList.push_back(this);
	}
	_recPair_fanout.push_back(g);
	// for(size_t i = 0; i < _fanout.size(); ++i)
	// {
	// 	if(_fanout[i] == g) 
	// 	{
	// 		cout << "lolo" << g -> getName() << endl;
	// 		tmp[i] = (g -> getName());
	// 	}
	// }
}

void
NtkMgr::identifyRecPairs(patchRecord &record)
{
	NtkGatePQ rPairs_found;

	NtkGate* g_old;
	NtkGate* g_gol;

	for( size_t i = 0; i < _numPO; i++ )
	{
		g_old = getGate(_POList_old[i]);
		g_gol = getGate(_POList_gol[i]);

		// Warning: make sure they are in the same order
		// TODO: check the whole mg_list
		if ( ! g_old -> isMergedWith( g_gol ) )
		{
			// TODO: some PO use other PO as fanin
			//if ( g_old->getType() == NTK_GATE_BUF ) g_old = g_old -> _fanin[0];
			//if ( g_gol->getType() == NTK_GATE_BUF ) g_gol = g_gol -> _fanin[0];

			g_old -> _recPair_fanout.push_back(g_gol);
			_recPairList.push_back( g_old );

			if ( g_gol -> _mg_list ) continue;
			rPairs_found.push( g_old );

		} else {

			// debug
			if ( debug_c ) cout << "Merged output: " << g_old->getNameShort() << endl;

		}

	}

	CutPair cutPairs;

	while( !rPairs_found.empty() )
	{
		g_old = rPairs_found.top();
		rPairs_found.pop();
		
		for( size_t k = 0, n = g_old -> _recPair_fanout.size(); k<n; k++ )
		{
			if ( g_old -> _mergeFlag ) continue;
			g_gol = g_old -> _recPair_fanout[k];
			if ( g_gol -> _mg_list ) continue;

			cutPairs = cutMatching(g_old, g_gol);

			/*
			if ( g_old->getNameShort() == "n_753" )
			{
				cout << "test: change the paring of n_753\n";
				cout << getGate(cutPairs.goldCutIdList[0])->getNameShort() << " " << getGate(cutPairs.goldCutIdList[1])->getNameShort() << endl;
				swap( cutPairs.goldCutIdList[0], cutPairs.goldCutIdList[1] );
				cout << getGate(cutPairs.goldCutIdList[0])->getNameShort() << " " << getGate(cutPairs.goldCutIdList[1])->getNameShort();
			}
			*/

			IdList &oldList = cutPairs.oldCutIdList;
			IdList &golList = cutPairs.goldCutIdList;

			// mark cut function for rec net identification
			// WTF
			NtkGate::increaseGlobalFlag(2);
			for( size_t i = 0; i < oldList.size(); i++ )
			{
				getGate(oldList[i]) -> setFlag(-1);
			}
			g_old -> postOrderVisitToCut();
			
			for( size_t i = 0; i < oldList.size(); i ++ )
			{
				getGate(oldList[i]) -> addRecPairNRecord( getGate(golList[i]), rPairs_found , record);
			}

		}

	} 

	// cout << "# related gates on cut: " << relatedCutGatesCnt << endl;


	// check the backward patches
	for( size_t i = 0; i < _netList_old.size(); i++ )
		getGate( _netList_old[i] ) -> _numBwdPatch = 0;

	_bwdPatchMap.clear();

	for( size_t i = 0; i < _bwdPatchList.size(); i++ )
	{
		NGateList &l = _bwdPatchList[i] -> _recPair_fanout;
		for( size_t j = 0; j < l.size(); j++ )
		{
			if ( _bwdPatchMap[l[j]].empty() && l[j]->_recPair_fanout.empty() ) _recPairList.push_back( l[j] );
			_bwdPatchMap[l[j]].push_back( _bwdPatchList[i] );
			l[j] -> _numBwdPatch ++;
			//cout << "new bwd patch: " << _bwdPatchList[i]->getName() << " -> " << l[j] -> getName() << endl;
		}
	}

	// sort by levels
	sort( _recPairList.begin(), _recPairList.end(), compareByLevel );

	cout  << endl << "rec pairs found: " << endl;
	for( size_t i = 0; i < _recPairList.size(); i++ )
	{
		if ( debug_c ) _recPairList[i] -> reportFanoutRecPair(true);
		else _recPairList[i] -> reportFanoutRecPair(false);
	}
	cout << endl;

}

void
NtkMgr::checkBwdPatches()
{

	// check the backward patches
	for( size_t i = 0; i < _netList_old.size(); i++ )
		getGate( _netList_old[i] ) -> _numBwdPatch = 0;

	_bwdPatchMap.clear();

	for( size_t i = 0; i < _bwdPatchList.size(); i++ )
	{
		NGateList &l = _bwdPatchList[i] -> _recPair_fanout;
		for( size_t j = 0; j < l.size(); j++ )
		{
			if ( _bwdPatchMap[l[j]].empty() && l[j]->_recPair_fanout.empty() ) _recPairList.push_back( l[j] );
			_bwdPatchMap[l[j]].push_back( _bwdPatchList[i] );
			l[j] -> _numBwdPatch ++;
			//cout << "new bwd patch: " << _bwdPatchList[i]->getName() << " -> " << l[j] -> getName() << endl;
		}
	}
}
