#ifndef _LIBWRAPPER_HPP_
#define _LIBWRAPPER_HPP_

#include <string>
#include <vector>

#include "file.hpp"
#include "lib.h"

using std::string;
using std::vector;

//this structure is wrapper and it need for unification
//results of search whith return Dicts class
struct TSearchResult {
	string bookname;
	string def;
	string exp;

	TSearchResult(const string& bookname_, const string& def_, const string& exp_)
		: bookname(bookname_), def(def_), exp(exp_)
		{
		}
};

typedef vector<TSearchResult> TSearchResultList;
typedef TSearchResultList::iterator PSearchResult;

//this class is wrapper around Dicts class for easy use
//of it
class Library : public Libs {
public:
	Library();
	~Library();

	std::vector<InstantDictIndex> query_dictmask;
	CurrentIndex *iCurrentIndex;

	void ListWords(CurrentIndex* iIndex);
	bool BuildResultData(std::vector<InstantDictIndex> &dictmask, const char* sWord, CurrentIndex *iIndex, int iLib, TSearchResultList& res_list);

	bool SimpleLookup(const gchar* sWord, CurrentIndex* piIndex);
private:
	void LookupWithFuzzy(const string &str, TSearchResultList& res_list);
	void LookupWithRule(const string &str, TSearchResultList& res_lsit);
	void LookupData(const string &str, TSearchResultList& res_list);
};

#endif//!_LIBWRAPPER_HPP_
