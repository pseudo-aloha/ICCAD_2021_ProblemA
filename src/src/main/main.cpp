/****************************************************************************
  FileName     [ main.cpp ]
  PackageName  [ main ]
  Synopsis     [ Define main() ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2007-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cstdarg>

#include "../util/util.h"
#include "../ntk/ntkMgr.h"
#include "../cir/cirMgr.h"
#include "../ntk/ntkGate.h"


extern "C"
{
// procedures to start and stop the ABC framework
// (should be called before and after the ABC procedures are called)
void   Abc_Start();
void   Abc_Stop();

// procedures to get the ABC framework and execute commands in it
typedef struct Abc_Frame_t_ Abc_Frame_t;

Abc_Frame_t * Abc_FrameGetGlobalFrame();
int    Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand );
}

using namespace std;

// time threshold
int time_threshold = 3600;
int total_time_threshold = 3600;

//----------------------------------------------------------------------
//    Global cmd Manager
//----------------------------------------------------------------------

NtkMgr* 	ntkMgr = 0;
CirMgr*		aigMiter = 0;
CirMgr*		aigOld = 0;
CirMgr*		aigGol = 0;
CirMgr*		aigSel = 0;
SatSolver*	solver = 0;
Abc_Frame_t * pAbc = 0;


// debug flag
bool debug_c;	// cut matching
bool debug_n; 	// netlist
bool debug_s;	// selector
bool debug_f;	// cut function fraig
bool debug_e;	// cut checking (marking)
bool debug_r;	// cut function fraig
bool debug_a;   // try to patch a low level bug first
bool debug_p;	// patch
bool debug_t;	// try all methods

//prepatch record
patchRecord globalPrePatch;

// count total time
Timer timer_total;

int
main(int argc, char** argv)
{
	timer_total.start();
	// setup
	Abc_Start();
	pAbc = Abc_FrameGetGlobalFrame();

	/*
	// oficial input format
	assert(argc==5);
	char* fname_old = argv[3];
	char* fname_gol = argv[2];
	char* fname_patch = argv[4];
	*/
	char fname_old[128], fname_gol[128];
	sprintf( fname_old, "%s/g1.v", argv[1]);
	sprintf( fname_gol, "%s/r2.v", argv[1]);
	char fname_patch[100] = "patch.v";
	char fname_patched[100] = "patched.v";
	// simple input format
	if (argc<5)
	{
		cout << "usage: ./eco <input> <# of sim> <sim thre> <lv thre> [debug options]" << endl;
		return 0;
	}

	// show arguments
	for ( int i = 0; i < argc; i++ )
		cout << argv[i] << " ";
	cout << endl;

	int main_sim_num = stoi(string(argv[2]));
	float main_sim_thre = stof(string(argv[3]));
	int main_lv_thre = stoi(string(argv[4]));
	int main_minCS_init = 4;

	// check debug flag
	string options =  "";
	if (argc==6) options = argv[5];
	debug_c = ( options.find('c') != string::npos );
	debug_n = ( options.find('n') != string::npos );
	debug_s = ( options.find('s') != string::npos );
	debug_p = ( options.find('p') != string::npos );
	debug_f = ( options.find('f') != string::npos );
	debug_e = ( options.find('e') != string::npos );
	debug_r = ( options.find('r') != string::npos );
	debug_a = ( options.find('a') != string::npos );
	debug_t = ( options.find('t') != string::npos );

	// initialize solver
	ntkMgr = new NtkMgr();
	solver = new SatSolver();
	solver -> initialize();
	//adde by yen_ju, construct the lookup table for simulation
	aigSel -> setGlobalSimLookupTable();
	ntkMgr -> attachFiles( fname_old, fname_gol, fname_patch );


	// start
	int bestCost = INT_MAX;

	if ( !debug_t )
	{

		ntkMgr -> eco( bestCost, main_sim_num, main_sim_thre, main_lv_thre );
	}
	else
	{
		ntkMgr -> tryDifferentParas( bestCost , globalPrePatch );
		cout << endl << endl;
		cout << "best cost = " << bestCost << endl;

	}


	// end
	reportTimers();

	// TODO: reiterate

	// terminate
	Abc_Stop();
	delete ntkMgr;
	delete aigMiter;
	delete aigOld;
	delete aigGol;
	delete aigSel;
	delete solver;

   	return 0;
}