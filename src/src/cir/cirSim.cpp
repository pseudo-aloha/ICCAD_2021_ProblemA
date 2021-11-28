/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "ntkGate.h"
#include "../util/util.h"
#include <cstring>
#include <algorithm>

using namespace std;

extern void printBits( size_t n );

extern CirMgr* aigGol;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
string pStr;
size_t pIdx;
// the lookup table used for global simulation
vector<int> globalSimLookupTable;

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

struct RnPatternGen {
  RnPatternGen() { srand(0); }
  size_t operator() () {
    size_t num = rand();
    num = ( (num << 31) + rand() );
    num = ( (num << 31) + rand()%4 );
    return num;
  }
} rnPatternGen;

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void CirMgr::randomSim( unsigned repeat ) {

	unsigned numGoodBit = 0;

	cout << "Perform " << repeat << "*64 simulations." << endl;
	_numSim += repeat;

	while( repeat -- )
	{
		_gateList[0]->_sim.push_back(0);

		for( size_t i = 0; i < _PIList.size(); i++ )
		{
			_gateList[_PIList[i]] -> _sim.push_back(rnPatternGen());
		}
		simulate();

		CirGate *miter = getPO(0);
		size_t sim_miter = ~(miter->_sim.back());
		for (unsigned i = 0; i < 64; ++i)
			if (((sim_miter >> i) % 2))
			{
				++numGoodBit;
			}

	}

	_numGoodBit += numGoodBit;
}


float CirMgr::cosineSimilarity( NtkGate* g1, NtkGate* g2 )
{

	if ( _numGoodBit == 0) { return 0; }

	// check if it is a good vector (for miter PO)
	CirGate *miter = getPO(0);
	unsigned counter = 0;
	size_t sim1, sim2, mask, same;

	for ( size_t k = 0; k < _numSim; k++ )
	{

		sim1 = g1 -> getSim(k);
		sim2 = g2 -> getSim(k);
		mask = miter -> _sim[k];
		same = (sim1 ^ ~sim2) & (~mask);

		for (unsigned i = 0; i < 8; ++i)
		{
			counter += globalSimLookupTable[((same >> (8 * i)) & 255)];
		}
		// for (unsigned i = 0; i < 64; ++i)
		// {
		// 	if ( (same >> i) % 2 )
		// 	{
		// 		++counter;
		// 	}
		// }

	}

	assert(counter <= _numGoodBit );

	float cosine_value = (float) counter / _numGoodBit;

	//return cosine_value > 0.5 ? cosine_value-0.5 : 0.5-cosine_value ;
	return cosine_value;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/

void CirMgr::simulate() {
  for( size_t i = 0; i < _netList.size(); i++ )
  {
	  _gateList[_netList[i]] -> simulate();
  }
}


// added by Hugo
float CirMgr::structuralSimilarity(NtkGate* g1, NtkGate* g2){
  // traverse g1 fanin cone, calculate its merged gate amount and temporarily store its merged gate pointer
  size_t mg_amount = 0;

  int traverse_level = 30;
  NtkGate::_globalFlagB += 2;
  int g1_level_limit = (g1->getLevel() - traverse_level > 0) ? (g1->getLevel() - traverse_level) : 0;
  ntkMgr->MarkFaninConeTraverse(g1, g1_level_limit, mg_amount);
  // traverse g2 fanin cone, calculate its merged gate amount and check the overlapped merged gate with g1
  int g2_level_limit = (g2->getLevel() - traverse_level > 0) ? (g2->getLevel() - traverse_level) : 0;
  // old store old mg gate, gold store gold mg gate
  // traverse gold_mg_gate->_mg_list->...->_mg_next, to see if (gate)->_mergeFlag == true
  // calculate the same merged gate
  int same_merged = 0; 
  ntkMgr->CountRepeatedMgGate(g2, g2_level_limit, same_merged, mg_amount);
  
	// for (vector<NtkGate*>::const_iterator nIterator = g1_mgList.begin(); nIterator != g1_mgList.end(); nIterator++)
	// {
	// 	if(std::find(g2_mgList.begin(), g2_mgList.end(), *nIterator) != g2_mgList.end())
	// 		++same_merged;
	// }

  // calculate the structural similarity
  float struct_value;
  struct_value = ( (float)(same_merged+1) / ((mg_amount >> 1)+1.5) );

//   cout << "total_merged = " << mg_amount / 2<< endl;
//   cout << "same_merged = " << same_merged << endl;
  cout << "same merged gate = " << same_merged << endl;
  return same_merged;
}

void CirMgr::setGlobalSimLookupTable()
{
	size_t tmp = 0;
	int count = 0;
	for(size_t i = 0; i < 256; ++i)
	{
		count = 0;
		tmp = i;
		// cout << tmp << endl;
		for(size_t j = 0; j < 64; ++j)
		{
			count += (1 & tmp);
			tmp >>= 1;
		}
		globalSimLookupTable.push_back(count);
	}
}