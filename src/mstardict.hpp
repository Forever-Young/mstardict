/*
 *  MStarDict - International dictionary for Maemo.
 *  Copyright (C) 2010 Roman Moravcik
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include <hildon/hildon.h>

class Library;
class MStarDict;

extern MStarDict *pMStarDict;

class MStarDict {
private:
	GtkWidget *label_widget;
	GtkWidget *results_widget;
	GtkWidget *search;
	GtkWidget *results_view;
	GtkWidget *results_view_scroll;

	GtkListStore *results_list;

	Library *oLibs;
	TSearchResultList results;

	static gboolean onResultsViewSelectionChanged (GtkTreeSelection *selection, MStarDict *mStarDict);
	static gboolean onSearchEntryChanged (GtkEditable *editable, MStarDict *mStarDict);

public:
	MStarDict();
	~MStarDict();

	void CreateTranslationWindow(const gchar *bookname, const gchar *def, const gchar *exp);
	void CreateMainWindow();

	void LoadDictionaries();

	void ResultsListClear();
	void ResultsListInsertLast(const gchar *word);
	void ReScroll();
};
