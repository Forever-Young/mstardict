/* 
 * This file part of sdcv - console version of Stardict program
 * http://sdcv.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <map>

#include "libwrapper.hpp"
#include "mstardict.hpp"

static std::string xdxf2text(const char *p)
{
	std::string res;
	for (; *p; ++p) {
		if (*p!='<') {
			if (g_str_has_prefix(p, "&gt;")) {
				res+=">";
				p+=3;
			} else if (g_str_has_prefix(p, "&lt;")) {
				res+="<";
				p+=3;
			} else if (g_str_has_prefix(p, "&amp;")) {
				res+="&";
				p+=4;
			} else if (g_str_has_prefix(p, "&quot;")) {
				res+="\"";
				p+=5;
			} else
				res+=*p;
			continue;
		}

		const char *next=strchr(p, '>');
		if (!next)
			continue;

		std::string name(p+1, next-p-1);

		if (name=="abr")
			res+="";
		else if (name=="/abr")
			res+="";
		else if (name=="k") {
			const char *begin=next;
			if ((next=strstr(begin, "</k>"))!=NULL)
				next+=sizeof("</k>")-1-1;
			else
				next=begin;
		} else if (name=="b")
			res+="";
		else if (name=="/b")
			res+="";
		else if (name=="i")
			res+="";
		else if (name=="/i")
			res+="";
		else if (name=="tr")
			res+="[";
		else if (name=="/tr")
			res+="]";
		else if (name=="ex")
			res+="";
		else if (name=="/ex")
			res+="";
		else if (!name.empty() && name[0]=='c' && name!="co") {
			std::string::size_type pos=name.find("code");
			if (pos!=std::string::size_type(-1)) {
				pos+=sizeof("code=\"")-1;
				std::string::size_type end_pos=name.find("\"");
				std::string color(name, pos, end_pos-pos);
				res+="";
			} else {
				res+="";
			}
		} else if (name=="/c")
			res+="";

		p=next;
	}
	return res;
}

static string parse_data(const gchar *data)
{
	if (!data)
		return "";

	string res;
	guint32 data_size, sec_size=0;
	gchar *m_str;
	const gchar *p=data;
	data_size=*((guint32 *)p);
	p+=sizeof(guint32);
	while (guint32(p - data)<data_size) {
		switch (*p++) {
		case 'g':
		case 'm':
		case 'l': //need more work...
			sec_size = strlen(p);
			if (sec_size) {
				res+="\n";
				m_str = g_strndup(p, sec_size);
				res += m_str;
				g_free(m_str);
			}
			sec_size++;
			break;
		case 'x':
			sec_size = strlen(p);
			if (sec_size) {
				res+="\n";
				m_str = g_strndup(p, sec_size);
				res += xdxf2text(m_str);
				g_free(m_str);
			}
			sec_size++;
			break;
		case 't':
			sec_size = strlen(p);
			if(sec_size){
				res+="\n";
				m_str = g_strndup(p, sec_size);
				res += "["+string(m_str)+"]";
				g_free(m_str);
			}
			sec_size++;
			break;
		case 'y':
			sec_size = strlen(p);
			sec_size++;				
			break;
		case 'W':
		case 'P':
			sec_size=*((guint32 *)p);
			sec_size+=sizeof(guint32);
			break;
		}
		p += sec_size;
	}

	return res;
}

void Library::ListWords(CurrentIndex* iIndex)
{
	CurrentIndex *iCurrent = (CurrentIndex*)g_memdup(iIndex, sizeof(CurrentIndex)*query_dictmask.size());

	pMStarDict->ResultsListClear();

	int iWordCount=0;
	const gchar * poCurrentWord = poGetCurrentWord(iIndex, query_dictmask, 0);
	if (poCurrentWord) {
		pMStarDict->ResultsListInsertLast(poCurrentWord);
		iWordCount++;

		while (iWordCount < 30 && (poCurrentWord = poGetNextWord(NULL, iIndex, query_dictmask, 0))) {
			pMStarDict->ResultsListInsertLast(poCurrentWord);
			iWordCount++;
		}
	}
	pMStarDict->ReScroll();

	if (iCurrent)
		g_free (iCurrent);
}

bool Library::BuildResultData(std::vector<InstantDictIndex> &dictmask, const char* sWord, CurrentIndex *iIndex, int iLib, TSearchResultList& res_list)
{
	int iRealLib;
	bool bFound = false, bLookupWord = false, bLookupSynonymWord = false;
	gint nWord=0, count=0, i=0, j=0;

	iRealLib = dictmask[iLib].index;

	bLookupWord = LookupWord(sWord, iIndex[iLib].idx, iIndex[iLib].idx_suggest, iRealLib, 0);
	if (!bLookupWord)
		bLookupWord = LookupSimilarWord(sWord, iIndex[iLib].idx, iIndex[iLib].idx_suggest, iRealLib, 0);
	if (!bLookupWord)
		bLookupWord = SimpleLookupWord(sWord, iIndex[iLib].idx, iIndex[iLib].idx_suggest, iRealLib, 0);

	bLookupSynonymWord = LookupSynonymWord(sWord, iIndex[iLib].synidx, iIndex[iLib].synidx_suggest, iRealLib, 0);
	if (!bLookupSynonymWord)
		bLookupSynonymWord = LookupSynonymSimilarWord(sWord, iIndex[iLib].synidx, iIndex[iLib].synidx_suggest, iRealLib, 0);
	if (!bLookupSynonymWord)
		bLookupSynonymWord = SimpleLookupSynonymWord(sWord, iIndex[iLib].synidx, iIndex[iLib].synidx_suggest, iRealLib, 0);

	g_debug ("bookname: %s, iLib: %d, iRealLib: %d, str: %s", dict_name(iLib).c_str(), iLib, iRealLib, sWord);

	if (bLookupWord || bLookupSynonymWord) {
		if (bLookupWord)
			nWord++;

		if (bLookupSynonymWord)
			nWord+=GetOrigWordCount(iIndex[iLib].synidx, iRealLib, false);

		if (bLookupWord) {
			count = GetOrigWordCount(iIndex[iLib].idx, iRealLib, true);
			for (i=0;i<count;i++) {
				res_list.push_back(TSearchResult(dict_name(iLib),
								 poGetWord(iIndex[iLib].idx, iRealLib, 0),
								 parse_data(poGetOrigWordData(iIndex[iLib].idx+i, iRealLib))));
			}
			i = 1;
		} else {
			i = 0;
		}
		for (j = 0; i < nWord; i++, j++) {
				res_list.push_back(TSearchResult(dict_name(iLib),
								 poGetWord(iIndex[iLib].synidx+j, iRealLib, 0),
								 parse_data(poGetOrigWordData(iIndex[iLib].synidx+j, iRealLib))));
		}

		bFound = true;
	}

	return bFound;
}

bool Library::SimpleLookup(const gchar* sWord, CurrentIndex* piIndex)
{
	CurrentIndex *iIndex;
	TSearchResultList results;
	bool bFound = false;

	if (!piIndex)
		iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * query_dictmask.size());
	else
		iIndex = piIndex;

	for (size_t iLib=0; iLib<query_dictmask.size(); iLib++) {
		bFound = BuildResultData(query_dictmask, sWord, iIndex, iLib, results);
	}

	if (!piIndex)
		g_free(iIndex);

	return bFound;
}

void Library::LookupWithFuzzy(const gchar* sWord)
{
	static const int MAX_FUZZY_MATCH_ITEM=100;
	gchar *fuzzy_reslist[MAX_FUZZY_MATCH_ITEM];
	bool bFound = false;

	pMStarDict->ResultsListClear();

	bFound = Libs::LookupWithFuzzy(sWord, fuzzy_reslist, MAX_FUZZY_MATCH_ITEM, query_dictmask);
	if (bFound) {
		SimpleLookup(fuzzy_reslist[0], iCurrentIndex);

		for (int i=0; i<MAX_FUZZY_MATCH_ITEM && fuzzy_reslist[i]; i++) {
			pMStarDict->ResultsListInsertLast(fuzzy_reslist[i]);
			g_free(fuzzy_reslist[i]);
		}
		pMStarDict->ReScroll();
	}
}

void Library::LookupWithRule(const gchar* sWord)
{
	gint iMatchCount=0;
	gchar **ppMatchWord = (gchar **)g_malloc(sizeof(gchar *) * (MAX_MATCH_ITEM_PER_LIB) * query_dictmask.size());

	pMStarDict->ResultsListClear();

	iMatchCount=Libs::LookupWithRule(sWord, ppMatchWord, query_dictmask);
	if (iMatchCount) {
		for (gint i=0; i<iMatchCount; i++)
			pMStarDict->ResultsListInsertLast(ppMatchWord[i]);

		SimpleLookup(ppMatchWord[0], iCurrentIndex);
		pMStarDict->ReScroll();

		for(gint i=0; i<iMatchCount; i++)
			g_free(ppMatchWord[i]);
	}
	g_free(ppMatchWord);
}

void Library::LookupData(const string &str, TSearchResultList& res_list)
{
	bool cancel = false;
	std::vector<gchar *> drl[query_dictmask.size()];
	if (!Libs::LookupData(str.c_str(), drl, NULL, NULL, &cancel, query_dictmask))
		return;
	for (size_t iLib=0; iLib<query_dictmask.size(); iLib++)
		for (std::vector<gchar *>::size_type j=0; j<drl[iLib].size(); ++j) {
//			SimpleLookup(drl[iLib][j], res_list);
			g_free(drl[iLib][j]);
		}
}

Library::Library () : Libs (NULL, FALSE, 0, 0)
{
	iCurrentIndex = NULL;
}

Library::~Library ()
{
	if (iCurrentIndex)
		g_free (iCurrentIndex);
}