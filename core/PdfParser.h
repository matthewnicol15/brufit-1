////////////////////////////////////////////////////////////////
///
///Class:               PdfParser
///Description:  Create a FactoryPDF and its required
///              parameter list, function list, formula list
///

#pragma once
#include <regex>
#include <utility>
#include <utility>
#include <vector>
#include <map>
#include <string>
#include <iostream>


namespace HS{
  namespace FIT{

    using std::vector;
    using std::string;

    struct SumIndex;
    using SumIndices=vector<SumIndex>;
    
    using strings_std = std::vector<string>;
    
    class PdfParser {
      
      
    public:
    PdfParser(string name):_name(std::move(std::move(name))){};
      
      virtual string ConstructPDF(string str)=0;

      //Deal with Summations
      string ReplaceSummations(string str);
      string ExpandSummation(string str);
      SumIndices GetSumIndices(string str);
      string SumOverIndex(string subject, SumIndices sumIndices,uint Ni);
 
      //Complex summation
      string ReplaceComplexSumSqd(string str);
      
      //Parameter,Functions, Formulas...
      void AddParameter(string par);
      void AddConstant(const string& constant);
      void AddFunction(string fun);
      void AddFormula(const string& form);
      bool CheckFormulaList(const string& fun);
      bool CheckFunctionList(const string& fun);
      bool CheckParameterList(const string& par);
      bool CheckConstantsList(const string& par);

      void AddFunctionTemplate(string func,string temp);
      void AddComplexFunctionTemplate(const string& func,string temp);

      const strings_std GetParameters() const {return _parList;}
      const strings_std GetConstants() const {return _constList;}
      const strings_std GetFunctions() const {return _funList;}
      const strings_std GetFormulas() const {return _formList;}

      string GetName(){return _name;}
      string GetPDF(){return _pdfString;}

      void SetVars(const string& vars){_varsString="{"+vars+"}";}
      
      void ParseTerm(string term);
      
    protected :

      strings_std _parList;
      strings_std _constList;
      strings_std _funList;
      strings_std _formList;
      strings_std _complexArgs;

      std::map<string,string> _funNames; //map full strings to just names
      
      string _name;
      string _pdfString;
      string _varsString;
      std::vector<std::pair<std::string,std::regex>> _functionTemplates;

      
      
    };//class PdfParser

    //struct to configure summation indexes
    struct SumIndex{
      string _label; //summation index
      vector<string> _maxdep; //can depend on other index
      vector<string> _mindep; //can depend on other index
      vector<string> _notequaldep; //depnd on another index
      
      vector<int>_vals; //values of summation index
      int _currval=0; //keep track in case others depend on this

      int _entry=-1;
      bool next(){
	if(_entry+1==(int)_vals.size()){
	  _entry=-1;
	  _currval=_vals[0];
	  return false;
	}
	_entry++;
	_currval=_vals[_entry];
	return true;
      }
    };
    //class to consolidate all summation indexes into 1
    class ConsolidateIndex{
    public :
      ConsolidateIndex(const SumIndices indices);

      //vector of index labels with current values
      std::vector<std::pair<string,int>> next();
      bool getValues(int Nl);
    private :
      SumIndices _indices;
      std::vector<std::string> _labels; //summation indexes
      std::vector<std::vector<int>>_vals; //values of summation for each index
      std::vector<int> _tempVals;
      int _entry=0;
    };
   
    ///////////////////////////////////////////////////////////////////////////////
    ///string helper functions
    vector<string> Tokenize(const string& str,const char symbol);
    bool StringContainsChar(const string& str,const char symbol);
    bool StringContainsString(const string& str1,const string& str2);
    string WithinBrackets(const string expr,const char bracket);
    string StringReplaceFirst(string str,const string& s1,const string& s2);
    string StringReplaceAll(string str,const string& s1,const string& s2);
    string StringToNext(const string& str,size_t &pos,const string& s1);
    int StringWhichIsNext(const string& str, size_t &pos, const string& s1,const string& s2,const string& s3=string());
    string StringToNext(string str,string s1);
    string StringBetweenFirst(const string& str,const string& s1,const string& s2);
    bool StringIsNumber(string str);
    void PrintStrings(const strings_std strs);

     SumIndex* GetIndex(const string& label,SumIndices indices);
    int EvalIndiceFormula(string formula,SumIndices &indices);
  }
}
