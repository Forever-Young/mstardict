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

class MStarDict {
private:
	GtkWidget *label_widget;
	GtkWidget *results_widget;
	GtkWidget *search;
	GtkWidget *results_view;

	GtkListStore *results_list;

	Library *lib;
	TSearchResultList result_list;

	static gboolean on_results_view_selection_changed (GtkTreeSelection *selection, MStarDict *mStarDict);
	static gboolean on_search_entry_changed (GtkEditable *editable, MStarDict *mStarDict);

public:
	MStarDict ();
	~MStarDict ();
	void create_translation_window (const gchar *bookname, const gchar *def, const gchar *exp);
	void create_main_window ();
	void load_dictionaries ();
};
