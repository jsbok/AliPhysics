// Analysis Task for the Quality Assurence of Beam Gas Monitoring
//
// This code will draw the Tracklet vs Cluster 2D histogram with various Triggers and PF conditions
//
// Authors
// Alexander Borissov <aborisso@mail.cern.ch>
// Bong-Hwi Lim <bong-hwi.lim@cern.ch>
//
// If you have any comment or question of this code,
// Please send a mail to Bong-Hwi
//
// Last update: 2016.08.09 (blim)
//
//#include <Riostream.h>
#include <iostream>
#include"AliAnalysisBGMonitorQAHLT.h"
#include <TFile.h>
#include <TChain.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH1D.h>
#include <TH3F.h>
#include"TCanvas.h"
#include"TArrayI.h"
#include "TString.h"
#include"AliAnalysisTask.h"
#include"AliAnalysisManager.h"
#include"AliVEvent.h"
#include"AliVfriendEvent.h"
#include"AliVEventHandler.h"
#include"AliLog.h"
#include"AliAnalysisFilter.h"
#include"AliTriggerAnalysis.h"
#include"AliAnalysisCuts.h"
#include"AliMultiplicity.h"
#include"AliVMultiplicity.h"
#include"AliESDVZERO.h"
#include"AliESDVZEROfriend.h"
#include"AliESDTZERO.h"
#include "AliLog.h"

class AliAnalysisBGMonitorQAHLT;
using namespace std;

ClassImp(AliAnalysisBGMonitorQAHLT)

AliAnalysisBGMonitorQAHLT::AliAnalysisBGMonitorQAHLT() : AliAnalysisTask(),
fESD(0x0),
fESDfriend(0x0),
fList(0),
fUseTree(kFALSE),
fSpdClusters(0),
fSpdTracklets(0),
triggerType(0),
ntracks(0),
ntr(0),
nbunch(0),
nV0A(0),
nV0C(0),
nV0ABG(0),
nV0CBG(0)
{
}

//________________________________________________________________________
AliAnalysisBGMonitorQAHLT::AliAnalysisBGMonitorQAHLT(const char *name):
AliAnalysisTask(name,name),
fESD(0x0),
fESDfriend(0x0),
fList(0),
fUseTree(kFALSE),
fSpdClusters(0),
fSpdTracklets(0),
triggerType(0),
ntracks(0),
ntr(0),
nbunch(0),
nV0A(0),
nV0C(0),
nV0ABG(0),
nV0CBG(0)
{
    // Constructor
    DefineInput(0, TChain::Class());
    DefineOutput(1, TList::Class());
    DefineOutput(0, TTree::Class()); //RunNumber
}
AliAnalysisBGMonitorQAHLT::~AliAnalysisBGMonitorQAHLT()
{
    // destructor
    if(fList) {
      fList->Delete();
    }
    delete fList;     // at the end of your task, it is deleted from memory by calling this function
}

//________________________________________________________________________
void AliAnalysisBGMonitorQAHLT::CreateHistograms(TList*& list, Option_t* /*options*/)
{
  if (!list) {
    list = new TList();
    list->SetOwner(kTRUE);    
  }
  TH3F *hTotalTrkVsClsSPID = new TH3F("hTotalTrkVsClsSPID","; Spd : total",140,0,140,500,0,500,10,-0.5,9.5);
  hTotalTrkVsClsSPID->GetXaxis()->SetTitle("Tracklet");
  hTotalTrkVsClsSPID->GetYaxis()->SetTitle("Cluster (fspdC1+fspdC2)");
  list->Add(hTotalTrkVsClsSPID);
}

//________________________________________________________________________
Int_t AliAnalysisBGMonitorQAHLT::ResetHistograms(TList* list)
{
  int rc = 0;
  TIter i(list);
  while (TObject* obj = i.Next()) {
    TH1* hist = dynamic_cast<TH1*>(obj);
    if (hist) {
      hist->Reset();
      rc++;
    }
  }
  return rc;
}

//________________________________________________________________________
Bool_t AliAnalysisBGMonitorQAHLT::ResetOutputData()
{
  return ResetHistograms(fList)>0;
}

//________________________________________________________________________
void AliAnalysisBGMonitorQAHLT::CreateOutputObjects()
{
    CreateHistograms(fList);
    PostData(1, fList);
}

//________________________________________________________________________
void AliAnalysisBGMonitorQAHLT::Exec(Option_t *)
{
  AliVEventHandler *esdH = AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler();
  if (!esdH) {
    AliError("ERROR: Could not get VEventHandler");
    return;
  }
  
  fESD = esdH->GetEvent();
  if(!fESD) {
    AliError("no VEvent!");
    return;
  }
  
  fESDfriend = fESD->FindFriend();

  if (!fESDfriend) {
    AliError("no VEventFriend!");
    return;
  }

  //get the VZERO
  AliESDVZERO tmpvzero;
  AliESDVZERO* vzero = NULL;
  if (fESD->GetVZEROData(tmpvzero)==0) {
    vzero = &tmpvzero;
  }

  //get the VZERO friend
  AliESDVZEROfriend tmpesdV0friend;
  AliESDVZEROfriend* vzeroFriend = NULL;
  if (fESDfriend->GetESDVZEROfriend(tmpesdV0friend)==0) {
    vzeroFriend = &tmpesdV0friend;
  }

  //--- SPD cluster and tracklets
  AliMultiplicity tmpMult;
  AliVMultiplicity* mult = NULL;
  if (fESD->GetMultiplicity(tmpMult)==0) {
    mult = &tmpMult;
  }

  //string with fired trigger classes
  std::string firedTriggerClasses = fESD->GetFiredTriggerClasses().Data();
  
  //FILL the histos
  FillHistograms(fList, &firedTriggerClasses, mult, vzero, vzeroFriend);
}

//________________________________________________________________________
void AliAnalysisBGMonitorQAHLT::Terminate(Option_t *)
{
}

//________________________________________________________________________
void AliAnalysisBGMonitorQAHLT::FillHistograms( TList* list,
                                           std::string* firedTriggers,
                                           AliVMultiplicity* mult,
                                           AliVVZERO* vzero,
                                           AliVVZEROfriend* vzeroFriend)
{

  if (!list || !firedTriggers || !mult || !vzero || !vzeroFriend) {
    return;
  }

  Int_t nSPDclusters = 0;
  Int_t nSPDtracklets = 0;
  if (mult) {
    nSPDclusters = mult->GetNumberOfITSClusters(0) + mult->GetNumberOfITSClusters(1);
    nSPDtracklets = mult->GetNumberOfTracklets();
  }

  //"online" V0 flags
  Int_t nV0A = 0;
  Int_t nV0ABG = 0;
  Int_t nV0C = 0;
  Int_t nV0CBG = 0;
    for (Int_t i = 32; i < 64; ++i) {
      if (vzero->GetBBFlag(i)) nV0A++;
      if (vzero->GetBGFlag(i)) nV0ABG++;
    }
    for (Int_t i = 0; i < 32; ++i) {
      if (vzero->GetBBFlag(i)) nV0C++;
      if (vzero->GetBGFlag(i)) nV0CBG++;
    }
  
  UShort_t nbunch = 21;
  Int_t BGFlagA[kMaxUShort];
  Int_t BGFlagC[kMaxUShort];
  Int_t BBFlagA[kMaxUShort];
  Int_t BBFlagC[kMaxUShort];
  memset(BGFlagA, 0, sizeof(Float_t)*nbunch);
  memset(BBFlagA, 0, sizeof(Float_t)*nbunch);
  memset(BGFlagC, 0, sizeof(Float_t)*nbunch);
  memset(BBFlagC, 0, sizeof(Float_t)*nbunch);

    for(Int_t j = 0; j < 20; j++){
      for (Int_t i = 32; i < 64; ++i) {
        if(vzeroFriend->GetBBFlag(i,j)) BBFlagA[j]++;
        if(vzeroFriend->GetBGFlag(i,j)) BGFlagA[j]++;
      }
      for (Int_t i = 0; i < 32; ++i) {
        if(vzeroFriend->GetBBFlag(i,j)) BBFlagC[j]++;
        if(vzeroFriend->GetBGFlag(i,j)) BGFlagC[j]++;
      }
    }

  //--- Trigger classes --//
  UShort_t ntr = 10;
  Int_t triggerType=0;

  if( firedTriggers->find("CINT7-B-NOPF-ALLNOTRD")!=std::string::npos ||
      firedTriggers->find("CINT7-S-NOPF-ALLNOTRD")!=std::string::npos ||
      firedTriggers->find("CINT1-B-NOPF-ALLNOTRD")!=std::string::npos ||
      firedTriggers->find("CINT1-S-NOPF-ALLNOTRD")!=std::string::npos ||
      firedTriggers->find("CINT7-A-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CINT7-B-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CINT7-C-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CINT7-E-NOPF-CENT")!=std::string::npos) 
    triggerType = 1; // CINT7 trigger

  if( firedTriggers->find("CVHMV0M-A-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMV0M-B-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMV0M-C-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMV0M-E-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMV0M-B-SPD1-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMV0M-A-NOPF-CENTNOTRD")!=std::string::npos ||
      firedTriggers->find("CVHMV0M-B-NOPF-CENTNOTRD")!=std::string::npos ||
      firedTriggers->find("CVHMV0M-C-NOPF-CENTNOTRD")!=std::string::npos ||
      firedTriggers->find("CVHMV0M-E-NOPF-CENTNOTRD")!=std::string::npos)
    triggerType = 4; // VOM trigger

  if( firedTriggers->find("CVHMSH2-A-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMSH2-B-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMSH2-C-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMSH2-E-NOPF-CENT")!=std::string::npos ||
      firedTriggers->find("CVHMSH2-B-NOPF-ALL")!=std::string::npos ||
      firedTriggers->find("CVHMSH2-B-NOPF-CENTNOTRD")!=std::string::npos )
    triggerType = 7; // SH2 trigger

  if (triggerType==0) {
    return;
  }
  TH3* tmp = NULL;
  tmp = static_cast<TH3*>(list->FindObject("hTotalTrkVsClsSPID"));
  if (tmp) tmp->Fill(nSPDtracklets, nSPDclusters,triggerType); // No PF Selection
  
  Bool_t SelGoodEvent = 0;
  for(Int_t ii=1; ii<33; ii++) {
    //___________
    SelGoodEvent = BBFlagA[11]<ii  &
                   BBFlagA[12]<ii  &
                   BBFlagA[13]<ii  &
                   BBFlagA[14]<ii  &
                   BBFlagA[15]<ii  &
                   BBFlagA[16]<ii  &
                   BBFlagA[17]<ii; //BB-A 11-17

    SelGoodEvent &= BBFlagC[11]<ii  &
                    BBFlagC[12]<ii  &
                    BBFlagC[13]<ii  &
                    BBFlagC[14]<ii  &
                    BBFlagC[15]<ii  &
                    BBFlagC[16]<ii  &
                    BBFlagC[17]<ii; //BB-C 11-17

    SelGoodEvent &= BBFlagA[9]<ii  &
                    BBFlagA[8]<ii  &
                    BBFlagA[7]<ii  &
                    BBFlagA[6]<ii  &
                    BBFlagA[5]<ii  &
                    BBFlagA[4]<ii  &
                    BBFlagA[3]<ii; //BB-A 3-9

    SelGoodEvent &= BBFlagC[9]<ii  &
                    BBFlagC[8]<ii  &
                    BBFlagC[7]<ii  &
                    BBFlagC[6]<ii  &
                    BBFlagC[5]<ii  &
                    BBFlagC[4]<ii  &
                    BBFlagC[3]<ii; //BB-C 3-9
    //___________
    if(SelGoodEvent) {
      if(ii == 2){
        tmp = static_cast<TH3*>(list->FindObject("hTotalTrkVsClsSPID"));
        if (tmp) tmp->Fill(nSPDtracklets, nSPDclusters,triggerType+1); // PF = 2 Condition
      }
      if(ii == 10){
  tmp = static_cast<TH3*>(list->FindObject("hTotalTrkVsClsSPID"));
        if (tmp) tmp->Fill(nSPDtracklets, nSPDclusters,triggerType+2); // PF = 10 Condition
      }
    }
  }
}
