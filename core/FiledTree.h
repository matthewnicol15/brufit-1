////////////////////////////////////////////////////////////////
///
///Class:               FiledTree
///Description:
///            A tree connected to its own TFile !
///            An attempt to stop file directory related crashes!
///            Note in interactive root session must create a new
///            object so we can call delete. If you let ROOT delete
///            after .q the file will not be writeable!
 
#pragma once

#include <TTree.h>
#include <TFile.h>
#include <TString.h>
#include <iostream>

namespace HS{
  namespace FIT{
  
    using std::cout;
    using std::endl;

    using tfile_ptr=std::shared_ptr<TFile>;
    using ttree_ptr=std::shared_ptr<TTree>;

    class FiledTree  {
      
  
      enum class Mode_t{null,recreate,create,read,update,copyempty,copyfull};
    
      using filed_ptr=std::shared_ptr<HS::FIT::FiledTree>;
      using filed_uptr=std::unique_ptr<HS::FIT::FiledTree>;

    public:
      //////////////////////////////////////////////////////////////////////
      ///Construct a new tree to be branched and filled
      FiledTree(const TString& tname,const TString& fname,const TString& opt="recreate"){
	fFile.reset(TFile::Open(fname,opt));
	fTree.reset(new TTree(tname,"A FiledTree"));
	SetTreeDirectory();
      }
      //////////////////////////////////////////////////////////////////////
      ///Construct a tree based on an existing one
      ///if isfull is true copy all the events, if false just create empty tree
      FiledTree(Bool_t isfull,TTree *tree,const TString& fname,const TString& opt="recreate"){
	fFile.reset(TFile::Open(fname,opt));
	if(isfull)fTree.reset(tree->CloneTree(-1,"fast"));
	else fTree.reset(tree->CloneTree(0));
	SetTreeDirectory();
      }
      // FiledTree()=default;
      FiledTree(const FiledTree&)=default;
      FiledTree(FiledTree&&)=default;
      FiledTree& operator=(const FiledTree& other)=default;
      FiledTree& operator=(FiledTree&& other) = default;

      virtual ~FiledTree();
    
      const ttree_ptr Tree() const {return fTree;}
      void Fill(){Tree()->Fill();}

      static filed_uptr Recreate(const TString tname,const TString fname);
      static filed_uptr Create(const TString tname,const TString fname);
      static filed_uptr Read(const TString& tname,const TString& fname);
      static filed_uptr Update(const TString& tname,const TString& fname);
      static filed_uptr CloneEmpty(TTree* tree,const TString fname);
      static filed_uptr CloneFull(TTree* tree,const TString fname);
      static filed_uptr CloneEmpty(const ttree_ptr& tree,const TString fname);
      static filed_uptr CloneFull(const ttree_ptr& tree,const TString fname);
      static filed_uptr RecreateCopyFull(TTree* tree,const TString& fname,const TString& selection="",const Long64_t nentries = TTree::kMaxEntries);
      static filed_uptr RecreateCopyFull(const ttree_ptr& tree,const TString& fname,const TString& selection="",const Long64_t nentries = TTree::kMaxEntries);

      void SetMode(const Mode_t m){fMode=m;}
      Mode_t Mode(){return fMode;}
    
    protected :

      TFile* File() const {return fFile.get();}
      void SetTreeDirectory(){ Tree()->SetDirectory(File());}
      void CreateFile(const TString& fname,const TString& opt){ fFile.reset(TFile::Open(fname,opt));}
      void CreateTree(const TString& tname){fTree.reset(new TTree(tname,"A FiledTree"));}
      void SetFile(TFile* f){ fFile.reset(f);}
      void SetTree(TTree* t){ fTree.reset(t);}

    private:
      tfile_ptr fFile;//file before tree as is owner
      ttree_ptr fTree;
      Mode_t fMode=Mode_t::null;

      FiledTree()=default;
    
    };
  
    using filed_ptr=std::shared_ptr<HS::FIT::FiledTree>;
    using filed_uptr=std::unique_ptr<HS::FIT::FiledTree>;
    using filed_shptr=std::shared_ptr<HS::FIT::FiledTree>;

  }
}
