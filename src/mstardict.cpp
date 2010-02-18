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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <clocale>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>
#include <hildon/hildon.h>

#include <getopt.h>
#include <string>
#include <vector>
#include <memory>

#include "libwrapper.hpp"
#include "file.hpp"
#include "mstardict.hpp"

MStarDict *pMStarDict;

enum {
	DEF_COLUMN,
	N_COLUMNS
};

MStarDict::MStarDict ()
{
	label_widget = NULL;
	results_widget = NULL;
	results_view = NULL;
	results_view_scroll = NULL;

	/* create list of ressults */
	results_list = gtk_list_store_new (N_COLUMNS,
					   G_TYPE_STRING);	/* DEF_COLUMN */

	/* initialize stardict library */
	oLibs = new Library ();
}

MStarDict::~MStarDict ()
{
	/* destroy list of results */
	g_object_unref (results_list);

	/* deinitialize stardict library */
	delete oLibs;
}

gboolean
MStarDict::onResultsViewSelectionChanged (GtkTreeSelection *selection,
					      MStarDict *mStarDict)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *bookname, *def, *exp;
	const gchar *sWord;
	bool bFound = false;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		/* unselect selected rows */
		gtk_tree_selection_unselect_all (selection);

		gtk_tree_model_get (model, &iter, DEF_COLUMN, &sWord, -1);

		/* clear previous search results */
		mStarDict->results.clear();

		for (size_t iLib=0; iLib<mStarDict->oLibs->query_dictmask.size(); iLib++) {
			bFound = mStarDict->oLibs->BuildResultData(mStarDict->oLibs->query_dictmask,
								   sWord,
								   mStarDict->oLibs->iCurrentIndex,
								   iLib,
								   mStarDict->results);
		}

		bookname = g_markup_printf_escaped ("<span color=\"dimgray\" size=\"x-small\">%s</span>",
						    mStarDict->results[0].bookname.c_str());
		def = g_markup_printf_escaped ("<span color=\"darkred\" weight=\"heavy\" size=\"large\">%s</span>",
					       mStarDict->results[0].def.c_str());
		exp = g_strdup (mStarDict->results[0].exp.c_str());

		/* create translation window */
		mStarDict->CreateTranslationWindow (bookname, def, exp);

		g_free (bookname);
		g_free (def);
		g_free (exp);
	}

	/* grab focus to search entry */
	gtk_widget_grab_focus (GTK_WIDGET (mStarDict->search));

	return TRUE;
}

gboolean
MStarDict::onSearchEntryChanged (GtkEditable *editable,
				    MStarDict *mStarDict)
{
	GtkTreeSelection *selection;
	const gchar *sWord;
	bool bFound = false;
	std::string query;

	sWord = gtk_entry_get_text (GTK_ENTRY (editable));

	if (strcmp (sWord, "") == 0) {
		gtk_widget_show (mStarDict->label_widget);
		gtk_widget_hide (mStarDict->results_widget);
	} else {
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mStarDict->results_view));
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

		/* unselect rows */
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mStarDict->results_view));
		gtk_tree_selection_unselect_all (selection);

		switch (analyse_query(sWord, query)) {
			case qtFUZZY:
				g_debug ("FUZZY");
				mStarDict->oLibs->LookupWithFuzzy(sWord);
				break;
			case qtREGEX:
				g_debug ("REGEX");
				mStarDict->oLibs->LookupWithRule(sWord);
				break;
			case qtSIMPLE:
				g_debug ("SIMPLE");
				bFound = mStarDict->oLibs->SimpleLookup(sWord, mStarDict->oLibs->iCurrentIndex);
				if (!bFound) {
					const gchar *sugWord = mStarDict->oLibs->GetSuggestWord(sWord, mStarDict->oLibs->iCurrentIndex, mStarDict->oLibs->query_dictmask, 0);
					if (sugWord) {
						gchar *sSugWord = g_strdup(sugWord);
						bFound = mStarDict->oLibs->SimpleLookup(sSugWord, mStarDict->oLibs->iCurrentIndex);
						g_free(sSugWord);
					}
				}
				mStarDict->oLibs->ListWords(mStarDict->oLibs->iCurrentIndex);
				break;
			case qtDATA:
				g_debug ("DATA");
//				oLibs->LookupData(query, res_list);
				break;
			default:
				g_debug ("DEFAULT");
				/*nothing*/;
			}

		/* unselect selected rows */
		gtk_tree_selection_unselect_all (selection);
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

		gtk_widget_hide (mStarDict->label_widget);
		gtk_widget_show (mStarDict->results_widget);
	}

	return TRUE;
}

class GetAllDictList {
public:
	GetAllDictList(std::list<std::string> &dict_all_list_) :
					dict_all_list(dict_all_list_) {}
	void operator()(const std::string& url, bool disable) {
		dict_all_list.push_back(url);
	}
private:
	std::list<std::string> &dict_all_list;
};

void
MStarDict::LoadDictionaries ()
{
	strlist_t dicts_dir_list;
	strlist_t order_list;
	strlist_t disable_list;

	std::list<std::string> load_list;

	/* dictionary directory */
	dicts_dir_list.push_back (std::string ("/home/user/MyDocs/mstardict"));

	for_each_file(dicts_dir_list, ".ifo", order_list, disable_list, GetAllDictList(load_list));
	oLibs->load(load_list);

	oLibs->query_dictmask.clear();
	for (std::list<std::string>::iterator i = load_list.begin(); i != load_list.end(); ++i) {
		size_t iLib;
		if (oLibs->find_lib_by_filename(i->c_str(), iLib)) {
			InstantDictIndex instance_dict_index;
			instance_dict_index.type = InstantDictType_LOCAL;
			instance_dict_index.index = iLib;
			oLibs->query_dictmask.push_back(instance_dict_index);
		}
	}

	if (oLibs->iCurrentIndex)
		g_free (oLibs->iCurrentIndex);
	oLibs->iCurrentIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * oLibs->query_dictmask.size());

}

void
MStarDict::CreateTranslationWindow (const gchar *bookname,
				      const gchar *def,
				      const gchar *exp)
{
	GtkWidget *window, *alignment, *pannable, *vbox, *label;

	window = hildon_stackable_window_new ();
	gtk_window_set_title (GTK_WINDOW (window), _("Translation"));

	alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment),
				   HILDON_MARGIN_DEFAULT,
				   HILDON_MARGIN_DEFAULT,
				   HILDON_MARGIN_DOUBLE,
				   HILDON_MARGIN_DEFAULT);
	gtk_container_add (GTK_CONTAINER (window), alignment);

	pannable = hildon_pannable_area_new ();
	g_object_set (G_OBJECT (pannable), "mov-mode", HILDON_MOVEMENT_MODE_BOTH,
		      NULL);
	gtk_container_add (GTK_CONTAINER (alignment), pannable);

	vbox = gtk_vbox_new (FALSE, 0);
	hildon_pannable_area_add_with_viewport (HILDON_PANNABLE_AREA (pannable),
						vbox);

	label = gtk_label_new ("Bookname");
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), bookname);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new ("Definition");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), def);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new ("Expresion");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	gtk_label_set_markup (GTK_LABEL (label), exp);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	gtk_widget_show_all (window);
}

void
MStarDict::CreateMainWindow ()
{
	HildonProgram *program = NULL;
	GtkWidget *window, *alignment, *vbox;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	/* hildon program */
	program = hildon_program_get_instance ();
	g_set_application_name (_("MStardict"));

	/* main window */
	window = hildon_stackable_window_new ();
	hildon_program_add_window (program, HILDON_WINDOW (window));

	/* aligment */
	alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment),
				   HILDON_MARGIN_HALF,
				   0,
				   HILDON_MARGIN_DEFAULT,
				   HILDON_MARGIN_DEFAULT);
	gtk_container_add (GTK_CONTAINER (window), alignment);

	/* main vbox */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment), vbox);

	/* no_search_result label */
	label_widget = gtk_label_new (_("No search result"));
	hildon_helper_set_logical_color (label_widget, GTK_RC_FG, 
					 GTK_STATE_NORMAL, "SecondaryTextColor");
	hildon_helper_set_logical_font (label_widget, "LargeSystemFont");
	gtk_box_pack_start (GTK_BOX (vbox), label_widget, TRUE, TRUE, 0);

	/* alignment for pannable area */
	results_widget = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (results_widget),
				   0,
				   0,
				   HILDON_MARGIN_DEFAULT,
				   HILDON_MARGIN_DEFAULT);
	gtk_box_pack_start (GTK_BOX (vbox), results_widget, TRUE, TRUE, 0);

	/* pannable for tree view */
	results_view_scroll = hildon_pannable_area_new ();
	gtk_container_add (GTK_CONTAINER (results_widget), results_view_scroll);

	/* result tree view */
	results_view = hildon_gtk_tree_view_new (HILDON_UI_MODE_EDIT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (results_view),
				 GTK_TREE_MODEL (results_list));
	gtk_container_add (GTK_CONTAINER (results_view_scroll), results_view);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (results_view));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (onResultsViewSelectionChanged), this);

	/* def column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (results_view),
						     -1,
						     "Def", renderer,
						     "text", DEF_COLUMN,
						     NULL);
	g_object_set (G_OBJECT (renderer), "xpad", 10, NULL);

	/* search entry */
	search = hildon_entry_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_box_pack_end (GTK_BOX (vbox), search, FALSE, TRUE, 0);
	g_signal_connect (search, "changed",
			  G_CALLBACK (onSearchEntryChanged), this);

	/* window signals */
	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (gtk_main_quit), NULL);

	/* show all widget instead of alignment */
	gtk_widget_show_all (GTK_WIDGET (window));
	gtk_widget_hide (results_widget);

	/* grab focus to search entry */
	gtk_widget_grab_focus (GTK_WIDGET (search));
}

void
MStarDict::ResultsListClear()
{
	gtk_list_store_clear (results_list);
}

void
MStarDict::ResultsListInsertLast(const gchar *word)
{
	GtkTreeIter iter;
	gtk_list_store_append (results_list, &iter);
	gtk_list_store_set (results_list, &iter, DEF_COLUMN, word, -1);
}

void
MStarDict::ReScroll()
{
	hildon_pannable_area_scroll_to (HILDON_PANNABLE_AREA (results_view_scroll), -1, 0);
}

int
main (int argc, char **argv)
{
	/* initialize hildon */
	hildon_gtk_init (&argc, &argv);

	/* initialize localization */
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	/* create main window */
	MStarDict mStarDict;
	pMStarDict = &mStarDict;
	mStarDict.CreateMainWindow();

	/* load all dictionaries */
	mStarDict.LoadDictionaries();

	gtk_main ();
	return 0;
}