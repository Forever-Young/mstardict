/*
 *  MStarDict - International dictionary for Maemo.
 *  Copyright (C) 2010 Roman Moravcik
 *
 *  base on code of stardict:
 *  Copyright (C) 2003-2007 Hu Zheng <huzheng_001@163.com>
 *
 *  based on code of sdcv:
 *  Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
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
class MStarDictConf;
class MStarDict;

extern MStarDict *pMStarDict;

class MStarDict {
  private:
    GtkWidget *main_window;
    GtkWidget *label_widget;
    GtkWidget *results_widget;
    GtkWidget *search;
    GtkWidget *results_view;
    GtkWidget *results_view_scroll;

    GtkListStore *results_list;

    Library *oLibs;
    MStarDictConf *oConf;
    TSearchResultList results;

    static gboolean onResultsViewSelectionChanged(GtkTreeSelection *selection,
						  MStarDict *mStarDict);
    static gboolean onSearchEntryChanged(GtkEditable *editable,
					 MStarDict *mStarDict);
    static gboolean onDictionariesMenuItemClicked(GtkButton *button,
						  MStarDict *mStarDict);
    static gboolean onQuitMenuItemClicked(GtkButton *button,
					  MStarDict *mStarDict);

  public:
     MStarDict();
    ~MStarDict();

    void CreateTranslationWindow(const gchar *bookname,
				 const gchar *def,
				 const gchar *exp);
    void CreateMainWindow();
    void CreateMainMenu();

    void GetAllDictionaryList(std::list < std::string > &dict_list);
    void LoadDictionaries();
    void ReLoadDictionaries(std::list < std::string > &dict_list);

    void ResultsListClear();
    void ResultsListInsertLast(const gchar *word);
    void ReScroll();
    void ShowNoResults(bool bNoResults);
    void ShowNoDictionary(bool bNoDictionary);
    void ShowProgressIndicator(bool bShow);
};
