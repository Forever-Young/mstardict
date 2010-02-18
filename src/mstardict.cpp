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

enum {
	INDEX_COLUMN = 0,
	BOOKNAME_COLUMN,
	DEF_COLUMN,
	N_COLUMNS
};


MStarDict::MStarDict ()
{
	label_widget = NULL;
	results_widget = NULL;
	results_view = NULL;

	/* create list of ressults */
	results_list = gtk_list_store_new (N_COLUMNS,
					   G_TYPE_INT,		/* INDEX_COLUMN */
					   G_TYPE_STRING,	/* BOOKNAME_COLUMN */
					   G_TYPE_STRING);	/* DEF_COLUMN */

	/* set sorting of resuslts */
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (results_list),
					      DEF_COLUMN,
					      GTK_SORT_ASCENDING);

	/* initialize stardict library */
	lib = new Library (true, true);
}

MStarDict::~MStarDict ()
{
	/* destroy list of results */
	g_object_unref (results_list);

	/* deinitialize stardict library */
	delete lib;
}

gboolean
MStarDict::on_results_view_selection_changed (GtkTreeSelection *selection,
					      MStarDict *mStarDict)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *bookname, *def, *exp;
	gint selected = 0;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		/* unselect selected rows */
		gtk_tree_selection_unselect_all (selection);
		gtk_tree_model_get (model, &iter, INDEX_COLUMN, &selected, -1);

		bookname = g_markup_printf_escaped ("<span color=\"dimgray\" size=\"x-small\">%s</span>",
						    mStarDict->result_list[selected].bookname.c_str());
		def = g_markup_printf_escaped ("<span color=\"darkred\" weight=\"heavy\" size=\"large\">%s</span>",
					       mStarDict->result_list[selected].def.c_str());
		exp = g_strdup (mStarDict->result_list[selected].exp.c_str());

		/* create translation window */
		mStarDict->create_translation_window (bookname, def, exp);

		g_free (bookname);
		g_free (def);
		g_free (exp);
	}

	/* grab focus to search entry */
	gtk_widget_grab_focus (GTK_WIDGET (mStarDict->search));

	return TRUE;
}

gboolean
MStarDict::on_search_entry_changed (GtkEditable *editable,
				    MStarDict *mStarDict)
{
	GtkTreeSelection *selection;
	const gchar *search;
	GtkTreeIter iter;

	search = gtk_entry_get_text (GTK_ENTRY (editable));

	if (strcmp (search, "") == 0) {
		gtk_widget_show (mStarDict->label_widget);
		gtk_widget_hide (mStarDict->results_widget);
	} else {
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mStarDict->results_view));
		gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

		/* clear previous search results */
		mStarDict->result_list.clear();
		gtk_list_store_clear (mStarDict->results_list);

		/* unselect rows */
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (mStarDict->results_view));
		gtk_tree_selection_unselect_all (selection);

		/* fill list with new results */
		mStarDict->lib->process_phrase(search, mStarDict->result_list);
		if (!mStarDict->result_list.empty()) {
			for (size_t i = 0; i < mStarDict->result_list.size(); ++i) {
				gtk_list_store_append (mStarDict->results_list, &iter);
				gtk_list_store_set (mStarDict->results_list,
						    &iter,
						    INDEX_COLUMN, i,
						    BOOKNAME_COLUMN, mStarDict->result_list[i].bookname.c_str(),
						    DEF_COLUMN, mStarDict->result_list[i].def.c_str(),
						    -1);
			}
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
MStarDict::load_dictionaries ()
{
	strlist_t dicts_dir_list;
	strlist_t order_list;
	strlist_t disable_list;

	std::list<std::string> load_list;

	/* dictionary directory */
	dicts_dir_list.push_back (std::string ("/home/user/MyDocs/mstardict"));

	for_each_file(dicts_dir_list, ".ifo", order_list, disable_list, GetAllDictList(load_list));
	lib->load(load_list);

	lib->query_dictmask.clear();
	for (std::list<std::string>::iterator i = load_list.begin(); i != load_list.end(); ++i) {
		size_t iLib;
		if (lib->find_lib_by_filename(i->c_str(), iLib)) {
			InstantDictIndex instance_dict_index;
			instance_dict_index.type = InstantDictType_LOCAL;
			instance_dict_index.index = iLib;
			lib->query_dictmask.push_back(instance_dict_index);
		}
	}
}

void
MStarDict::create_translation_window (const gchar *bookname,
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
MStarDict::create_main_window ()
{
	HildonProgram *program = NULL;
	GtkWidget *window, *alignment, *vbox, *pannable;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GdkColor style_color;

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
	pannable = hildon_pannable_area_new ();
	gtk_container_add (GTK_CONTAINER (results_widget), pannable);

	/* result tree view */
	results_view = hildon_gtk_tree_view_new (HILDON_UI_MODE_EDIT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (results_view),
				 GTK_TREE_MODEL (results_list));
	gtk_container_add (GTK_CONTAINER (pannable), results_view);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (results_view));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (on_results_view_selection_changed), this);

	/* def column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (results_view),
						     -1,
						     "Def", renderer,
						     "text", DEF_COLUMN,
						     NULL);
	g_object_set (G_OBJECT (renderer), "xpad", 10, NULL);

	/* bookname column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (results_view),
						     -1,
						     "Bookname", renderer,
						     "text", BOOKNAME_COLUMN,
						     NULL);

	if (!gtk_style_lookup_color (GTK_WIDGET (label_widget)->style, "SecondaryTextColor",
				     &style_color)) {
		gdk_color_parse ("grey", &style_color);
	}
	g_object_set (G_OBJECT (renderer),
		      "xalign", 1.0,
		      "width-chars", 10,
		      "foreground-gdk", &style_color,
		      "foreground-set", TRUE,
		      "size", 12000,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "ellipsize-set", TRUE,
		      NULL);

	/* search entry */
	search = hildon_entry_new (HILDON_SIZE_FINGER_HEIGHT);
	gtk_box_pack_end (GTK_BOX (vbox), search, FALSE, TRUE, 0);
	g_signal_connect (search, "changed",
			  G_CALLBACK (on_search_entry_changed), this);

	/* window signals */
	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (gtk_main_quit), NULL);

	/* show all widget instead of alignment */
	gtk_widget_show_all (GTK_WIDGET (window));
	gtk_widget_hide (results_widget);

	/* grab focus to search entry */
	gtk_widget_grab_focus (GTK_WIDGET (search));
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
	mStarDict.create_main_window ();

	/* load all dictionaries */
	mStarDict.load_dictionaries ();

	gtk_main ();
	return 0;
}
