/****************************************************************************
  FileName     [ cirDef.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic data or var for cir package ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_DEF_H
#define CIR_DEF_H

#include <vector>

using namespace std;
#define MAX_UINT 0x80000000

// TODO: define your own typedef or enum
enum CirGateType {
  UNDEF_GATE = 0,
  PI_GATE    = 1,
  PO_GATE    = 2,
  AIG_GATE   = 3,
  CONST_GATE = 4,

  TOT_GATE
};
enum CirType {
	CIR_OLD,
	CIR_GOL,
	CIR_MITER,
	CIR_SEL,
	CIR_CF,
	CIR_TOTAL
};

class CirGateV;
class CirGate;
class AigGate;
class POGate;
class PIGate;
class ConstGate;
class UndefGate;
class CirMgr;
class SatSolver;

typedef vector<CirGate*> 	CirGateList;
typedef vector<unsigned> 	IdList;
typedef vector<CirGateV> 	PinList;

#endif // CIR_DEF_H
