{

TFile *f = new TFile("/home/matthewn/Chanser/Final_States/KpKm/Output_fall2018_skim3_Outbending_290921_correct_branches/matthewn/KpKm_ALLALL_configuration1/FinalState.root");
TTree *t1 = (TTree*)f->Get("FINALOUTTREE");

t1->SetAlias("KpKmPol","Pol");




}
