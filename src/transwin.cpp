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

#include <string>
#include <vector>
#include <memory>
#include <list>

#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <hildon/hildon.h>

#include "libwrapper.hpp"
#include "mstardict.hpp"
#include "transwin.hpp"

TransWin::TransWin(MStarDict *mStarDict)
{
    oStarDict = mStarDict;
    window = NULL;
}

TransWin::~TransWin()
{
}

GtkWidget *
TransWin::CreateTransWidget(SearchResult *result)
{
    GtkWidget *vbox, *hbox, *label;
    char *bookname = NULL, *def = NULL, *exp = NULL;

    bookname = g_markup_printf_escaped("<span color=\"dimgray\" size=\"x-small\">%s</span>",
				       result->bookname);
    def = g_markup_printf_escaped("<span color=\"darkred\" weight=\"heavy\" size=\"large\">%s</span>",
				  result->def);
    exp = g_strdup(result->exp);

    vbox = gtk_vbox_new(FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Definition");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    if (def) {
	gtk_label_set_markup(GTK_LABEL(label), def);
	g_free(def);
    }

    label = gtk_label_new("Bookname");
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    if (bookname) {
	gtk_label_set_markup(GTK_LABEL(label), bookname);
	g_free(bookname);
    }

    label = gtk_label_new("Expresion");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    if (exp) {
	gtk_label_set_markup(GTK_LABEL(label), exp);
	g_free(exp);
    }

    return vbox;
}

void
TransWin::CreateTransWindow(GList *results)
{
    GtkWidget *window, *alignment, *pannable, *vbox, *trans, *separator;
    GList *result = NULL;

    window = hildon_stackable_window_new();
    gtk_window_set_title(GTK_WINDOW(window), _("Translation"));

    alignment = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment),
			      HILDON_MARGIN_DEFAULT,
			      HILDON_MARGIN_DEFAULT, HILDON_MARGIN_DOUBLE, HILDON_MARGIN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(window), alignment);

    pannable = hildon_pannable_area_new();
    gtk_container_add(GTK_CONTAINER(alignment), pannable);

    vbox = gtk_vbox_new(FALSE, 16);
    hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pannable), vbox);

    for (result = results; result != NULL; result = result->next) {
	trans = CreateTransWidget((SearchResult *) result->data);
	gtk_box_pack_start(GTK_BOX(vbox), trans, FALSE, FALSE, 0);

	separator = gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(window);
}
