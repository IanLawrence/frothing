/**
 * @file xulrunner.c a xulrunner browser module implementation.
 *   
 * Copyright (C) 2003-2006 Lars Lindner <lars.lindner@gmx.net>   
 * Copyright (C) 2004-2006 Nathan J. Conrad <t98502@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtkmozembed.h>
#include "ui/ui_htmlview.h"
#include "mozembed.h"
#include "plugin.h"

static void xulrunner_init(void) {
	
	/* gtk_moz_embed_set_comp_path() must not be called
	   without a component path for XulRunner. Passing
	   NULL will crash Liferea. */
	gtk_moz_embed_set_comp_path(XULRUNNER_HOME);

	mozembed_init();	
}

static struct htmlviewPlugin xulrunnerInfo = {
	.api_version	= HTMLVIEW_PLUGIN_API_VERSION,
	.name		= "XulRunner",
	.priority	= 11,
	.externalCss	= TRUE,
	.plugin_init	= xulrunner_init,
	.plugin_deinit	= mozembed_deinit,
	.create		= mozembed_create,
	.write		= mozembed_write,
	.launch		= mozembed_launch_url,
	.launchInsidePossible = mozembed_launch_inside_possible,
	.zoomLevelGet	= mozsupport_get_zoom,
	.zoomLevelSet	= mozsupport_set_zoom,
	.scrollPagedown	= mozsupport_scroll_pagedown,
	.setProxy	= mozembed_set_proxy,
	.setOffLine	= mozsupport_set_offline_mode
};

static struct plugin pi = {
	PLUGIN_API_VERSION,
	"XulRunner Rendering Plugin",
	PLUGIN_TYPE_HTML_RENDERER,
	&xulrunnerInfo
};

DECLARE_PLUGIN(pi);
DECLARE_HTMLVIEW_PLUGIN(xulrunnerInfo);
