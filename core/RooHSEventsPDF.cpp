#include "RooHSEventsPDF.h"
 
#include <RooRealVar.h>
#include <RooCategory.h> 
#include <RooRandom.h>
#include <RooDataSet.h> 
#include <RooMsgService.h> 
#include <RooGaussian.h>
#include <RooPlot.h>
#include <RooHist.h>
#include <RooPullVar.h> 
#include <TMath.h>
#include <TObjArray.h> 
#include <TCanvas.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <TSystem.h>
#include <TEntryList.h>
#include <algorithm> 
#include <random>


namespace HS{
  namespace FIT{

    Bool_t RooHSEventsPDF::RooHSEventsPDF_IsPlotting=kFALSE;

    void RooHSEventsPDF::SetIsPlotting(Bool_t is){
      RooHSEventsPDF_IsPlotting=is;	
    }

    RooHSEventsPDF::RooHSEventsPDF(const RooHSEventsPDF& other, const char* name) :  RooAbsPdf(other,name) 
    {
      // cout<<"RooHSEventsPDF::RooHSEventsPDF "<<GetName()<<other.fNTreeEntries<< " "<<other.fvecReal.size()<<endl;
      fIsClone=kTRUE;
      fParent=const_cast<RooHSEventsPDF*>(&other);
      fvecReal=other.fvecReal;
      fvecCat=other.fvecCat;
      fvecRealGen=other.fvecRealGen;
      fvecCatGen=other.fvecCatGen;
      fNTreeEntries=other.fNTreeEntries;
      fTreeEntryNumber=other.fTreeEntryNumber;
    
      if(other.fEvTree)fEvTree=other.fEvTree->CopyTree("");
      //    if(other.fInWeights) fInWeights=other.fInWeights; //probably need to clone this
      fNInt=other.fNInt;
      fGeni=other.fGeni;
      //if(other.fEntryList)fEntryList=(TEntryList*)other.fEntryList->Clone();
      fForceConstInt=other.fForceConstInt;
      fForceNumInt=other.fForceNumInt;
      fConstInt=other.fConstInt;
      fCheckInt=other.fCheckInt;
      fUseWeightsGen=other.fUseWeightsGen;
      fCut=other.fCut;
      fInWeightCut=other.fInWeightCut;
      fIsValid=other.fIsValid;
      fUseEvWeights=other.fUseEvWeights;
    
      fWgtsConf=other.fWgtsConf;
									     
      fEvWeights=other.fEvWeights;
      fHistIntegrals=other.fHistIntegrals;
      //fWgtSpecies=other.fWgtSpecies;
      //fWgtsFile=other.fWgtsFile;
      //fWgtsName=other.fWgtsName;
      fMaxValue=other.fMaxValue;
      fIntRangeLow=other.fIntRangeLow;
      fIntRangeHigh=other.fIntRangeHigh;
    }
    RooHSEventsPDF::~RooHSEventsPDF(){

      /*   std::cout<<"delete RooHSEventsPDF::~RooHSEventsPDF() "
	<<fIsClone<<" "<<fParent<<" "<<fEntryList<<" "
	<<fMaxValue<<" "
	<< endl;
      */   
      //If I made a generator entry list save it
      //so it can be used to filter the original tree
      //object the entrylist if I want to use it!
      if(fEntryList!=nullptr){
	if(fEntryList->GetN()!=0){//has this clone used generator?
	  TFile entryFile(TString("entryFile_")+GetName()+".root","recreate");
	  fEntryList->Write();
	}
      }
      
      if(fEntryList) delete fEntryList;
      if(fLast) delete fLast;
      if(fEvTree) delete fEvTree;
   
      if(fWeights){
	fWeights->Save();
	delete fWeights;
      }
      if(fInWeights){
	delete fInWeights;
      }
      for(auto & i : fVarSet)
	delete i;
 
      fVarSet.clear();
    }

    void RooHSEventsPDF::InitSets(){
      fNpars=fParSet.size();
      fNvars=fProxSet.size();
      fNcats=fCatSet.size();
      fLastLength=fNpars+1;
      fLast=new Float_t[fNpars+1]; //Number of fit parameters
      for(Int_t i=0;i<fNpars+1;i++)
	fLast[i]=100;
    }
    RooArgSet RooHSEventsPDF::VarSet(Int_t iset) const{
      RooArgSet aset(Form("VarSet_%d",iset));
      if(iset==0){
	for(auto i : fProxSet){
	  aset.add(i->arg());
	}
	for(auto i : fCatSet){
	  aset.add(i->arg());
	}
      }
      else{
	for(auto j : fProxSet){//add if not proxy being removed
	  if(fProxSet[iset-1]->GetName()!=j->GetName()) aset.add(j->arg());
	}
	for(auto i : fCatSet){ //Just add all categories
	  aset.add(i->arg());
	}
      }
      return aset;
    }
    Int_t RooHSEventsPDF::getGenerator(const RooArgSet& directVars, RooArgSet &generateVars, Bool_t staticInitOK) const
    {
      cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!RooHSEventsPDF::getGenerator "<<fEvTree<<" "<<fvecReal.size()<<endl;
      Info("RooHSEventsPDF::getGenerator","Looking for generator");
      if(!fEvTree) return 0; //no MC events to generate from
      //case generate all variables
      if (matchArgs(directVars,generateVars,VarSet(0))) return 1 ;
      return 0;

    }
    void RooHSEventsPDF::initIntegrator()
    {
  
    }
    void RooHSEventsPDF::initGenerator(Int_t code)
    {
      Info("RooHSEventsPDF::initGenerator","Going to generate starting from %lld with weights %d",fGeni,(Int_t)fUseWeightsGen);
      //Calculate the max value for accept reject purposes
      //Note we use parent to make sure this is only done once
      //RooFit creates a clone PDF instance each time it wants to generate
      if(fParent->GetMaxValue()==0||fParent->CheckChange()){	
	Double_t value=0;		
	if(code==1){	
	  //Brute force find maximum value
	  fMaxValue=0;
	  for(Int_t i=0;i<fNTreeEntries;i++){
	    fTreeEntry=i;
	    value=evaluateMC(&fvecRealGen,&fvecCatGen);
       
	    if(value>fMaxValue)fMaxValue=value*1.01;//make it a little larger
	  }
 
	  fParent->SetMaxValue(fMaxValue);
	}
      }		
      //construct entry list so can reproduce full tree branches,
      //not jist those loaded as variables
      if(!fEntryList){
	fEntryList=new TEntryList("GenEvents","GenEvents",fEvTree);
      }
      else{
	fEntryList->Reset();
	fEntryList->SetTree(fEvTree);
      }
      if(fUseWeightsGen){
	fWeights=new Weights("genWeights");
	fWeights->SetSpecies(GetName());
	fWeights->SetFile(TString(GetName())+"Weights.root");
      }
      Info("RooHSEventsPDF::initGenerator","Max value %lf",fMaxValue);
    }
    void RooHSEventsPDF::generateEvent(Int_t code){
      // Info("RooHSEventsPDF::generateEvent","Going to generate starting from %lld with ",fGeni);
  
      Double_t value=0;
      if(!fUseWeightsGen){
	while(fGeni<fNTreeEntries){
	  //fParent->SetGeni(fGeni);
	  //fEvTree->GetEntry(fGeni++);
	  fTreeEntry=fGeni++;
	  value=evaluateMC(&fvecRealGen,&fvecCatGen); //evaluate true values
	  if(value>fMaxValue*RooRandom::uniform()){//accept
	    for(Int_t i=0;i<fNvars;i++)
	      (*(fProxSet[i]))=fvecReal[fTreeEntry*fNvars+i]; //write reconstructed
	    for(Int_t i=0;i<fNcats;i++)
	      (*(fCatSet[i]))=fvecCat[fTreeEntry*fNcats+i];

											    //Add actual entry number from original tree
	    //this can then be used to filter original tree
	    //with all branches
	    fEntryList->Enter(fTreeEntryNumber[fTreeEntry]);
	    return;
	  }
	}
      }
      else{
	//using weights
	while(fGeni<fNTreeEntries){
	  fParent->SetGeni(fGeni);
	  //fEvTree->GetEntry(fGeni++);
	  fTreeEntry=fGeni++;
	  if(!CheckRange("")) continue;
	  value=evaluateMC(&fvecRealGen,&fvecCatGen);
	  for(Int_t i=0;i<fNvars;i++)
	    (*(fProxSet[i]))=fvecReal[fTreeEntry*fNvars+i];
	  for(Int_t i=0;i<fNcats;i++)
	    (*(fCatSet[i]))=fvecCat[fTreeEntry*fNcats+i];
	  fWeights->FillWeight(fGeni-1,value); 
	  fEntryList->Enter(fGeni-1);
	  return;
	}
      }
      Error("RooHSEventsPDF::generateEvent","Ran out of events at %lld",fGeni);
      //Used up all the events in the tree!
    }
    Int_t RooHSEventsPDF::getAnalyticalIntegral(RooArgSet& allVars, RooArgSet& analVars,const char* rangeName) const
    {
      // cout<<"RooHSEventsPDF::getAnalyticalIntegral "<<fEvTree<<" "<<fvecReal.size()<<" "<<fProxSet.size()<<endl;
      // for(auto& prox:fProxSet)
      //cout<<prox->GetName()<<endl;
  
      if(fForceNumInt) return 0; //might be good to check numerical integral sometimes
      if(!fEvTree&&!fForceConstInt) return 0; //no MC events to integrate over

      // cout<<"RooHSEventsPDF::getAnalyticalIntegral "<<fProxSet.size()<<" "<<RooHSEventsPDF_IsPlotting<<endl;
      if(fProxSet.size()==1&&fCatSet.size()==0){//special case 1 variable
	//if(RooHSEventsPDF_IsPlotting) {return 0;}
	if (matchArgs(allVars,analVars,VarSet(0))){
	  // allVars.Print();analVars.Print();
	  // if(RooHSEventsPDF_IsPlotting) {return 1;}
	  //else return 1 ;
	  //if now plotting create histograms
	  if(RooHSEventsPDF_IsPlotting&&fHistIntegrals.size()==0)
	    HistIntegrals(rangeName);

	  return 1;
	}
      }
      else{//For variables
	//    for(UInt_t i=0;i<1+fProxSet.size();i++){
	for(UInt_t i=0;i<1+fProxSet.size();i++){
	  if(!fEvTree&&fForceConstInt&&i==0) {return 1;} //no tree, but const int
	  else if(!fEvTree) return 0; //no const integral for projections
	  if (matchArgs(allVars,analVars,VarSet(i))) {return i+1 ;}
	}
      }
      //Note not implemented for cats
      return 0;
    }

    Double_t RooHSEventsPDF::analyticalIntegral(Int_t code,const char* rangeName) const
    {
      // cout<<"Analystic "<<fIntCounter++<<" "<<fEvTree<<" "<<code<<endl;
      if(code==1&&fForceConstInt&&!fEvTree) {fLast[0]=1;return fLast[0];}
      //sort number of events first in case forced
      Long64_t NEv=0;
  
 
      // Info("RooHSEventsPDF::analyticalIntegral","calcing my own integral");
      // return 1;
      //In case changed for generation
  
      Double_t integral=0;
      Long64_t ilow=0;
      Long64_t ihigh=0;
      SetLowHighVals(ilow,ihigh); 
      //only recalculate if a par changes when all variables included (ie code=1)
      if(code==1)
	if(!CheckChange()) return fLast[0];
 
      if(code==1){
	Long64_t accepted=0;
	for(Long64_t ie=ilow;ie<ihigh;ie++){
	  fTreeEntry=ie;
	  // fEvTree->GetEntry(ie);
	  if(!CheckRange(TString(rangeName).Data())) continue;
	  accepted++;
	  integral+=evaluateMC(&fvecReal,&fvecCat)*GetIntegralWeight(ie);
	}
	integral/=accepted;
      }
      else{
	if(fHistIntegrals.size()==0)
	  HistIntegrals(rangeName);
	
	Int_t vindex=code-2;
	Double_t vval=*(fProxSet[vindex]);
	integral=fHistIntegrals[vindex].Interpolate(vval);
      }
      // else {
      //   //inegrate over other variables for one variable fixed
      //   //index given by code -2 (defintion of code in getAnalyticalIntegral
      //   //This is used for plotting data and PDFS
      //   Int_t vindex=code-2;
      //   if(vindex<0)vindex=0;
      //   Double_t rmax=fProxSet[vindex]->max();
      //   Double_t rmin=fProxSet[vindex]->min();
      //   Double_t vval=*(fProxSet[vindex]);
      //   Double_t vrange=rmax-rmin;
      //   //  fProxSet[code-2]->Print();
      //   Int_t nbins=((RooRealVar*)(&(fProxSet[vindex]->arg())))->getBins();
      //   Double_t delta=vrange/nbins/2;
      //   //Double_t delta=vrange/nbins;
      //   for(Int_t ie=ilow;ie<ihigh;ie++){
      //     fTreeEntry=ie;
      //     // fEvTree->GetEntry(ie);
      //     if(!CheckRange(TString(rangeName).Data())) continue;
      //   //only inlcude events within same bin as vval in integral
      //     if(TMath::Abs(fvecReal[fTreeEntry*fNvars+vindex]-vval)>delta)continue;
      //     integral+=evaluateMC(&fvecReal,&fvecCat)*GetIntegralWeight(ie);
      //   }
      //   //correct for delta integration width
      //   //first 2 case near range limits 
      //   if((rmax-vval)<delta) delta=delta+rmax-vval;
      //   else if((vval-rmin)<delta) delta=delta+vval-rmin;
      //   else delta=delta*2;
      //   integral=integral/delta;
      //   cout<<"Integral "<<integral<<" "<<delta<<" "<<nbins<<" "<<vval<<endl;

      // }
      //else return 1;
      // Set Last[0] so we can just return that if no parameter changes
      fLast[0]=integral;

      return fLast[0];
    }

    Double_t RooHSEventsPDF::unnormalisedIntegral(Int_t code,const char* rangeName) const{
		Double_t integral=0;
		Double_t nev=0;
		Double_t nMC=0;
		if(code==1){
			for(Long64_t ie=0;ie<fNTreeEntries;ie++){
				fTreeEntry=ie;
				// fEvTree->GetEntry(ie);
				if(!CheckRange(TString(rangeName).Data())) continue;
				integral+=evaluateMC(&fvecReal,&fvecCat)*GetIntegralWeight(ie);
				nev++;
			}
			cout << "RooHSEventsPDF::unnormalisedIntegral #MC=" << nev << endl;
		}
		else if(code==2 && fHasMCGenTree){
			for(Long64_t ie=0;ie<fNMCGenTreeEntries;ie++){
				fTreeEntry=ie;
				integral+=evaluateMC(&fvecRealMCGen,&fvecCatMCGen);
				nMC++;
			}
		cout << "RooHSEventsPDF::unnormalisedIntegral #GEN= " << nMC << endl;
		}
		else{
		  return 0;
		}
		return integral;
    }
    
    void RooHSEventsPDF::HistIntegrals(const char* rangeName) const{
      //create histograms for each observable and fill integrating
      //over other observables, filling with evaluateMC
      Long64_t ilow=0;
      Long64_t ihigh=0;
      SetLowHighVals(ilow,ihigh);
      // cout<<"RooHSEventsPDF::HistIntegrals"<<endl;
      for(Int_t i=0;i<fNvars;i++){
	auto  arg=dynamic_cast<const RooRealVar*>(&fProxSet[i]->arg());
	if(arg)
	  fHistIntegrals.emplace_back(arg->GetName(),arg->GetName(),arg->getBins(),arg->getMin(),arg->getMax());
      }
      Long64_t accepted=0;
      for(Int_t ie=ilow;ie<ihigh;ie++){
	fTreeEntry=ie;
	if(!CheckRange(TString(rangeName).Data())){continue;}
	accepted++;
	Double_t value=evaluateMC(&fvecReal,&fvecCat)*GetIntegralWeight(ie);
	for(Int_t vindex=0;vindex<fNvars;vindex++){
	  fHistIntegrals[vindex].Fill(fvecReal[fTreeEntry*fNvars+vindex],value/fHistIntegrals[vindex].GetBinWidth(1));
	}
      }
      //normalise to number of accepted events
      for(Int_t vindex=0;vindex<fNvars;vindex++)
	fHistIntegrals[vindex].Scale(1./accepted);
  
      fParent->SetHistIntegrals(fHistIntegrals);
  
    }

    void RooHSEventsPDF::SetLowHighVals(Long64_t& ilow,Long64_t& ihigh) const{
      ilow=0;
      ihigh=0;
  
      if(fParent){
	ilow=fParent->GetIntRangeLow();
	ihigh=fParent->GetIntRangeHigh();
      }
      else{
	ilow=GetIntRangeLow();
	ihigh=GetIntRangeHigh();
      }

      if(ihigh==0&&fNInt>-1) ihigh=fNInt;
  
      else if(ihigh==0) ihigh=fNTreeEntries; 
  
      if(ihigh>fNTreeEntries) ihigh=fNTreeEntries;
    }

    Bool_t RooHSEventsPDF::CheckRange(const char* rangeName) const{
      //bool brange=TString(rangeName)==TString("");
      //if(brange) return kTRUE;
      for(UInt_t i=0;i<fProxSet.size();i++){
	//	RooRealVar* var=(dynamic_cast<RooRealVar*>(&(fProxSet[i]->arg())));
	auto var=(dynamic_cast<const RooRealVar*>(&(fProxSet[i]->arg())));
	if(!var->inRange(fvecReal[fTreeEntry*fNvars+i],TString(rangeName).Data())){return kFALSE;}
      }
      return kTRUE;

    }
    Bool_t RooHSEventsPDF::CheckChange() const{
      //Note analytical integral is const funtion so can only change data members
      //which are pointed to something, thus need Double_t *fLast
      //and construct a N-D array where we can change elements

      Bool_t hasChanged=false;
      for(Int_t i=1;i<fNpars+1;i++)
	if(fLast[i]!=(*(fParSet[i-1]))) hasChanged=true;
      if(hasChanged){
	for(Int_t i=1;i<fNpars+1;i++){
	  std::cout<<"RooHSEventsPDF check change "<<fParSet[i-1]->GetName()<<" "<<fLast[i]<<" "<<*(fParSet[i-1])<<std::endl;
	  fLast[i]=*(fParSet[i-1]);
	 
	}
      }
      return hasChanged;
    }
    // Bool_t RooHSEventsPDF::SetEvTree(TChain* tree,TString cut,Long64_t ngen){
    //   if(!tree->GetEntries()) return kFALSE;
    //   return SetEvTree(tree,cut,ngen);
    // }
    Bool_t RooHSEventsPDF::SetEvTree(TTree* tree,TString cut,TTree* MCGenTree){
      if(!tree->GetEntries())return kFALSE;
      Info("RooHSEventsPDF::SetEvTree"," with name %s and cut  = %s",tree->GetName(),cut.Data());
      //      cout<<"RooHSEventsPDF::SetEvTree "<<GetName()<<endl;
      //Set the cut
      //Note weight cut can be set with WEIGHT@expr in factory constructor
      if(cut==TString())
	fCut=fInWeightCut;
      else if (fInWeightCut==TString())	
	fCut=cut;
      else
	fCut=cut+"&&"+fInWeightCut;

      ProcInfo_t info;
      fEvTree=tree;
      if(MCGenTree){ // generated events used for acceptance correction, do only if tree is available
	fMCGenTree=MCGenTree;
	fHasMCGenTree=kTRUE;
	//cout<<"RooHSEventsPDF::SetEvTree set MC generated tree with " << fMCGenTree->GetEntries() << " entries." <<endl;
      }
      
      
      
      fConstInt=fEvTree->GetEntries();
      fEvTree->ResetBranchAddresses();
      //fEvTree->SetBranchStatus("*",0);
      if(MCGenTree) // generated events used for acceptance correction, do only if tree is available
	fMCGenTree->ResetBranchAddresses();
      fBranchStatus=kTRUE;
      
      TVectorD MCVar(fProxSet.size());
      TVectorD GenVar(fProxSet.size());
      TVectorD MCGenVar(fProxSet.size()); // generated events used for acceptance correction
      vector<Int_t> MCCat(fCatSet.size());
      vector<Int_t> GenCat(fCatSet.size());
      vector<Int_t> MCGenCat(fCatSet.size()); // generated events used for acceptance correction
      fGotGenVar.resize(fProxSet.size());
      fGotGenCat.resize(fProxSet.size());
  
      for(UInt_t i=0;i<fProxSet.size();i++){
	fGotGenVar[i]=0;
    
	if(fEvTree->GetBranch(fProxSet[i]->GetName())){
	  fEvTree->SetBranchStatus(fProxSet[i]->GetName(),true);
	  fEvTree->SetBranchAddress(fProxSet[i]->GetName(),&MCVar[i]);
	  if(fEvTree->GetBranch(TString("gen")+fProxSet[i]->GetName())){
	    fEvTree->SetBranchStatus(TString("gen")+fProxSet[i]->GetName(),true);
	    fEvTree->SetBranchAddress(TString("gen")+fProxSet[i]->GetName(),&GenVar[i]);
	    fGotGenVar[i]=1;
	    cout<<"Using Generated branch "<<TString("gen")+fProxSet[i]->GetName()<<endl;
	  }
	}
	else{
	  Warning("RooHSEventsPDF::SetEvTree","Branch %s not found",fProxSet[i]->GetName()); //May still get set as prototype data
	  //Check if this branch exists in cut and remove it if it does
	  //It may be added later via AddProtoData which should already
	  //have Cut applied to it
	  if(fCut.Contains(fProxSet[i]->GetName())){
	    TString newcut=fCut;
	    newcut.Replace(newcut.Index(fProxSet[i]->GetName())-2,2,"");
	    newcut.Replace(newcut.Index(TString(fProxSet[i]->GetName())+">"),(newcut.Index(TString(fProxSet[i]->GetName())+"<")-newcut.Index(TString(fProxSet[i]->GetName())+">"))*2-1,"");
	    if(newcut==TString("&&"))newcut="";
	    fCut=newcut;
	    cout<<"RooHSEventsPDF::SetEvTree Ammended cut "<<fCut<<endl;
	  }
	  fBranchStatus=kFALSE;
	}
    
	if(MCGenTree){// generated events used for acceptance correction, do only if tree is available
	  if(fMCGenTree->GetBranch(fProxSet[i]->GetName())){
	    fMCGenTree->SetBranchStatus(fProxSet[i]->GetName(),true);
	    fMCGenTree->SetBranchAddress(fProxSet[i]->GetName(),&MCGenVar[i]);
	  }
	  else{
	    Warning("RooHSEventsPDF::SetEvTree","Branch %s not found in MCGen tree. Acceptance correction will be wrong!!!",fProxSet[i]->GetName()); 
	  }
	}
      }
      for(UInt_t i=0;i<fCatSet.size();i++){
	fGotGenCat[i]=0;
	if(fEvTree->GetBranch(fCatSet[i]->GetName())){
	  fEvTree->SetBranchStatus(fCatSet[i]->GetName(),true);
	  fEvTree->SetBranchAddress(fCatSet[i]->GetName(),&MCCat[i]);
	  if(fEvTree->GetBranch(TString("gen")+fCatSet[i]->GetName())){
	    fEvTree->SetBranchStatus(TString("gen")+fCatSet[i]->GetName(),true);
	    fEvTree->SetBranchAddress(TString("gen")+fCatSet[i]->GetName(),&GenCat[i]);
	    fGotGenCat[i]=1;
	    cout<<"Using Generated branch "<<TString("gen")+fCatSet[i]->GetName()<<endl;
	  }	
	}
	else{
	  Warning("RooHSEventsPDF::SetEvTree","Branch %s not found",fCatSet[i]->GetName()); //May still get set as prototype data
	  //Check if this branch exists in cut and remove it if it does
	  //It may be added later via AddProtoData which should already
	  //have Cut applied to it
	  if(fCut.Contains(fCatSet[i]->GetName())){
	    TString newcut=fCut;
	    TString catCut;
	    auto typeIter=fCatSet[i]->arg().typeIterator();
	    catCut+="(";
	    Bool_t first=kTRUE;
	    while(auto type=dynamic_cast<RooCatType*>(typeIter->Next())){
	      if(first){
		catCut+=Form("%s==%d",fCatSet[i]->GetName(),type->getVal());
		first=kFALSE;
	      }
	      else
		catCut+=Form("||%s==%d",fCatSet[i]->GetName(),type->getVal());
	  
	    }	
	    catCut+=")";
	    newcut.ReplaceAll(catCut,"");
	    if(newcut==TString("&&"))newcut="";
	    fCut=newcut;
	    cout<<"RooHSEventsPDF::SetEvTree Ammended cut "<<fCut<<endl;
	  }
	  fBranchStatus=kFALSE;
	}
	if(MCGenTree){// generated events used for acceptance correction, do only if tree is available
	  if(fMCGenTree->GetBranch(fCatSet[i]->GetName())){
	    fMCGenTree->SetBranchStatus(fCatSet[i]->GetName(),true);
	    fMCGenTree->SetBranchAddress(fCatSet[i]->GetName(),&MCGenCat[i]);
	  }
	  else{
	    Warning("RooHSEventsPDF::SetEvTree","Branch %s not found in MCGen tree. Acceptance correction will be wrong!!!",fCatSet[i]->GetName());
	  }
	}
      }
      fEvTree->GetEntry(0);
      
      //Loop over tree, extracting values into vector
      UInt_t ProxSize=fNvars;
      UInt_t CatSize=fNcats;
      fNTreeEntries=fEvTree->GetEntries();
      fvecReal.resize(fNTreeEntries*ProxSize);
      fvecRealGen.resize(fNTreeEntries*ProxSize);
      fvecCat.resize(fNTreeEntries*CatSize);
      fvecCatGen.resize(fNTreeEntries*CatSize);
      if(MCGenTree){// generated events used for acceptance correction, do only if tree is available
	fNMCGenTreeEntries=fMCGenTree->GetEntries();
	fvecRealMCGen.resize(fNMCGenTreeEntries*ProxSize);
	fvecCatMCGen.resize(fNMCGenTreeEntries*CatSize);
      }

      Double_t idVal=0;
      Int_t spId=-1;
      if(fWgtsConf.IsValid()){ //add in ID branch for weighted sim data
	fEvWeights.clear();
	LoadInWeights();
	if(fEvTree->GetBranch(fInWeights->GetIDName())){ //the weight ID branch is in fEvTree
	  fUseEvWeights=kTRUE;
	  fEvTree->SetBranchStatus(fInWeights->GetIDName(),true);
	  fEvTree->SetBranchAddress(fInWeights->GetIDName(),&idVal);
	  fEvWeights.resize(fNTreeEntries);
	  spId=fInWeights->GetSpeciesID(fWgtsConf.Species());
	}
	else cout<<"WARNING RooHSEventsPDF::SetEvTree InWeights ID : "<<fInWeights->GetIDName()<<" does not exist in event tree"<<endl;

      }
  
      //Get entries that pass cut
      //A little subtle but this must be done before SetMakeClass or it
      //doesn't find any entries
      tree->Draw(">>elist", fCut, "entrylist");
      auto *elist = dynamic_cast<TEntryList*>(gDirectory->Get("elist"));
      fEvTree->SetEntryList(elist);
      Long64_t entryNumber=0;
      Long64_t localEntry=0;
      fNTreeEntries=elist->GetN();
	  
      TEntryList* elistMCGen;
      if(MCGenTree){// generated events used for acceptance correction, do only if tree is available
	MCGenTree->Draw(">>elistMCGen", "", "entrylistMCGen"); // TODO include fCut???
	elistMCGen = dynamic_cast<TEntryList*>(gDirectory->Get("elistMCGen"));
	fMCGenTree->SetEntryList(elistMCGen);
	fNMCGenTreeEntries=elistMCGen->GetN();
      }
      
      cout<<"RooHSEventsPDF::SetEvTree "<<GetName()<<" "<<fNTreeEntries<<endl;
      if(MCGenTree) cout<<"RooHSEventsPDF::SetEvTree "<<GetName()<<"__MCGEN "<<fNMCGenTreeEntries<<endl;

      //Read weights into fEvWeights
      TBranch* idBranch=nullptr;
      vector<Long64_t> idEntries;
      
      Long64_t corrEvent=0;
      for(Long64_t iEvent=0;iEvent<fNTreeEntries;iEvent++){
	entryNumber = fEvTree->GetEntryNumber(iEvent);
	if (entryNumber < 0) break;
	localEntry = fEvTree->LoadTree(entryNumber);
	if (localEntry < 0) break;
	fEvTree->GetEntry(localEntry);
	
	Bool_t removeNaNEvent=false;//in case of NaN

	for(UInt_t ip=0;ip<ProxSize;ip++){
	  if( TMath::IsNaN(MCVar[ip]) ){
	    //this event contains a NaN so will ignore
	    cout<<"RooHSEventsPDF::SetEvTree "<<GetName()<<" event with NaN will  remove it "<< localEntry<<endl;
	    removeNaNEvent=true;
	  }
	  else{
	    fvecReal[corrEvent*ProxSize+ip]=MCVar[ip];
	    //Read the generated values if exist if not
	    //use mcvar again, this duplicates data so should
	    //be better optimised
	    if(!fGotGenVar[ip]) fvecRealGen[corrEvent*ProxSize+ip]=MCVar[ip];
	    else fvecRealGen[corrEvent*ProxSize+ip]=GenVar[ip];
	  }
	}
	if(removeNaNEvent) continue;
	//This event has passed all requirements and we are going to keep it
	fTreeEntryNumber.push_back(localEntry);

	//Get weights if used
	if(fUseEvWeights==kTRUE){ 
	  fInWeights->GetEntryBinarySearch(static_cast<Long64_t>(idVal));
	  fEvWeights[corrEvent]=fInWeights->GetWeight(spId);
	}
	//and any categories
	for(UInt_t ip=0;ip<CatSize;ip++){
	  fvecCat[corrEvent*CatSize+ip]=MCCat[ip];
	  if(!fGotGenCat[ip]) fvecCatGen[corrEvent*CatSize+ip]=MCCat[ip];
	  else fvecCatGen[corrEvent*CatSize+ip]=GenCat[ip];
	}
	corrEvent++;
      }
      //finished data loop
      if(fNTreeEntries!=corrEvent){
	cout<<"RooHSEventsPDF::SetEvTree note only accepted "<<corrEvent<<" out of "<<fNTreeEntries<<" original events"<<endl;
	fNTreeEntries=corrEvent;
      }
      delete fInWeights;fInWeights=nullptr;
      fEvTree->SetEntryList(nullptr);
      delete elist;elist=nullptr;
 
      entryNumber=0;
      localEntry=0;
      if(MCGenTree){// generated events used for acceptance correction, do only if tree is available
	for(Long64_t iEvent=0;iEvent<fNMCGenTreeEntries;iEvent++){
	  entryNumber = fMCGenTree->GetEntryNumber(iEvent);
	  if (entryNumber < 0)
	    break;
	  localEntry = fMCGenTree->LoadTree(entryNumber);
	  if (localEntry < 0)
	    break;
	  fMCGenTree->GetEntry(localEntry);
	  for(UInt_t ip=0;ip<ProxSize;ip++){
	    //  cout<<iEvent<<" "<<MCVar[ip]<<endl;
	    fvecRealMCGen[iEvent*ProxSize+ip]=MCGenVar[ip];
	  }
	  for(UInt_t ip=0;ip<CatSize;ip++){
	    fvecCatMCGen[iEvent*CatSize+ip]=MCGenCat[ip];
	  }
	}
	fMCGenTree->SetEntryList(nullptr);
	delete elistMCGen;elistMCGen=nullptr;
      }
      
      /* 
      //Read weights into fEvWeights
      if(fWgtsConf.IsValid()){
    
	fEvWeights.clear();
	LoadInWeights();
    
	if(fEvTree->GetBranch(fInWeights->GetIDName())){ //the weight ID branch is in fEvTree
	  fUseEvWeights=kTRUE;
	  fEvTree->SetBranchStatus(fInWeights->GetIDName(),true);
	  TLeaf* idleaf=dynamic_cast<TLeaf*>(fEvTree->GetBranch(fInWeights->GetIDName())->GetListOfLeaves()->First());
	  if(!idleaf) { cout<<"ERROR RooHSEventsPDF::SetEvTree weights id branch "<<fInWeights->GetIDName()<<" is not part of event tree "<<endl; fEvTree->Print();exit(1);}
	  auto idbranch=fEvTree->GetBranch(fInWeights->GetIDName());
	  auto spId=fInWeights->GetSpeciesID(fWgtsConf.Species());
	  fEvWeights.resize(fNTreeEntries);
	  for(Long64_t iw=0;iw<fNTreeEntries;iw++){
	    idbranch->GetEntry(iw);
	    fInWeights->GetEntryBinarySearch(static_cast<Long64_t>(idleaf->GetValue()));
	    fEvWeights[iw]=fInWeights->GetWeight(spId);
	  }
	}
	else cout<<"WARNING RooHSEventsPDF::SetEvTree InWeights ID : "<<fInWeights->GetIDName()<<" does not exist in event tree"<<endl;
	delete fInWeights;fInWeights=nullptr;
      }
      */
   
      if(dynamic_cast<TChain*>(fEvTree)){
	TTree* coptree=fEvTree->CloneTree(0);//convert chain to tree
	fEvTree=coptree;
      }
      fEvTree->Reset();  //empty tree to save memory
      if(MCGenTree)
	fMCGenTree->Reset();
      return fBranchStatus;
    }
    
    void  RooHSEventsPDF::LoadInWeights(){
      //GetWeights object 
      cout<<"void RooHSEventsPDF::LoadWeights and use species "<< fWgtsConf.Species()<<" "<<fWgtsConf.File()<<" "<<fWgtsConf.ObjName()<<endl;
      if(fInWeights) delete fInWeights;
      fInWeights=nullptr;
      fInWeights=new Weights();
      fInWeights->LoadSaved(fWgtsConf.File(),fWgtsConf.ObjName());
      // fWgtSpecies = fWgtsConf.Species();
      if(fInWeights->GetSpeciesID(fWgtsConf.Species())==-1){
	cout<<"ERROR RooHSEventsPDF::LoadInWeights() requested species "<<fWgtsConf.Species()<<" not found in given weights"<<endl;
	fInWeights->Print();
	fIsValid=kFALSE;
      }
    }

    void  RooHSEventsPDF::CheckIntegralParDep(Int_t Ntests){
      fCheckInt=Ntests;
      if(!fEvTree) return; //will check later when tree is set
      if(!fBranchStatus) return; //missing branches may need to add protodata
  
      Long64_t saveNint=fNInt;
      // fNInt=nint;//REOMVE FOR NOW nint is not used
      fNInt=fNTreeEntries;//just use all entries
      //scale Ntests by Ndimensions
      Ntests=TMath::Power((Double_t)Ntests,(Double_t)fParSet.size());

      Info("RooHSEventsPDF::CheckIntegralParDep","Going to run %d calculations of integral with random parameters",Ntests);
  
      RooRealVar integral("integral","integral",0,0,2);
      integral.setError(sqrt(fNInt)/fNInt); //Error needs to be set before entering in ds
      RooDataSet ds("intds","intds",RooArgSet(integral));
      //want to set random paramter values
      //loop over each paramter and calculate integral
      vector<Double_t> SavedPars;
      for(auto & ip : fParSet){//first save parameters
        auto par=(dynamic_cast<const RooRealVar*>(&(ip->arg())));
	SavedPars.push_back(par->getValV());
      }
      for(Int_t ir=0;ir<Ntests;ir++){ //loop over tests
	for(auto & ip : fParSet){//loop over parameters
	  auto par=(RooRealVar*)(&(ip->arg()));
	  par->setVal((par->getMax("")-par->getMin(""))*RooRandom::uniform()+par->getMin(""));
	}
	//Now calculate integral
	integral.setVal(analyticalIntegral(1,""));
	ds.add(RooArgSet(integral));
      }

 
      Double_t low=0;
      Double_t high=0;
      ds.getRange(integral,low,high);
      integral.setRange(low,high);
      RooPlot *frame=integral.frame();
      ds.plotOn(frame);

      frame->Draw();

      RooRealVar  mean("mean","mean",ds.mean(integral));
      RooRealVar pvar("IntPull","Integral Pull Dist.",-5,5);
      RooPullVar pull("IntPull","Integral Pull Dist.",integral,mean);
      ds.addColumn(pull,kFALSE);

      ds.Print();
      ds.getRange(pvar,low,high);
      pvar.setRange(low,high);
      RooPlot *framePull=pvar.frame();
      ds.plotOn(framePull);
      RooRealVar mp("mp","mp",0,-5,5) ;
      RooRealVar sp("sp","sp",1,0,100);
      RooGaussian gp("gp","gp",pvar,mp,sp);
      gp.fitTo(ds);
      gp.paramOn(framePull);
      gp.plotOn(framePull);
  
      pvar.Print();
      new TCanvas();
      framePull->Draw();
  
 
      //numerical check gives constant integral, can force const for fit speed
      fConstInt=mean.getVal();
      fNInt=saveNint;
      if(sp.getVal()<2){
	Info("RooHSEventsPDF::CheckIntegralParDep","Numerical integral constant. Will not recalculate during fits to save time, if you want to force calculation do not call this function");
	SetConstInt();
      }
      //reset pars
      for(UInt_t ip=0;ip<fParSet.size();ip++){//first save parameters
	auto par=(RooRealVar*)(&(fParSet[ip]->arg()));
	par->setVal(SavedPars[ip]);
      }
      fCheckInt=kFALSE; //only do once
    }
    Bool_t RooHSEventsPDF::AddProtoData(const RooDataSet* data){
      //merge the current tree with data from another dataset
      //Default it will add any branches in data not in fEvTree
      //      cout<<"RooHSEventsPDF::AddProtoData "<<data<<" "<<fEvTree<<endl;
      if(!fEvTree) return kFALSE;
      if(!fNTreeEntries) return kFALSE;
  
      //Loop over data branches and check if any missing
      const RooArgSet *dataVars=data->get();
      RooArgSet thisVars =VarSet(0);
  
      //Each entry in fEvTree should be given a random value of new variables
      Long64_t Nentries=data->numEntries();
      vector<Long64_t> vrandom(Nentries);
      for(Long64_t ir=0;ir<Nentries;ir++)
	vrandom[ir]=ir;
      std::shuffle(vrandom.begin(),vrandom.end(), std::mt19937(std::random_device()()));

      TIter iter=dataVars->createIterator();
      vector<Double_t> brD;
      vector<Int_t> brI;
      Int_t ND=0;
      Int_t NI=0;
      vector<Bool_t> isD;
      vector<Int_t> index;
      TObjArray branches;
      //Add new branches to EvTree
      TDirectory* saveDir=gDirectory;
      fEvTree->GetDirectory()->cd();

      vector<Short_t> protoDataForVar;
      vector<Short_t> protoDataForCat;
  
      //Look for variables in data that were not in fEvTree
      while(auto* arg=dynamic_cast<RooAbsArg*>(iter())){
	if(TString("UID")==arg->GetName()) continue; //don't replicate ID branch
	if(fEvTree->GetBranch(arg->GetName())) continue; //already exists
	for(Int_t ip=0;ip<fNvars;ip++)
	  if(TString(arg->GetName())==TString(fProxSet[ip]->GetName()))
	    protoDataForVar.push_back(ip);
	for(Int_t ip=0;ip<fNcats;ip++)
	  if(TString(arg->GetName())==TString(fCatSet[ip]->GetName()))
	    protoDataForCat.push_back(ip);
	Info("RooHSEventsPDF::AddProtoData","Added data branch %s",arg->GetName());
      }


      //Loop over data in random order and fill new branches
      Long64_t idata=0;

      UInt_t Nreal=protoDataForVar.size();
      UInt_t Ncat=protoDataForCat.size();
  
      if(!(Nreal+Ncat)) return kTRUE;
  
      //copy proto data to vecs
      for(Long64_t id=0;id<fNTreeEntries;id++){
	dataVars=data->get(vrandom[idata]);
	for(short ip : protoDataForVar){
	  Double_t val=dataVars->getRealValue(fProxSet[ip]->GetName());
	  fvecReal[id*fNvars+ip]=val;
	  fvecRealGen[id*fNvars+ip]=val;
	}  
	for(short ip : protoDataForCat){
	  Int_t val=dataVars->getCatIndex(fCatSet[ip]->GetName());
	  fvecCat[id*fNcats+ip]=val;
	  fvecCatGen[id*fNcats+ip]=val;     
	}
    
	if(idata==(Long64_t)vrandom.size()-1){//Need to reuse data until done all MC events
	  std::shuffle(vrandom.begin(),vrandom.end(), std::mt19937(std::random_device()()));
	  idata=0;
	}
	idata++;
      }
  
      saveDir->cd();
  
      return fBranchStatus;  
    }
    void RooHSEventsPDF::ResetTree(){
  
      if(fEvTree) {delete fEvTree;fEvTree=nullptr;}
    }

    void RooHSEventsPDF::SetNextRange(Int_t ir){
      Long64_t Nentries=fNTreeEntries;
      Int_t range=((Double_t)Nentries)/fNRanges;

      fIntRangeLow=ir*range;
      fIntRangeHigh=(ir+1)*range;

    }
  }
}
