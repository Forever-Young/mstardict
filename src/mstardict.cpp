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
#include <list>

#include "conf.hpp"
#include "dictmngr.hpp"
#include "libwrapper.hpp"
#include "transwin.hpp"
#include "mstardict.hpp"

MStarDict *pMStarDict;

enum {
    DEF_COLUMN,
    N_COLUMNS
};

MStarDict::MStarDict()
{
    window = NULL;
    label_widget = NULL;
    results_widget = NULL;
    results_view = NULL;
    results_view_scroll = NULL;

    /* create list of ressults */
    results_list = gtk_list_store_new(N_COLUMNS,
				      G_TYPE_STRING);	/* DEF_COLUMN */

    /* initialize configuration */
    oConf = new Conf();

    /* initialize stardict plugins */
    std::list < std::string > plugin_order_list;
    std::list < std::string > plugin_disable_list;
    oStarDictPlugins = new StarDictPlugins("/usr/lib/mstardict/plugins",
					   plugin_order_list,
					   plugin_disable_list);

    /* initialize dict manager */
    oDict = new DictMngr(this);

    /* initialize translation window */
    oTransWin = new TransWin(this);

    /* initialize stardict library */
    oLibs = new Library(this);
}

MStarDict::~MStarDict()
{
    /* destroy list of results */
    g_object_unref(results_list);

    /* deinitialize stardict library */
    delete oLibs;

    /* deinitialize translation window */
    delete oTransWin;

    /* deinitialize dict manager */
    delete oDict;

    /* deinitialize stardict plugins */
    delete oStarDictPlugins;

    /* deinitialize configuration */
    delete oConf;
}

gboolean
MStarDict::onResultsViewSelectionChanged(GtkTreeSelection *selection,
					 MStarDict *mStarDict)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    const gchar *sWord;
    bool bFound = false;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
	GList *results = NULL;

	/* unselect selected rows */
	gtk_tree_selection_unselect_all(selection);

	gtk_tree_model_get(model, &iter, DEF_COLUMN, &sWord, -1);

	for (size_t iLib = 0; iLib < mStarDict->oLibs->query_dictmask.size(); iLib++) {
	    bFound =
		mStarDict->oLibs->BuildResultData(mStarDict->oLibs->query_dictmask, sWord,
						  mStarDict->oLibs->iCurrentIndex, iLib,
						  &results);
	}

	/* create translation window */
	mStarDict->oTransWin->CreateTransWindow(results);

	/* free result data */
	mStarDict->oLibs->FreeResultData(results);
    }

    /* grab focus to search entry */
    mStarDict->GrabFocus();

    return true;
}

gboolean
MStarDict::onSearchEntryChanged(GtkEditable* editable,
				MStarDict* mStarDict)
{
    const gchar *sWord;
    bool bFound = false;
    std::string query;

    if (mStarDict->oLibs->query_dictmask.empty())
	return true;

    sWord = gtk_entry_get_text(GTK_ENTRY(editable));
    if (strcmp(sWord, "") == 0) {
	mStarDict->ShowNoResults(true);
    } else {
	mStarDict->ShowProgressIndicator(true);
	mStarDict->ResultsUnselectAll(GTK_SELECTION_NONE);

	switch (analyse_query(sWord, query)) {
	case qtFUZZY:
	    bFound = mStarDict->oLibs->LookupWithFuzzy(query.c_str());
	    break;

	case qtPATTERN:
	    bFound = mStarDict->oLibs->LookupWithRule(query.c_str());
	    break;

	case qtREGEX:
	    bFound = mStarDict->oLibs->LookupWithRegex(query.c_str());
	    break;

	case qtSIMPLE:
	    bFound = mStarDict->oLibs->SimpleLookup(query.c_str(), mStarDict->oLibs->iCurrentIndex);
	    if (!bFound) {
		const gchar *sugWord = mStarDict->oLibs->GetSuggestWord(query.c_str(),
									mStarDict->
									oLibs->iCurrentIndex,
									mStarDict->
									oLibs->query_dictmask, 0);
		if (sugWord) {
		    gchar *sSugWord = g_strdup(sugWord);
		    bFound =
			mStarDict->oLibs->SimpleLookup(sSugWord, mStarDict->oLibs->iCurrentIndex);
		    g_free(sSugWord);
		}
	    }
	    mStarDict->oLibs->ListWords(mStarDict->oLibs->iCurrentIndex);
	    break;

	default:
	    break;
	}

	if (bFound)
	    mStarDict->ShowNoResults(false);
	else
	    mStarDict->ShowNoResults(true);

	mStarDict->ResultsUnselectAll(GTK_SELECTION_SINGLE);
	mStarDict->ShowProgressIndicator(false);
    }

    return true;
}

gboolean
MStarDict::onDictionariesMenuItemClicked(GtkButton *button,
					 MStarDict *mStarDict)
{
    mStarDict->oDict->CreateDictMngrDialog();

    /* trigger re-search */
    mStarDict->onSearchEntryChanged(GTK_EDITABLE(mStarDict->search), mStarDict);
    return true;
}

gboolean
MStarDict::onQuitMenuItemClicked(GtkButton *button,
				 MStarDict *mStarDict)
{
    gtk_main_quit();
    return true;
}

gboolean
MStarDict::onLookupProgressDialogResponse(GtkDialog *dialog,
					  gint response_id,
					  bool *cancel)
{
    *cancel = true;
    return true;
}

gboolean
MStarDict::onMainWindowKeyPressEvent(GtkWidget *window,
				     GdkEventKey *event,
				     MStarDict *mStarDict)
{
    if (event->type == GDK_KEY_PRESS && event->keyval == GDK_KP_Enter) {
	mStarDict->SearchWord();
    } else if (event->type == GDK_KEY_PRESS && event->keyval >= 0x21 && event->keyval <= 0x7E) {
	mStarDict->GrabFocus();
    }
    return false;
}

GtkWidget *
MStarDict::CreateLookupProgressDialog(bool *cancel)
{
    GtkWidget *dialog, *progress;

    /* create dialog */
    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), _("Searching"));
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_OK);

    g_signal_connect(dialog, "response", G_CALLBACK(onLookupProgressDialogResponse), cancel);

    /* add progress bar */
    progress = gtk_progress_bar_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), progress);
    g_object_set_data(G_OBJECT(dialog), "progress_bar", progress);

    /* show dialog */
    gtk_widget_show_all(dialog);

    while (gtk_events_pending())
	gtk_main_iteration();

    return dialog;
}

void
MStarDict::DestroyLookupProgressDialog(GtkWidget *dialog)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
MStarDict::CreateMainWindow()
{
    HildonProgram *program = NULL;
    GtkWidget *alignment, *vbox;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;

    /* hildon program */
    program = hildon_program_get_instance();
    g_set_application_name(_("MStardict"));

    /* main window */
    window = hildon_stackable_window_new();
    hildon_program_add_window(program, HILDON_WINDOW(window));

    /* aligment */
    alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment),
			      HILDON_MARGIN_HALF, 0, HILDON_MARGIN_DEFAULT, HILDON_MARGIN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(window), alignment);

    /* main vbox */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), vbox);

    /* no_search_result label */
    label_widget = gtk_label_new(_("No search result"));
    hildon_helper_set_logical_color(label_widget, GTK_RC_FG,
				    GTK_STATE_NORMAL, "SecondaryTextColor");
    hildon_helper_set_logical_font(label_widget, "LargeSystemFont");
    gtk_box_pack_start(GTK_BOX(vbox), label_widget, TRUE, TRUE, 0);

    /* alignment for pannable area */
    results_widget = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(results_widget),
			      0, 0, HILDON_MARGIN_DEFAULT, HILDON_MARGIN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(vbox), results_widget, TRUE, TRUE, 0);

    /* pannable for tree view */
    results_view_scroll = hildon_pannable_area_new();
    gtk_container_add(GTK_CONTAINER(results_widget), results_view_scroll);

    /* result tree view */
    results_view = hildon_gtk_tree_view_new(HILDON_UI_MODE_EDIT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(results_view), GTK_TREE_MODEL(results_list));
    gtk_container_add(GTK_CONTAINER(results_view_scroll), results_view);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(results_view));
    g_signal_connect(selection, "changed", G_CALLBACK(onResultsViewSelectionChanged), this);

    /* def column */
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW
						(results_view), -1, "Def",
						renderer, "text", DEF_COLUMN, NULL);
    g_object_set(G_OBJECT(renderer),
		 "xpad", 10,
		 "ellipsize", PANGO_ELLIPSIZE_END,
		 "ellipsize-set", TRUE,
		 NULL);

    /* search entry */
    search = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_box_pack_end(GTK_BOX(vbox), search, FALSE, TRUE, 0);
    g_signal_connect(search, "changed", G_CALLBACK(onSearchEntryChanged), this);

    /* window signals */
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(onMainWindowKeyPressEvent), this);

    /* show all widget instead of alignment */
    gtk_widget_show_all(GTK_WIDGET(window));

    /* grab focus to search entry */
    GrabFocus();
}

void
MStarDict::CreateMainMenu()
{
    HildonAppMenu *menu;
    GtkWidget *item;

    menu = HILDON_APP_MENU(hildon_app_menu_new());
    hildon_window_set_app_menu(HILDON_WINDOW(window), menu);

    /* dictionaries menu item */
    item = hildon_gtk_button_new(HILDON_SIZE_AUTO);
    gtk_button_set_label(GTK_BUTTON(item), _("Dictionaries"));
    hildon_app_menu_append(menu, GTK_BUTTON(item));
    g_signal_connect(item, "clicked", G_CALLBACK(onDictionariesMenuItemClicked), this);

    /* quit menu item */
    item = hildon_gtk_button_new(HILDON_SIZE_AUTO);
    gtk_button_set_label(GTK_BUTTON(item), _("Quit"));
    hildon_app_menu_append(menu, GTK_BUTTON(item));
    g_signal_connect(item, "clicked", G_CALLBACK(onQuitMenuItemClicked), this);

    /* show main menu */
    gtk_widget_show_all(GTK_WIDGET(menu));
}

void
MStarDict::SearchWord()
{
    const gchar *sWord;
    bool bFound = false;
    std::string query;

    if (oLibs->query_dictmask.empty())
	return;

    sWord = gtk_entry_get_text(GTK_ENTRY(search));
    if (strcmp(sWord, "") == 0) {
	ShowNoResults(true);
    } else {
	/* unselect rows */
	ResultsUnselectAll(GTK_SELECTION_NONE);

	switch (analyse_query(sWord, query)) {
	case qtDATA:
	    bFound = oLibs->LookupData(query.c_str());
	    break;
	default:
	    /* nothing */ ;
	}

	/* unselect selected rows */
	ResultsUnselectAll(GTK_SELECTION_SINGLE);

	if (bFound)
	    ShowNoResults(false);
	else
	    ShowNoResults(true);
    }
}

void
MStarDict::ResultsListClear()
{
    gtk_list_store_clear(results_list);
}

void
MStarDict::ResultsListInsertLast(const gchar *word)
{
    GtkTreeIter iter;
    gtk_list_store_append(results_list, &iter);
    gtk_list_store_set(results_list, &iter, DEF_COLUMN, word, -1);
}

void
MStarDict::ResultsReScroll()
{
    hildon_pannable_area_scroll_to(HILDON_PANNABLE_AREA(results_view_scroll), -1, 0);
}

void
MStarDict::ResultsUnselectAll(GtkSelectionMode mode)
{
    GtkTreeSelection *selection;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(results_view));
    gtk_tree_selection_set_mode(selection, mode);
    gtk_tree_selection_unselect_all(selection);
}

void
MStarDict::ShowNoResults(bool bNoResults)
{
    if (bNoResults) {
	gtk_label_set_text(GTK_LABEL(label_widget), _("No search result"));
	gtk_widget_show(label_widget);
	gtk_widget_hide(results_widget);
    } else {
	gtk_widget_hide(label_widget);
	gtk_widget_show(results_widget);
    }
}

void
MStarDict::ShowNoDictionary(bool bNoDictionary)
{
    if (bNoDictionary) {
	gtk_label_set_text(GTK_LABEL(label_widget), _("No loaded dictionary"));
	gtk_widget_show(label_widget);
	gtk_widget_hide(results_widget);
    } else {
	gtk_widget_hide(label_widget);
	gtk_widget_show(results_widget);
    }
}

void
MStarDict::ShowProgressIndicator(bool bShow)
{
    if (bShow)
	hildon_gtk_window_set_progress_indicator(GTK_WINDOW(window), 1);
    else
	hildon_gtk_window_set_progress_indicator(GTK_WINDOW(window), 0);
}

void
MStarDict::GrabFocus()
{
    gtk_widget_grab_focus(GTK_WIDGET(search));
}

int
main(int argc,
     char **argv)
{
    /* initialize hildon */
    hildon_gtk_init(&argc, &argv);

    /* initialize localization */
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    /* create main window */
    MStarDict mStarDict;
    pMStarDict = &mStarDict;
    mStarDict.CreateMainWindow();
    mStarDict.CreateMainMenu();
    mStarDict.ShowNoResults(true);

    /* load dictionaries */
    mStarDict.oDict->LoadDictionaries();

    gtk_main();
    return 0;
}
