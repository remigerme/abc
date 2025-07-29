/**CFile****************************************************************

  FileName    [darCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Core of the rewriting package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darCore.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "darInt.h"
#include "certificate.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// iterator over the nodes in the topological order
#define Aig_ManForEachNodeInOrder( p, pObj )                                    \
    for ( assert(p->pOrderData), p->iPrev = 0, p->iNext = p->pOrderData[1];     \
          p->iNext && (((pObj) = Aig_ManObj(p, p->iNext)), 1);                  \
          p->iNext = p->pOrderData[2*p->iPrev+1] )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the structure with default assignment of parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dar_ManDefaultRwrParams( Dar_RwrPar_t * pPars )
{
    memset( pPars, 0, sizeof(Dar_RwrPar_t) );
    pPars->nCutsMax     =  8; // 8
    pPars->nSubgMax     =  5; // 5 is a "magic number"
    pPars->nMinSaved    =  1;
    pPars->fFanout      =  1;
    pPars->fUpdateLevel =  0;
    pPars->fUseZeros    =  0;
    pPars->fPower       =  0;
    pPars->fRecycle     =  1;
    pPars->fVerbose     =  0;
    pPars->fVeryVerbose =  0;
}

#define MAX_VAL 10

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_ManRewrite( Aig_Man_t * pAig, Dar_RwrPar_t * pPars)
{
    extern Vec_Int_t * Saig_ManComputeSwitchProbs( Aig_Man_t * p, int nFrames, int nPref, int fProbOne );
    Dar_Man_t * p;
//    Bar_Progress_t * pProgress;
    Dar_Cut_t * pCut;
    Aig_Obj_t * pObj, * pObjNew;
    int i, k, nNodesOld, nNodeBefore, nNodeAfter, Required;
    abctime clk = 0, clkStart;
    int Counter = 0;
    int nMffcSize;//, nMffcGains[MAX_VAL+1][MAX_VAL+1] = {{0}};
    if ( pPars->fUseZeros )
        pPars->nMinSaved = 0;
    // prepare the library
    Dar_LibPrepare( pPars->nSubgMax ); 
    // create rewriting manager
    p = Dar_ManStart( pAig, pPars );
    if ( pPars->fPower )
        pAig->vProbs = Saig_ManComputeSwitchProbs( pAig, 48, 16, 1 );
    // remove dangling nodes
    Aig_ManCleanup( pAig );
    // if updating levels is requested, start fanout and timing
    if ( p->pPars->fFanout )
        Aig_ManFanoutStart( pAig );
    if ( p->pPars->fUpdateLevel )
        Aig_ManStartReverseLevels( pAig, 0 );
    // set elementary cuts for the PIs
//    Dar_ManCutsStart( p );
    // resynthesize each node once
    clkStart = Abc_Clock();
    p->nNodesInit = Aig_ManNodeNum(pAig);
    nNodesOld = Vec_PtrSize( pAig->vObjs );

//    pProgress = Bar_ProgressStart( stdout, nNodesOld );
    Aig_ManForEachObj( pAig, pObj, i )
//    pProgress = Bar_ProgressStart( stdout, 100 );
//    Aig_ManOrderStart( pAig );
//    Aig_ManForEachNodeInOrder( pAig, pObj )
    {
        if ( pAig->Time2Quit && !(i & 256) && Abc_Clock() > pAig->Time2Quit )
            break;
//        Bar_ProgressUpdate( pProgress, 100*pAig->nAndPrev/pAig->nAndTotal, NULL );
//        Bar_ProgressUpdate( pProgress, i, NULL );
        if ( !Aig_ObjIsNode(pObj) )
            continue;
        if ( i > nNodesOld )
//        if ( p->pPars->fUseZeros && i > nNodesOld )
            break;
        if ( pPars->fRecycle && ++Counter % 50000 == 0 && Aig_DagSize(pObj) < Vec_PtrSize(p->vCutNodes)/100 )
        {
//            printf( "Counter = %7d.  Node = %7d.  Dag = %5d. Vec = %5d.\n", 
//                Counter, i, Aig_DagSize(pObj), Vec_PtrSize(p->vCutNodes) );
//            fflush( stdout );
            Dar_ManCutsRestart( p, pObj );
        }

        // consider freeing the cuts
//        if ( (i & 0xFFF) == 0 && Aig_MmFixedReadMemUsage(p->pMemCuts)/(1<<20) > 100 )
//            Dar_ManCutsStart( p );

        // compute cuts for the node
        p->nNodesTried++;
clk = Abc_Clock();
        Dar_ObjSetCuts( pObj, NULL );
        Dar_ObjComputeCuts_rec( p, pObj );
p->timeCuts += Abc_Clock() - clk;

        //@ Old version - just adding a dummy mutation
        Vec_Ptr_t *mutations = Vec_PtrAlloc(1);
        CertifIdMan_t * certif_man = NULL;

        // check if there is a trivial cut
        Dar_ObjForEachCut( pObj, pCut, k )
            if ( pCut->nLeaves == 0 || (pCut->nLeaves == 1 && pCut->pLeaves[0] != pObj->Id && Aig_ManObj(p->pAig, pCut->pLeaves[0])) )
                break;
        if ( k < (int)pObj->nCuts )
        {
            assert( pCut->nLeaves < 2 );
            if ( pCut->nLeaves == 0 ) // replace by constant
            {
                assert( pCut->uTruth == 0 || pCut->uTruth == 0xFFFF );
                pObjNew = Aig_NotCond( Aig_ManConst1(p->pAig), pCut->uTruth==0 );
            }
            else
            {
                assert( pCut->uTruth == 0xAAAA || pCut->uTruth == 0x5555 );
                pObjNew = Aig_NotCond( Aig_ManObj(p->pAig, pCut->pLeaves[0]), pCut->uTruth==0x5555 );
            }
            // remove the old cuts
            Dar_ObjSetCuts( pObj, NULL );
            // replace the node
            Aig_ObjReplace( pAig, pObj, pObjNew, p->pPars->fUpdateLevel );
            continue;
        }

        // evaluate the cuts
        p->GainBest = -1;
        nMffcSize   = -1;
        Required    = pAig->vLevelR? Aig_ObjRequiredLevel(pAig, pObj) : ABC_INFINITY;
        Dar_ObjForEachCut( pObj, pCut, k )
        {
            int nLeavesOld = pCut->nLeaves;
            if ( pCut->nLeaves == 3 )
                pCut->pLeaves[pCut->nLeaves++] = 0;
            Dar_LibEval( p, pObj, pCut, Required, &nMffcSize );
            pCut->nLeaves = nLeavesOld; 
        }
        // check the best gain
        //if ( !(p->GainBest > 0 || (p->GainBest == 0 && p->pPars->fUseZeros)) )
        if ( p->GainBest < p->pPars->nMinSaved )
        {
//            Aig_ObjOrderAdvance( pAig );
            continue;
        }
//        nMffcGains[p->GainBest < MAX_VAL ? p->GainBest : MAX_VAL][nMffcSize < MAX_VAL ? nMffcSize : MAX_VAL]++;
        // remove the old cuts
        Dar_ObjSetCuts( pObj, NULL );
        // if we end up here, a rewriting step is accepted
        nNodeBefore = Aig_ManNodeNum( pAig );
        pObjNew = Dar_LibBuildBest( p , mutations, certif_man ); // pObjNew can be complemented!
        pObjNew = Aig_NotCond( pObjNew, Aig_ObjPhaseReal(pObjNew) ^ pObj->fPhase );
        assert( (int)Aig_Regular(pObjNew)->Level <= Required );
        // replace the node
        Aig_ObjReplace( pAig, pObj, pObjNew, p->pPars->fUpdateLevel );
        // compare the gains
        nNodeAfter = Aig_ManNodeNum( pAig );
        assert( p->GainBest <= nNodeBefore - nNodeAfter );
        // count gains of this class
        p->ClassGains[p->ClassBest] += nNodeBefore - nNodeAfter;
    }
//    Aig_ManOrderStop( pAig );
/*
    printf( "Distribution of gain (row) by MFFC size (column) %s 0-costs:\n", p->pPars->fUseZeros? "with":"without" );
    for ( k = 0; k <= MAX_VAL; k++ )
        printf( "<%4d> ", k );
    printf( "\n" );
    for ( i = 0; i <= MAX_VAL; i++ )
    {
        for ( k = 0; k <= MAX_VAL; k++ )
            printf( "%6d ", nMffcGains[i][k] );
        printf( "\n" );
    }
*/

p->timeTotal = Abc_Clock() - clkStart;
p->timeOther = p->timeTotal - p->timeCuts - p->timeEval;

//    Bar_ProgressStop( pProgress );
    Dar_ManCutsFree( p );
    // put the nodes into the DFS order and reassign their IDs
//    Aig_NtkReassignIds( p );
    // fix the levels
//    Aig_ManVerifyLevel( pAig );
    if ( p->pPars->fFanout )
        Aig_ManFanoutStop( pAig );
    if ( p->pPars->fUpdateLevel )
    {
//        Aig_ManVerifyReverseLevel( pAig );
        Aig_ManStopReverseLevels( pAig );
    }
    if ( pAig->vProbs )
    {
        Vec_IntFree( pAig->vProbs );
        pAig->vProbs = NULL;
    }
    // stop the rewriting manager
    Dar_ManStop( p );
    Aig_ManCheckPhase( pAig );
    // check
    if ( !Aig_ManCheck( pAig ) )
    {
        printf( "Aig_ManRewrite: The network check has failed.\n" );
        return 0;
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_ManRewriteCertificates( Aig_Man_t * pAig, Dar_RwrPar_t * pPars, Vec_Ptr_t * certificates)
{
    extern Vec_Int_t * Saig_ManComputeSwitchProbs( Aig_Man_t * p, int nFrames, int nPref, int fProbOne );
    Dar_Man_t * p;
//    Bar_Progress_t * pProgress;
    Dar_Cut_t * pCut;
    Aig_Obj_t * pObj, * pObjNew;
    int i, k, nNodesOld, nNodeBefore, nNodeAfter, Required;
    abctime clk = 0, clkStart;
    int Counter = 0;
    int nMffcSize;//, nMffcGains[MAX_VAL+1][MAX_VAL+1] = {{0}};
    if ( pPars->fUseZeros )
        pPars->nMinSaved = 0;
    // prepare the library
    Dar_LibPrepare( pPars->nSubgMax ); 
    // create rewriting manager
    p = Dar_ManStart( pAig, pPars );
    if ( pPars->fPower )
        pAig->vProbs = Saig_ManComputeSwitchProbs( pAig, 48, 16, 1 );
    // remove dangling nodes
    Aig_ManCleanup( pAig );
    // if updating levels is requested, start fanout and timing
    if ( p->pPars->fFanout )
        Aig_ManFanoutStart( pAig );
    if ( p->pPars->fUpdateLevel )
        Aig_ManStartReverseLevels( pAig, 0 );
    // set elementary cuts for the PIs
//    Dar_ManCutsStart( p );
    // resynthesize each node once
    clkStart = Abc_Clock();
    p->nNodesInit = Aig_ManNodeNum(pAig);
    nNodesOld = Vec_PtrSize( pAig->vObjs );

    //@ Creating vectors for the certificate.
    Vec_Ptr_t *mutations = Vec_PtrAlloc(500);
    Vec_Ptr_t *hints = Vec_PtrAlloc(50);
    CertifIdMan_t *certif_man = new_certif_id_man(pAig);

//    pProgress = Bar_ProgressStart( stdout, nNodesOld );
    Aig_ManForEachObj( pAig, pObj, i )
//    pProgress = Bar_ProgressStart( stdout, 100 );
//    Aig_ManOrderStart( pAig );
//    Aig_ManForEachNodeInOrder( pAig, pObj )
    {
        if ( pAig->Time2Quit && !(i & 256) && Abc_Clock() > pAig->Time2Quit )
            break;
//        Bar_ProgressUpdate( pProgress, 100*pAig->nAndPrev/pAig->nAndTotal, NULL );
//        Bar_ProgressUpdate( pProgress, i, NULL );
        if ( !Aig_ObjIsNode(pObj) )
            continue;
        if ( i > nNodesOld )
//        if ( p->pPars->fUseZeros && i > nNodesOld )
            break;
        if ( pPars->fRecycle && ++Counter % 50000 == 0 && Aig_DagSize(pObj) < Vec_PtrSize(p->vCutNodes)/100 )
        {
//            printf( "Counter = %7d.  Node = %7d.  Dag = %5d. Vec = %5d.\n", 
//                Counter, i, Aig_DagSize(pObj), Vec_PtrSize(p->vCutNodes) );
//            fflush( stdout );
            Dar_ManCutsRestart( p, pObj );
        }

        // consider freeing the cuts
//        if ( (i & 0xFFF) == 0 && Aig_MmFixedReadMemUsage(p->pMemCuts)/(1<<20) > 100 )
//            Dar_ManCutsStart( p );

        // compute cuts for the node
        p->nNodesTried++;
clk = Abc_Clock();
        Dar_ObjSetCuts( pObj, NULL );
        Dar_ObjComputeCuts_rec( p, pObj );
p->timeCuts += Abc_Clock() - clk;

        // check if there is a trivial cut
        Dar_ObjForEachCut( pObj, pCut, k )
            if ( pCut->nLeaves == 0 || (pCut->nLeaves == 1 && pCut->pLeaves[0] != pObj->Id && Aig_ManObj(p->pAig, pCut->pLeaves[0])) )
                break;
        if ( k < (int)pObj->nCuts )
        {
            assert( pCut->nLeaves < 2 );
            if ( pCut->nLeaves == 0 ) // replace by constant
            {
                assert( pCut->uTruth == 0 || pCut->uTruth == 0xFFFF );
                pObjNew = Aig_NotCond( Aig_ManConst1(p->pAig), pCut->uTruth==0 );
            }
            else
            {
                assert( pCut->uTruth == 0xAAAA || pCut->uTruth == 0x5555 );
                pObjNew = Aig_NotCond( Aig_ManObj(p->pAig, pCut->pLeaves[0]), pCut->uTruth==0x5555 );
            }
            //@ Saving mutation.
            //@ TODO : IS COMPLEMENT CORRECT FOR CONSTANT NODE ?
            int old_id = pObj->CertifId;
            int new_id = Aig_Regular(pObjNew)->CertifId;
            int complement = Aig_IsComplement(pObjNew);
            Mutation_t *mut = new_mutation_replace(old_id, new_id, complement);
            Vec_PtrPush(mutations, mut);

            printf("replacing %d(%d) with %d(%d) - compl:%d\n", old_id, pObj->Id,  new_id, Aig_Regular(pObjNew)->Id, complement);
            printf("refs:%d\n", Aig_ObjRefs(Aig_Regular(pObjNew)));


            // remove the old cuts
            Dar_ObjSetCuts( pObj, NULL );
            // replace the node
            Aig_ObjReplace( pAig, pObj, pObjNew, p->pPars->fUpdateLevel );

            check_certif_id(pAig);
            continue;
        }

        // evaluate the cuts
        p->GainBest = -1;
        nMffcSize   = -1;
        Required    = pAig->vLevelR? Aig_ObjRequiredLevel(pAig, pObj) : ABC_INFINITY;
        Dar_ObjForEachCut( pObj, pCut, k )
        {
            int nLeavesOld = pCut->nLeaves;
            if ( pCut->nLeaves == 3 )
                pCut->pLeaves[pCut->nLeaves++] = 0;
            Dar_LibEval( p, pObj, pCut, Required, &nMffcSize );
            pCut->nLeaves = nLeavesOld; 
        }
        // check the best gain
        //if ( !(p->GainBest > 0 || (p->GainBest == 0 && p->pPars->fUseZeros)) )
        if ( p->GainBest < p->pPars->nMinSaved )
        {
//            Aig_ObjOrderAdvance( pAig );
            continue;
        }
//        nMffcGains[p->GainBest < MAX_VAL ? p->GainBest : MAX_VAL][nMffcSize < MAX_VAL ? nMffcSize : MAX_VAL]++;
        // remove the old cuts
        Dar_ObjSetCuts( pObj, NULL );
        // if we end up here, a rewriting step is accepted
        nNodeBefore = Aig_ManNodeNum( pAig );
        pObjNew = Dar_LibBuildBest( p , mutations, certif_man ); // pObjNew can be complemented!
        int complement = Aig_ObjPhaseReal(pObjNew) ^ pObj->fPhase;
        pObjNew = Aig_NotCond( pObjNew, complement );
        assert( (int)Aig_Regular(pObjNew)->Level <= Required );

        //@ Emitting the certificates here:
        //@ - a `replace_node` mutation
        //@ - the hint associated with the rewrite.
        int old_id = Aig_Regular(pObj)->CertifId;
        int new_id = Aig_Regular(pObjNew)->CertifId;

        Mutation_t *mut = new_mutation_replace(
            old_id,
            new_id,
            complement
        );
        Vec_PtrPush(mutations, (void *)mut);

        Hint_t *hint = new_hint(
            old_id,
            new_id,
            complement
        );
        Vec_PtrPush(hints, (void *)hint);

        assert(Aig_ObjIsAnd(pObj));
        printf("replacing %d(%d) with %d(%d)\n", pObj->CertifId, pObj->Id,  Aig_Regular(pObjNew)->CertifId, Aig_Regular(pObjNew)->Id);

        // replace the node
        Aig_ObjReplace( pAig, pObj, pObjNew, p->pPars->fUpdateLevel );
        // compare the gains
        nNodeAfter = Aig_ManNodeNum( pAig );
        assert( p->GainBest <= nNodeBefore - nNodeAfter );
        // count gains of this class
        p->ClassGains[p->ClassBest] += nNodeBefore - nNodeAfter;
    }
//    Aig_ManOrderStop( pAig );
/*
    printf( "Distribution of gain (row) by MFFC size (column) %s 0-costs:\n", p->pPars->fUseZeros? "with":"without" );
    for ( k = 0; k <= MAX_VAL; k++ )
        printf( "<%4d> ", k );
    printf( "\n" );
    for ( i = 0; i <= MAX_VAL; i++ )
    {
        for ( k = 0; k <= MAX_VAL; k++ )
            printf( "%6d ", nMffcGains[i][k] );
        printf( "\n" );
    }
*/

    //@ Registering certificate associated with this rewrite pass.
    Certificate_t *certif = new_certificate(mutations, hints);
    Vec_PtrPush(certificates, (void *) certif);

p->timeTotal = Abc_Clock() - clkStart;
p->timeOther = p->timeTotal - p->timeCuts - p->timeEval;

//    Bar_ProgressStop( pProgress );
    Dar_ManCutsFree( p );
    // put the nodes into the DFS order and reassign their IDs
//    Aig_NtkReassignIds( p );
    // fix the levels
//    Aig_ManVerifyLevel( pAig );
    if ( p->pPars->fFanout )
        Aig_ManFanoutStop( pAig );
    if ( p->pPars->fUpdateLevel )
    {
//        Aig_ManVerifyReverseLevel( pAig );
        Aig_ManStopReverseLevels( pAig );
    }
    if ( pAig->vProbs )
    {
        Vec_IntFree( pAig->vProbs );
        pAig->vProbs = NULL;
    }
    // stop the rewriting manager
    Dar_ManStop( p );
    Aig_ManCheckPhase( pAig );
    // check
    if ( !Aig_ManCheck( pAig ) )
    {
        printf( "Aig_ManRewrite: The network check has failed.\n" );
        return 0;
    }

    check_certif_id(pAig);
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes the total number of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dar_ManCutCount( Aig_Man_t * pAig, int * pnCutsK )
{
    Dar_Cut_t * pCut;
    Aig_Obj_t * pObj;
    int i, k, nCuts = 0, nCutsK = 0;
    Aig_ManForEachNode( pAig, pObj, i )
        Dar_ObjForEachCut( pObj, pCut, k )
        {
            nCuts++;
            if ( pCut->nLeaves == 4 )
                nCutsK++;
        }
    if ( pnCutsK )
        *pnCutsK = nCutsK;
    return nCuts;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_MmFixed_t * Dar_ManComputeCuts( Aig_Man_t * pAig, int nCutsMax, int fSkipTtMin, int fVerbose )
{ 
    Dar_Man_t * p;
    Dar_RwrPar_t Pars, * pPars = &Pars; 
    Aig_Obj_t * pObj;
    Aig_MmFixed_t * pMemCuts;
    int i, nNodes;
    abctime clk = Abc_Clock();
    // remove dangling nodes
    if ( (nNodes = Aig_ManCleanup( pAig )) )
    {
//        printf( "Removing %d nodes.\n", nNodes );
    }
    // create default parameters
    Dar_ManDefaultRwrParams( pPars );
    pPars->nCutsMax = nCutsMax;
    // create rewriting manager
    p = Dar_ManStart( pAig, pPars );
    // set elementary cuts for the PIs
//    Dar_ManCutsStart( p );
    Aig_MmFixedRestart( p->pMemCuts );
    Dar_ObjPrepareCuts( p, Aig_ManConst1(p->pAig) );
    Aig_ManForEachCi( pAig, pObj, i )
        Dar_ObjPrepareCuts( p, pObj );
    // compute cuts for each nodes in the topological order
    Aig_ManForEachNode( pAig, pObj, i )
        Dar_ObjComputeCuts( p, pObj, fSkipTtMin );
    // print verbose stats
    if ( fVerbose )
    {
//        Aig_Obj_t * pObj;
        int nCuts, nCutsK;//, i;
        nCuts = Dar_ManCutCount( pAig, &nCutsK );
        printf( "Nodes = %6d. Total cuts = %6d. 4-input cuts = %6d.\n",
            Aig_ManObjNum(pAig), nCuts, nCutsK );
        printf( "Cut size = %2d. Truth size = %2d. Total mem = %5.2f MB  ",
            (int)sizeof(Dar_Cut_t), (int)4, 1.0*Aig_MmFixedReadMemUsage(p->pMemCuts)/(1<<20) );
        ABC_PRT( "Runtime", Abc_Clock() - clk );
/*
        Aig_ManForEachNode( pAig, pObj, i )
            if ( i % 300 == 0 )
                Dar_ObjCutPrint( pAig, pObj );
*/
    }
    // free the cuts
    pMemCuts = p->pMemCuts;
    p->pMemCuts = NULL;
//    Dar_ManCutsFree( p );
    // stop the rewriting manager
    Dar_ManStop( p );
    return pMemCuts;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

