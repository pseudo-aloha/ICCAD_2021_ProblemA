#ifndef NEWCMD_C
#define NEWCMD_C

// #include "newCmd.h"
#include "base/io/ioAbc.h"
#include "base/abc/abc.h"
#include "base/main/mainInt.h"

int eco_flag_parse = 0;
int eco_flag_strash = 0;
int eco_flag_fraig = 0;

int
newCmdEcoSetFlag( Abc_Frame_t * pAbc, int argc, char ** argv)
{
    int fVerbose, c;
    int fCheck, fBarBufs;
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk( pAbc );
    char * pFileName;

	fCheck = 1;
    fVerbose = 0;
    fBarBufs = 0;

	eco_flag_parse = 0;
	eco_flag_strash = 0;
	eco_flag_fraig = 0;

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "psf") ) != EOF)
    {
        switch ( c )
        {
			case 'p':	
				eco_flag_parse = 1;
				printf("ECO: flag parse = 1\n");
				break;
			case 's':
				eco_flag_strash = 1;
				printf("ECO: flag strash = 1\n");
				break;
			case 'f':
				eco_flag_fraig = 1;
				printf("ECO: flag fraig = 1\n");
				break;
            default:
                break;
        }
    }

    return 0;
}

int
newCmdEcoNet2PO( Abc_Frame_t * pAbc, int argc, char ** argv)
{
    int fVerbose, c;
    int fCheck, fBarBufs;
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk( pAbc );
    char * pFileName;

	fCheck = 1;
    fVerbose = 0;
    fBarBufs = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "hg") ) != EOF)
    {
        switch ( c )
        {
            // case 'h':
            //     goto usage;
			case 'g':
				fVerbose = 1;
				break;
            default:
                break;
        }
    }

    //read verilog file
    //pull internal signals to po
    int i;
    Abc_Obj_t* 	pNode;
    Abc_Obj_t* 	pNet;
    char*		str;

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
		str = Abc_ObjName( pNode );

		if ( Abc_ObjFaninNum(pNode) == 0 ) 
		{
			if ( !strcmp( Abc_ObjName(pNode), "1'b0" ) || !strcmp( Abc_ObjName(pNode), "1'b1") ) continue;
			if ( !Abc_ObjFanoutNum(pNode) ) continue;
			str = Abc_ObjName( Abc_ObjFanout0(pNode) );
		}

		pNet = Abc_NtkCreatePo( pNtk );
		Abc_ObjAddFanin( pNet, pNode );
		Abc_ObjAssignName( pNet, str, fVerbose?"_G":"_O" );

		
    //printf( "create node after miter (old): %x\t%x\t(%s)\n", pNode, pNode->pCopy, str);
    }

    return 0;
}

int
newCmdEcoShowId( Abc_Frame_t * pAbc, int argc, char ** argv)
{
    int fVerbose, c;
    int fCheck, fBarBufs;
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk( pAbc );
    char * pFileName;

	fCheck = 1;
    fVerbose = 0;
    fBarBufs = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h") ) != EOF)
    {
        switch ( c )
        {
            case 'h':
				break;
            default:
                break;
        }
    }

    int i;
    Abc_Obj_t* 	pNode;
    Abc_Obj_t* 	pNet;
    char*		str;

    Abc_NtkForEachCi( pNtk, pNode, i )
    {
		printf( "%5d  %s\n", Abc_ObjId(pNode), Abc_ObjName(pNode) ) ;
    }
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
		printf( "%5d  %s\n", Abc_ObjId(pNode), Abc_ObjName(pNode) ) ;
    }

    return 0;
}

void
newCmd_Init( Abc_Frame_t * pAbc)
{
    Cmd_CommandAdd( pAbc, "Synthesys", "eco_net2po", newCmdEcoNet2PO, 1);
    Cmd_CommandAdd( pAbc, "Synthesys", "eco_setFlag", newCmdEcoSetFlag, 1);
    Cmd_CommandAdd( pAbc, "Synthesys", "eco_showId", newCmdEcoShowId, 1);
}
void
newCmd_End( Abc_Frame_t * pAbc)
{

}
#endif
