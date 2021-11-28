#include <iomanip>

#include "ntkDef.h"
#include "ntkMgr.h"
#include "ntkGate.h"
#include "cirMgr.h"
#include "sat.h"
#include "util.h"

using namespace std;

extern SatSolver* solver;
extern CirMgr*	aigOld;
extern CirMgr*	aigGol;
extern CirMgr* 	aigMiter;

void
NtkMgr::checkCutFuncTest()
{
	// Used for test3
	cout << "Start cut function test" << endl;

	NtkGate* g_old = findGateByName("l_O");;
	NtkGate* g_gol = findGateByName("l_G");;

	CutPair cut;

	cut.checkSize = 2;
	cut.oldCutIdList.push_back( findGateByName("f1_O")->getId() );
	cut.oldCutIdList.push_back( findGateByName("g1_O")->getId() );
	cut.oldCutIdList.push_back( findGateByName("e_O")->getId() );
	cut.goldCutIdList.push_back( findGateByName("a_O")->getId() );
	cut.goldCutIdList.push_back( findGateByName("b_O")->getId() );
	cut.goldCutIdList.push_back( findGateByName("i1_O")->getId() );

	// Todo: check

}
void
NtkMgr::cutTest()
{
}

// a test
void
NtkMgr::satTest()
{
	cout << "start satTest " << endl;

	solver->reset();
	CirGate::increaseGlobalFlag();

	NtkGate* g1;
	NtkGate* g2;

	g1 = findGateByName( "less_O" );
	g2 = findGateByName( "less_G" );			// eq
	//g2 = findGateByName( "n_561_GOL"); 		// not eq
	g1->reportGate();
	g2->reportGate();

	g1->satAddClauseRec();
	g2->satAddClauseRec();

	Var v = satAddMiter( g1, g2 );
	solver->printStats();
	solver->assumeProperty(v, true);

	bool res = solver->assumpSolve();
	if (res) cout << "SAT" << endl;
	else cout << "UNSAT" << endl;
	solver -> printStats();

}

void
NtkMgr::simTest()
{
	cout << "Start sim test\n";
	NtkGate *g1, *g2, *g3, *g4, *g5;
	g1 = findGateByName( "n23_O" );
	g2 = findGateByName( "n26_O" );			
	g3 = findGateByName( "n16_G" );		
	g4 = findGateByName( "n17_G" );		
	g5 = findGateByName( "n14_G" );		

	IdList oldList, golList;
	oldList.push_back(g1->getId());
	oldList.push_back(g2->getId());
	golList.push_back(g3->getId());
	golList.push_back(g4->getId());
	golList.push_back(g5->getId());

	vector<SimEntry> vec;
	getSimPairs( oldList, golList ,0.7 , vec );

	for( size_t i=0; i<vec.size(); i++)
	{
		//cout << getGate(vec[i].old)->getName() <<  '\t' << getGate(vec[i].gol)->getName() << '\t' << vec[i].sim << endl;
	}

}