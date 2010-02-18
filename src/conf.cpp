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

#include <list>
#include <string>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "conf.hpp"

MStarDictConf::MStarDictConf()
{
    /* get the default client */
    gconf_client = gconf_client_get_default();
}

MStarDictConf::~MStarDictConf()
{
}

bool MStarDictConf::GetStringList(const gchar *key,
				  std::list < std::string > &list)
{
    GConfValue *value = NULL;
    GSList *slist = NULL;

    if (!gconf_client)
	return false;

    value = gconf_client_get(gconf_client, key, NULL);
    if (value) {
	slist = gconf_value_get_list(value);

	if (slist) {
	    GSList *entry = slist;

	    list.clear();
	    while (entry) {
		list.push_back(std::string(gconf_value_get_string((GConfValue *)entry->data)));
		entry = entry->next;
	    }
	}
	gconf_value_free(value);
    } else {
	return false;
    }
    return true;
}

bool MStarDictConf::SetStringList(const gchar *key,
				  std::list < std::string > &list)
{
    GSList *slist = NULL;
    gboolean ret = false;

    if (!gconf_client)
	return false;

    for (std::list < std::string >::iterator i = list.begin(); i != list.end(); ++i) {
	slist = g_slist_append(slist, (gpointer) i->c_str());
    }

    if (slist) {
	ret = gconf_client_set_list(gconf_client, key, GCONF_VALUE_STRING, slist, NULL);
	g_slist_free(slist);
    }
    return ret;
}
