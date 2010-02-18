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
#include "libwrapper.hpp"
#include "file.hpp"
#include "mstardict.hpp"

MStarDict *pMStarDict;

enum {
    DEF_COLUMN,
    N_COLUMNS
};

enum {
    BOOKNAME_DICT_INFO_COLUMN,
    FILENAME_DICT_INFO_COLUMN,
    N_DICT_INFO_COLUMNS
};

class GetAllDictList {
  public:
    GetAllDictList(std::list < std::string > &dict_all_list_):dict_all_list(dict_all_list_) {
    } void operator() (const std::string & url, bool disable) {
	dict_all_list.push_back(url);
    }
  private:
    std::list < std::string > &dict_all_list;
};

MStarDict::MStarDict()
{
    main_window = NULL;
    label_widget = NULL;
    results_widget = NULL;
    results_view = NULL;
    results_view_scroll = NULL;

    /* create list of ressults */
    results_list = gtk_list_store_new(N_COLUMNS,
				      G_TYPE_STRING);	/* DEF_COLUMN */

    /* initialize configuration */
    oConf = new MStarDictConf();

    /* initialize stardict library */
    oLibs = new Library();
}

MStarDict::~MStarDict()
{
    /* destroy list of results */
    g_object_unref(results_list);

    /* deinitialize stardict library */
    delete oLibs;

    /* deinitialize configuration */
    delete oConf;
}

gboolean
MStarDict::onResultsViewSelectionChanged(GtkTreeSelection *selection,
					 MStarDict *mStarDict)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    char *bookname, *def, *exp;
    const gchar *sWord;
    bool bFound = false;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
	/* unselect selected rows */
	gtk_tree_selection_unselect_all(selection);

	gtk_tree_model_get(model, &iter, DEF_COLUMN, &sWord, -1);

	/* clear previous search results */
	mStarDict->results.clear();

	for (size_t iLib = 0; iLib < mStarDict->oLibs->query_dictmask.size(); iLib++) {
	    bFound =
		mStarDict->oLibs->BuildResultData(mStarDict->oLibs->query_dictmask, sWord,
						  mStarDict->oLibs->iCurrentIndex, iLib,
						  mStarDict->results);
	}

	bookname =
	    g_markup_printf_escaped
	    ("<span color=\"dimgray\" size=\"x-small\">%s</span>",
	     mStarDict->results[0].bookname.c_str());
	def =
	    g_markup_printf_escaped
	    ("<span color=\"darkred\" weight=\"heavy\" size=\"large\">%s</span>",
	     mStarDict->results[0].def.c_str());
	exp = g_strdup(mStarDict->results[0].exp.c_str());

	/* create translation window */
	mStarDict->CreateTranslationWindow(bookname, def, exp);

	g_free(bookname);
	g_free(def);
	g_free(exp);
    }

    /* grab focus to search entry */
    gtk_widget_grab_focus(GTK_WIDGET(mStarDict->search));

    return true;
}

gboolean
MStarDict::onSearchEntryChanged(GtkEditable* editable,
				MStarDict* mStarDict)
{
    GtkTreeSelection *selection;
    const gchar *sWord;
    bool bFound = false;
    std::string query;

    sWord = gtk_entry_get_text(GTK_ENTRY(editable));

    if (mStarDict->oLibs->query_dictmask.empty())
	return true;

    if (strcmp(sWord, "") == 0) {
	mStarDict->ShowNoResults(true);
    } else {
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(mStarDict->results_view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

	/* unselect rows */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(mStarDict->results_view));
	gtk_tree_selection_unselect_all(selection);

	/* show progress indicator */
//	mStarDict->ShowProgressIndicator(true);

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
	case qtDATA:
	    bFound = mStarDict->oLibs->LookupData(query.c_str());
	    break;
	default:
	    /* nothing */ ;
	}

	/* unselect selected rows */
	gtk_tree_selection_unselect_all(selection);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

	/* hide progress indicator */
//	mStarDict->ShowProgressIndicator(false);

	if (bFound)
	    mStarDict->ShowNoResults(false);
	else
	    mStarDict->ShowNoResults(true);
    }

    return true;
}

gboolean
MStarDict::onDictionariesMenuItemClicked(GtkButton *button,
					 MStarDict *mStarDict)
{
    GtkWidget *dialog, *selector;
    GtkCellRenderer *renderer;
    HildonTouchSelectorColumn *column;
    GtkTreeModel *tree_model;
    GtkTreeIter iter;
    gboolean iter_valid = TRUE;
    std::list < std::string > all_dict_list;
    std::list < std::string > selected_dict_list;
    GtkListStore *dict_list = NULL;

    dict_list = gtk_list_store_new(N_DICT_INFO_COLUMNS,
				   G_TYPE_STRING,	/* bookname */
				   G_TYPE_STRING);	/* filename */

    /* create dialog */
    dialog = gtk_dialog_new();
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_title(GTK_WINDOW(dialog), _("Dictionaries"));
    gtk_dialog_add_button(GTK_DIALOG(dialog), "OK", GTK_RESPONSE_ACCEPT);
    gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 400);

    /* dictionary selector */
    selector = hildon_touch_selector_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), selector);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer), "xpad", 10, NULL);
    column =
	hildon_touch_selector_append_column(HILDON_TOUCH_SELECTOR
					    (selector),
					    GTK_TREE_MODEL(dict_list),
					    renderer, "text", BOOKNAME_DICT_INFO_COLUMN, NULL);
    hildon_touch_selector_column_set_text_column(column, 0);

    /* fill list with all available dictionaries */
    mStarDict->GetAllDictionaryList(all_dict_list);
    for (std::list < std::string >::iterator i = all_dict_list.begin();
	 i != all_dict_list.end(); ++i) {
	DictInfo dictinfo;

	dictinfo.load_from_ifo_file(i->c_str(), 0);
	gtk_list_store_append(dict_list, &iter);
	gtk_list_store_set(dict_list, &iter,
			   BOOKNAME_DICT_INFO_COLUMN,
			   dictinfo.bookname.c_str(), FILENAME_DICT_INFO_COLUMN, i->c_str(), -1);
    }
    g_object_unref(dict_list);

    /* set selector mode to multiple */
    hildon_touch_selector_set_column_selection_mode(HILDON_TOUCH_SELECTOR
						    (selector),
						    HILDON_TOUCH_SELECTOR_SELECTION_MODE_MULTIPLE);
    hildon_touch_selector_unselect_all(HILDON_TOUCH_SELECTOR(selector), BOOKNAME_DICT_INFO_COLUMN);

    /* select all load dictionaries */
    tree_model =
	hildon_touch_selector_get_model(HILDON_TOUCH_SELECTOR(selector), BOOKNAME_DICT_INFO_COLUMN);
    for (iter_valid = gtk_tree_model_get_iter_first(tree_model, &iter);
	 iter_valid; iter_valid = gtk_tree_model_iter_next(tree_model, &iter)) {
	const gchar *bookname;

	gtk_tree_model_get(tree_model, &iter, BOOKNAME_DICT_INFO_COLUMN, &bookname, -1);
	for (size_t iLib = 0; iLib < mStarDict->oLibs->query_dictmask.size(); iLib++) {
	    if (!strcmp(mStarDict->oLibs->dict_name(iLib).c_str(), bookname)) {
		hildon_touch_selector_select_iter(HILDON_TOUCH_SELECTOR
						  (selector),
						  BOOKNAME_DICT_INFO_COLUMN, &iter, FALSE);
		break;
	    }
	}
    }

    /* show dialog */
    gtk_widget_show_all(GTK_WIDGET(dialog));

    /* run the dialog */
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	GList *selected_dicts = NULL;

	selected_dicts =
	    hildon_touch_selector_get_selected_rows(HILDON_TOUCH_SELECTOR
						    (selector), BOOKNAME_DICT_INFO_COLUMN);
	if (selected_dicts) {
	    GList *dict = selected_dicts;
	    const gchar *filename;

	    while (dict) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(tree_model), &iter,
					(GtkTreePath *) (dict->data));
		gtk_tree_model_get(GTK_TREE_MODEL(tree_model), &iter,
				   FILENAME_DICT_INFO_COLUMN, &filename, -1);
		selected_dict_list.push_back(std::string(filename));
		dict = dict->next;
	    }
	    g_list_foreach(selected_dicts, (GFunc) gtk_tree_path_free, NULL);
	    g_list_free(selected_dicts);
	}

	if (mStarDict->oConf->SetStringList("/apps/maemo/mstardict/dict_list", selected_dict_list)) {
	    /* reload dictionaries */
	    mStarDict->ReLoadDictionaries(selected_dict_list);

	    /* trigger re-search */
	    mStarDict->onSearchEntryChanged(GTK_EDITABLE(mStarDict->search), mStarDict);
	}
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
    return true;
}

gboolean
MStarDict::onQuitMenuItemClicked(GtkButton *button,
				 MStarDict *mStarDict)
{
    gtk_main_quit();
    return true;
}

void
MStarDict::GetAllDictionaryList(std::list < std::string > &dict_list)
{
    strlist_t dicts_dir_list;
    strlist_t order_list;
    strlist_t disable_list;

    /* dictionary directory */
    dicts_dir_list.push_back(std::string("/home/user/MyDocs/mstardict"));
    for_each_file(dicts_dir_list, ".ifo", order_list, disable_list, GetAllDictList(dict_list));
}

void
MStarDict::LoadDictionaries()
{
    std::list < std::string > dict_list;

    if (!oConf->GetStringList("/apps/maemo/mstardict/dict_list", dict_list)) {
	GetAllDictionaryList(dict_list);
	oConf->SetStringList("/apps/maemo/mstardict/dict_list", dict_list);
    }

    oLibs->load(dict_list);
    oLibs->query_dictmask.clear();
    for (std::list < std::string >::iterator i = dict_list.begin(); i != dict_list.end(); ++i) {
	size_t iLib;
	if (oLibs->find_lib_by_filename(i->c_str(), iLib)) {
	    InstantDictIndex instance_dict_index;
	    instance_dict_index.type = InstantDictType_LOCAL;
	    instance_dict_index.index = iLib;
	    oLibs->query_dictmask.push_back(instance_dict_index);
	}
    }

    if (oLibs->iCurrentIndex)
	g_free(oLibs->iCurrentIndex);
    oLibs->iCurrentIndex =
	(CurrentIndex *) g_malloc(sizeof(CurrentIndex) * oLibs->query_dictmask.size());

    if (oLibs->query_dictmask.empty())
	ShowNoDictionary(true);
}

void
MStarDict::ReLoadDictionaries(std::list < std::string > &dict_list)
{
    oLibs->reload(dict_list, 0, 0);
    oLibs->query_dictmask.clear();
    for (std::list < std::string >::iterator i = dict_list.begin(); i != dict_list.end(); ++i) {
	size_t iLib;
	if (oLibs->find_lib_by_filename(i->c_str(), iLib)) {
	    InstantDictIndex instance_dict_index;
	    instance_dict_index.type = InstantDictType_LOCAL;
	    instance_dict_index.index = iLib;
	    oLibs->query_dictmask.push_back(instance_dict_index);
	}
    }

    if (oLibs->iCurrentIndex)
	g_free(oLibs->iCurrentIndex);
    oLibs->iCurrentIndex =
	(CurrentIndex *) g_malloc(sizeof(CurrentIndex) * oLibs->query_dictmask.size());

    if (oLibs->query_dictmask.empty())
	ShowNoDictionary(true);
}

void
MStarDict::CreateTranslationWindow(const gchar *bookname,
				   const gchar *def,
				   const gchar *exp)
{
    GtkWidget *window, *alignment, *pannable, *vbox, *label;

    window = hildon_stackable_window_new();
    gtk_window_set_title(GTK_WINDOW(window), _("Translation"));

    alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment),
			      HILDON_MARGIN_DEFAULT,
			      HILDON_MARGIN_DEFAULT, HILDON_MARGIN_DOUBLE, HILDON_MARGIN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(window), alignment);

    pannable = hildon_pannable_area_new();
    g_object_set(G_OBJECT(pannable), "mov-mode", HILDON_MOVEMENT_MODE_BOTH, NULL);
    gtk_container_add(GTK_CONTAINER(alignment), pannable);

    vbox = gtk_vbox_new(FALSE, 0);
    hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pannable), vbox);

    label = gtk_label_new("Bookname");
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_label_set_markup(GTK_LABEL(label), bookname);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    label = gtk_label_new("Definition");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_label_set_markup(GTK_LABEL(label), def);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    label = gtk_label_new("Expresion");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_label_set_markup(GTK_LABEL(label), exp);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
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
    main_window = hildon_stackable_window_new();
    hildon_program_add_window(program, HILDON_WINDOW(main_window));

    /* aligment */
    alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment),
			      HILDON_MARGIN_HALF, 0, HILDON_MARGIN_DEFAULT, HILDON_MARGIN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(main_window), alignment);

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
    g_object_set(G_OBJECT(renderer), "xpad", 10, NULL);

    /* search entry */
    search = hildon_entry_new(HILDON_SIZE_FINGER_HEIGHT);
    gtk_box_pack_end(GTK_BOX(vbox), search, FALSE, TRUE, 0);
    g_signal_connect(search, "changed", G_CALLBACK(onSearchEntryChanged), this);

    /* window signals */
    g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* show all widget instead of alignment */
    gtk_widget_show_all(GTK_WIDGET(main_window));

    /* grab focus to search entry */
    gtk_widget_grab_focus(GTK_WIDGET(search));
}

void
MStarDict::CreateMainMenu()
{
    HildonAppMenu *menu;
    GtkWidget *item;

    menu = HILDON_APP_MENU(hildon_app_menu_new());
    hildon_window_set_app_menu(HILDON_WINDOW(main_window), menu);

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
MStarDict::ReScroll()
{
    hildon_pannable_area_scroll_to(HILDON_PANNABLE_AREA(results_view_scroll), -1, 0);
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
	hildon_gtk_window_set_progress_indicator(GTK_WINDOW(main_window), 1);
    else
	hildon_gtk_window_set_progress_indicator(GTK_WINDOW(main_window), 0);
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

    /* load all dictionaries */
    mStarDict.LoadDictionaries();

    gtk_main();
    return 0;
}
