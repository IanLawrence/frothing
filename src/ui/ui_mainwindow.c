/**
 * @file ui_mainwindow.c some functions concerning the main window 
 *
 * Copyright (C) 2004-2006 Nathan J. Conrad <t98502@users.sourceforge.net>
 * Copyright (C) 2004-2007 Lars Lindner <lars.lindner@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#ifdef MAEMO_CHANGES
#  include <hildon/hildon-program.h>
#endif

#include <string.h>
#include <libintl.h>

#include "common.h"
#include "conf.h"
#include "debug.h"
#include "export.h"
#include "feedlist.h"
#include "itemlist.h"
#include "itemview.h"
#include "script.h"
#include "update.h"
#include "ui/ui_script.h"
#include "ui/ui_dialog.h"
#include "ui/ui_dnd.h"
#include "ui/ui_enclosure.h"
#include "ui/ui_feedlist.h"
#include "ui/ui_htmlview.h"
#include "ui/ui_itemlist.h"
#include "ui/ui_mainwindow.h"
#include "ui/ui_node.h"
#include "ui/ui_popup.h"
#include "ui/ui_prefs.h"
#include "ui/ui_search.h"
#include "ui/ui_session.h"
#include "ui/ui_shell.h"
#include "ui/ui_tabs.h"
#include "ui/ui_tray.h"
#include "ui/ui_update.h"

static struct mainwindow {
#ifdef MAEMO_CHANGES
	HildonProgram	*program;
	GtkWidget	*container;
	GtkWidget	*window;
#else
	GtkWindow	*window;
#endif
	GtkWidget	*menubar;
	GtkWidget	*toolbar;
	GtkWidget	*itemlistContainer;	/**< scrolled window holding item list tree view */
	GtkWidget	*itemlist;		/**< item list tree view */
	GtkWidget	*statusbar_feedsinfo;
	GtkActionGroup	*generalActions;
	GtkActionGroup	*addActions;		/**< all types of "New" options */
	GtkActionGroup	*feedActions;		/**< update and mark read */
	GtkActionGroup	*readWriteActions;	/**< node remove and properties, node itemset items remove */
	GtkActionGroup	*itemActions;		/**< item state toggline, single item remove */
	
	LifereaHtmlView	*htmlview;		/**< HTML rendering widget */
	gfloat		zoom;			/**< HTML rendering widget zoom level */
} *mainwindow_priv;

/* all used icons */
GdkPixbuf *icons[MAX_ICONS];

/* icon from theme names */
static const gchar *iconThemeNames[] = {
	NULL,			/* ICON_READ */
	NULL,			/* ICON_UNREAD */
	"emblem-important",	/* ICON_FLAG */
	NULL,			/* ICON_AVAILABLE */
	NULL,			/* ICON_AVAILABLE_OFFLINE */
	NULL,			/* ICON_UNAVAILABLE */
	NULL,			/* ICON_DEFAULT */
	NULL,			/* ICON_OCS */
	"folder",		/* ICON_FOLDER */
	"folder-saved-search",	/* ICON_VFOLDER */
	NULL,			/* ICON_NEWSBIN */
	NULL,			/* ICON_EMPTY */
	NULL,			/* ICON_EMPTY_OFFLINE */
	NULL,			/* ICON_ONLINE */
	NULL,			/* ICON_OFFLINE */
	NULL,			/* ICON_UPDATED */
	"mail-attachment",	/* ICON_ENCLOSURE */
	NULL
};

/* icon names */
static const gchar *iconNames[] = {
	"read.xpm",		/* ICON_READ */
	"unread.png",		/* ICON_UNREAD */
	"flag.png",		/* ICON_FLAG */
	"available.png",	/* ICON_AVAILABLE */
	"available_offline.png",	/* ICON_AVAILABLE_OFFLINE */
	NULL,			/* ICON_UNAVAILABLE */
	"default.png",		/* ICON_DEFAULT */
	"ocs.png",		/* ICON_OCS */
	"directory.png",	/* ICON_FOLDER */
	"vfolder.png",		/* ICON_VFOLDER */
	"newsbin.png",		/* ICON_NEWSBIN */
	"empty.png",		/* ICON_EMPTY */
	"empty_offline.png",	/* ICON_EMPTY_OFFLINE */
	"online.png",		/* ICON_ONLINE */
	"offline.png",		/* ICON_OFFLINE */
	"edit.png",		/* ICON_UPDATED */
	"attachment.png",	/* ICON_ENCLOSURE */
	NULL
};

GtkWidget 	*mainwindow;

/* some prototypes */
static void ui_mainwindow_restore_position(GtkWidget *window);
static gboolean on_close(GtkWidget *widget, GdkEvent *event, struct mainwindow *user_data);
static void ui_mainwindow_create_menus(struct mainwindow *mw);
static gboolean on_mainwindow_window_state_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);

LifereaHtmlView *
ui_mainwindow_get_active_htmlview (void)
{
	return mainwindow_priv->htmlview;
}

extern htmlviewPluginPtr htmlviewPlugin;

/* simple dialogs */

void ui_show_error_box(const char *format, ...) {
	GtkWidget	*dialog;
	va_list		args;
	gchar		*msg;

	g_return_if_fail(format != NULL);

	va_start(args, format);
	msg = g_strdup_vprintf(format, args);
	va_end(args);
	
	dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow),
                  GTK_DIALOG_DESTROY_WITH_PARENT,
                  GTK_MESSAGE_ERROR,
                  GTK_BUTTONS_CLOSE,
                  "%s", msg);
	(void)gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);
	g_free(msg);
}

void ui_show_info_box(const char *format, ...) { 
	GtkWidget	*dialog;
	va_list		args;
	gchar		*msg;

	g_return_if_fail(format != NULL);

	va_start(args, format);
	msg = g_strdup_vprintf(format, args);
	va_end(args);
		
	dialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow),
                  GTK_DIALOG_DESTROY_WITH_PARENT,
                  GTK_MESSAGE_INFO,
                  GTK_BUTTONS_CLOSE,
                  "%s", msg);
	(void)gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);
	g_free(msg);
}

/*------------------------------------------------------------------------------*/
/* exit handler	and menu callbacks						*/
/*------------------------------------------------------------------------------*/

void on_popup_quit(gpointer callback_data, guint callback_action, GtkWidget *widget) {

	(void)on_quit(NULL, NULL, NULL);
}

void on_about_activate(GtkMenuItem *menuitem, gpointer user_data) {
	GtkWidget *dialog;
	GtkLabel *versionLabel;
	gchar *text;

	dialog = liferea_dialog_new (NULL, "aboutdialog");
	versionLabel = GTK_LABEL(liferea_dialog_lookup(dialog, "version_label"));
	text = g_strdup_printf("%s %s", PACKAGE, VERSION);;
	gtk_label_set_text(versionLabel,text);
	g_free(text);
	gtk_widget_show(dialog);
}

void
on_homepagebtn_clicked (GtkButton *button, gpointer user_data)
{
	/* launch the homepage when button in about dialog is pressed */
	liferea_htmlview_launch_in_external_browser(_("http://liferea.sf.net"));
}

void on_topics_activate(GtkMenuItem *menuitem, gpointer user_data) {
	gchar *filename = g_strdup_printf("file://" PACKAGE_DATA_DIR "/" PACKAGE "/doc/html/%s", _("topics_en.html"));
	ui_tabs_new(filename, _("Help Topics"), TRUE);
	g_free(filename);
}


void on_quick_reference_activate(GtkMenuItem *menuitem, gpointer user_data) {
	gchar *filename = g_strdup_printf("file://" PACKAGE_DATA_DIR "/" PACKAGE "/doc/html/%s", _("reference_en.html"));
	ui_tabs_new(filename, _("Quick Reference"), TRUE);
	g_free(filename);
}

void on_faq_activate(GtkMenuItem *menuitem, gpointer user_data) {
	gchar *filename = g_strdup_printf("file://" PACKAGE_DATA_DIR "/" PACKAGE "/doc/html/%s", _("faq_en.html"));
	ui_tabs_new(filename, _("FAQ"), TRUE);
	g_free(filename);
}

/*------------------------------------------------------------------------------*/
/* keyboard navigation	 							*/
/*------------------------------------------------------------------------------*/

/* Set cursor to the first item on a treeview. */
static void
on_treeview_set_first (GtkWidget *treeview)
{
	GtkTreePath	*path;

	path = gtk_tree_path_new_first ();
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (treeview), path, NULL, FALSE);
	gtk_tree_path_free(path);
}

/* Move treeview cursor up and down. */
void
on_treeview_move (GtkWidget *treeview, gint step)
{
	gboolean	ret;

	gtk_widget_grab_focus(treeview);
	g_signal_emit_by_name(GTK_TREE_VIEW (treeview), "move-cursor", GTK_MOVEMENT_DISPLAY_LINES, step, &ret);
}

static void
on_treeview_prev (GtkWidget *treeview)
{
	on_treeview_move (treeview, -1);
}

static void
on_treeview_next (GtkWidget *treeview)
{
	on_treeview_move (treeview, 1);
}

gboolean
on_mainwindow_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	gboolean	modifier_matches = FALSE;
	guint		default_modifiers;
	const gchar	*type;
	GtkWidget	*focusw;

	if (event->type == GDK_KEY_PRESS) {
		default_modifiers = gtk_accelerator_get_default_mod_mask ();

		/* handle headline skimming hotkey */
		switch (event->keyval) {
			case GDK_space:
				switch (conf_get_int_value (BROWSE_KEY_SETTING)) {
					default:
					case 0:
						modifier_matches = ((event->state & default_modifiers) == 0);
						/* Hack to make space handled in the module. This is necessary
						   because the GtkMozEmbed code must be able to catch spaces
						   for input fields.
						   
						   By ignoring the space here it will be passed to the GtkMozEmbed
						   widget which in turn will pass it back if it is not eaten by
						   any input field currently focussed. */
						if (!strcmp (htmlviewPlugin->name, "Mozilla") ||
						    !strcmp (htmlviewPlugin->name, "XulRunner"))
							return FALSE;

						/* GtkHTML2 does handle <Space> correctly */
						break;
					case 1:
						modifier_matches = ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK);
						break;
					case 2:
						modifier_matches = ((event->state & GDK_MOD1_MASK) == GDK_MOD1_MASK);
						break;
				}
				
				if (modifier_matches) {
					/* Note that this code is duplicated in mozilla/mozilla.cpp! */
					if (liferea_htmlview_scroll () == FALSE)
						on_next_unread_item_activate (NULL, NULL);
					return TRUE;
				}
				break;
		}

		/* menu hotkeys (duplicated so they work with hidden menu */
		if (GDK_CONTROL_MASK == (event->state & default_modifiers)) {
			switch (event->keyval) {
				case GDK_KP_Add:
					on_popup_zoomin_selected (NULL, 0, NULL);
					return TRUE;
					break;
				case GDK_KP_Subtract:
					on_popup_zoomout_selected (NULL, 0, NULL);
					return TRUE;
					break;
				case GDK_period:
				case GDK_n:
					on_next_unread_item_activate (NULL, NULL);
					return TRUE;
					break;
				case GDK_r:
					on_menu_allread (NULL, NULL);
					return TRUE;
					break;
				case GDK_t:
					on_toggle_item_flag (NULL, NULL);
					return TRUE;
					break;
				case GDK_u:
					on_toggle_unread_status (NULL, NULL);
					return TRUE;
					break;
				case GDK_a:
					on_menu_update_all (NULL, NULL);
					return TRUE;
					break;
				case GDK_f:
					on_searchbtn_clicked (NULL, NULL);
					return TRUE;
					break;
			}
		}

		/* prevent usage of navigation keys in entries */
		focusw = gtk_window_get_focus (GTK_WINDOW (widget));
		if (GTK_IS_ENTRY (focusw))
			return FALSE;

		/* prevent usage of navigation keys in HTML view */
		type = g_type_name (GTK_WIDGET_TYPE (focusw));
		if (type && (!strcmp (type, "MozContainer")))
			return FALSE;

		/* somehow we don't need to check for GtkHTML2... */

		/* check for treeview navigation */
		if (0 == (event->state & default_modifiers)) {
			switch (event->keyval) {
				case GDK_KP_Delete:
				case GDK_Delete:
					on_remove_item_activate (NULL, NULL);
					return TRUE;
					break;
				case GDK_n: 
					on_next_unread_item_activate (NULL, NULL);
					return TRUE;
					break;
				case GDK_f:
					on_treeview_next (mainwindow_priv->itemlist);
					return TRUE;
					break;
				case GDK_b:
					on_treeview_prev (mainwindow_priv->itemlist);
					return TRUE;
					break;
				case GDK_u:
					on_treeview_prev (liferea_shell_lookup ("feedlist"));
					on_treeview_set_first (mainwindow_priv->itemlist);
					return TRUE;
					break;
				case GDK_d:
					on_treeview_next (liferea_shell_lookup ("feedlist"));
					on_treeview_set_first (mainwindow_priv->itemlist);
					return TRUE;
					break;
			}
		}
	}
	
	return FALSE;
}

static void radio_action_set_current_value(GtkRadioAction *action, gint current_value)
{
	GSList* group;
	gint value;

	/*gtk_radio_action_set_current_value(action, current_value);*/

	group = gtk_radio_action_get_group(action);

	for(; group; group = g_slist_next(group)) {
		action = GTK_RADIO_ACTION(group->data);
		g_object_get(G_OBJECT(action), "value", &value, NULL);
		if (value == current_value) {
			gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), TRUE);
			break;
		}
	}
}

static void
ui_mainwindow_htmlview_statusbar_changed (gpointer obj, gchar *url)
{
	ui_mainwindow_set_status_bar (url);
}

void
ui_mainwindow_set_layout (guint newMode)
{
	gchar	*htmlWidgetName, *ilWidgetName;
	GtkRadioAction *action;

	action = GTK_RADIO_ACTION (gtk_action_group_get_action (mainwindow_priv->generalActions, "NormalView"));
	radio_action_set_current_value (action, newMode);
	
	if (!mainwindow_priv->htmlview) {
		GtkWidget *renderWidget;
		mainwindow_priv->htmlview = liferea_htmlview_new (FALSE);		
		g_signal_connect (mainwindow_priv->htmlview, "statusbar-changed", 
		                  G_CALLBACK (ui_mainwindow_htmlview_statusbar_changed), 
		                  mainwindow_priv);
		renderWidget = liferea_htmlview_get_widget (mainwindow_priv->htmlview);
		gtk_container_add (GTK_CONTAINER (liferea_shell_lookup ("normalViewHtml")), renderWidget);
		gtk_widget_show (renderWidget);
	}
	
	liferea_htmlview_clear (mainwindow_priv->htmlview);

	debug1 (DEBUG_GUI, "Setting item list visibility mode: %d", newMode);
	
	switch (newMode) {
		case NODE_VIEW_MODE_NORMAL:
			htmlWidgetName = "normalViewHtml";
			ilWidgetName = "normalViewItems";
			break;
		case NODE_VIEW_MODE_WIDE:
			htmlWidgetName = "wideViewHtml";
			ilWidgetName = "wideViewItems";
			break;
		case NODE_VIEW_MODE_COMBINED:
			htmlWidgetName = "combinedViewHtml";
			ilWidgetName = "normalViewItems";
			break;
		default:
			g_warning("fatal: illegal viewing mode!");
			return;
			break;
	}

	/* reparenting HTML view */
	gtk_notebook_set_current_page (GTK_NOTEBOOK (liferea_shell_lookup ("itemtabs")), newMode);
	gtk_widget_reparent (liferea_htmlview_get_widget (mainwindow_priv->htmlview), liferea_shell_lookup (htmlWidgetName));
	gtk_widget_reparent (GTK_WIDGET (mainwindow_priv->itemlistContainer), liferea_shell_lookup (ilWidgetName));
 
	/* grab necessary to force HTML widget update (display must
	   change from feed description to list of items and vica 
	   versa */
	gtk_widget_grab_focus (liferea_shell_lookup ("feedlist"));
}

void
ui_mainwindow_set_toolbar_style (const gchar *toolbar_style)
{	
	if (toolbar_style == NULL) /* default to icons */
		gtk_toolbar_set_style (GTK_TOOLBAR(mainwindow_priv->toolbar), GTK_TOOLBAR_ICONS);
	else if (!strcmp (toolbar_style, "text"))
		gtk_toolbar_set_style (GTK_TOOLBAR(mainwindow_priv->toolbar), GTK_TOOLBAR_TEXT);
	else if (!strcmp (toolbar_style, "both"))
		gtk_toolbar_set_style (GTK_TOOLBAR(mainwindow_priv->toolbar), GTK_TOOLBAR_BOTH);
	else if (!strcmp (toolbar_style, "both_horiz") || !strcmp (toolbar_style, "both-horiz") )
		gtk_toolbar_set_style (GTK_TOOLBAR (mainwindow_priv->toolbar), GTK_TOOLBAR_BOTH_HORIZ);
	else /* default to icons */
		gtk_toolbar_set_style (GTK_TOOLBAR (mainwindow_priv->toolbar), GTK_TOOLBAR_ICONS);
}

static gboolean
on_key_press_event_null_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	return TRUE;
}

static gboolean
on_notebook_scroll_event_null_cb (GtkWidget *widget, GdkEventScroll *event)
{
	GtkNotebook *notebook = GTK_NOTEBOOK (widget);

	GtkWidget* child;
	GtkWidget* originator;

	if (!notebook->cur_page)
		return FALSE;

	child = gtk_notebook_get_nth_page (notebook, gtk_notebook_get_current_page (notebook));
	originator = gtk_get_event_widget ((GdkEvent *)event);

	/* ignore scroll events from the content of the page */
	if (!originator || gtk_widget_is_ancestor (originator, child))
		return FALSE;

	return TRUE;
}

static struct mainwindow *ui_mainwindow_new(void) {
	GtkWidget		*window;
	GtkWidget		*statusbar;
	gchar			*toolbar_style;
	struct mainwindow	*mw;

	window = liferea_shell_lookup ("mainwindow");
	mw = mainwindow_priv = g_new0 (struct mainwindow, 1);

#ifdef MAEMO_CHANGES
	mw->program = HILDON_PROGRAM(hildon_program_get_instance());
	mw->container = window;
	mw->window = hildon_window_new();
	gtk_container_add(GTK_CONTAINER(mw->window), GTK_WIDGET(mw->container));
	hildon_program_add_window(mw->program, HILDON_WINDOW(mw->window));
#else
	mw->window = GTK_WINDOW (window);
#endif
	
	toolbar_style = conf_get_toolbar_style ();

	gtk_widget_set_name (window, "lifereaMainwindow");
	gtk_widget_set_name (liferea_shell_lookup ("feedlist"), "feedlist");
	
	ui_mainwindow_create_menus(mw);

#ifdef MAEMO_CHANGES
	gtk_box_pack_end (GTK_BOX (liferea_shell_lookup ("vbox1")), mw->toolbar, FALSE, FALSE, 0);
#else
        gtk_box_pack_start (GTK_BOX (liferea_shell_lookup ("vbox1")), mw->toolbar, FALSE, FALSE, 0);
#endif
	gtk_box_reorder_child(GTK_BOX (liferea_shell_lookup ("vbox1")), mw->toolbar, 0);
	gtk_box_pack_start (GTK_BOX (liferea_shell_lookup ("vbox1")), mw->menubar, FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX (liferea_shell_lookup ("vbox1")), mw->menubar, 0);
	ui_mainwindow_set_toolbar_style(toolbar_style);
	g_free(toolbar_style);
	gtk_widget_show_all(GTK_WIDGET(mw->toolbar));

	g_signal_connect ((gpointer) liferea_shell_lookup ("itemtabs"), "key_press_event",
	                  G_CALLBACK (on_key_press_event_null_cb), NULL);

	g_signal_connect ((gpointer) liferea_shell_lookup ("itemtabs"), "key_release_event",
	                  G_CALLBACK (on_key_press_event_null_cb), NULL);
	
	g_signal_connect ((gpointer) liferea_shell_lookup ("itemtabs"), "scroll_event",
	                  G_CALLBACK (on_notebook_scroll_event_null_cb), NULL);
	
	g_signal_connect(mw->window, "delete_event", G_CALLBACK(on_close), mw);
	g_signal_connect(mw->window, "window_state_event", G_CALLBACK(on_mainwindow_window_state_event), mw);
	g_signal_connect(mw->window, "key_press_event", G_CALLBACK(on_mainwindow_key_press_event), mw);
	
	statusbar = liferea_shell_lookup ("statusbar");
	mw->statusbar_feedsinfo = gtk_label_new("");
	gtk_widget_show(mw->statusbar_feedsinfo);
	gtk_box_pack_start(GTK_BOX(statusbar), mw->statusbar_feedsinfo, FALSE, FALSE, 5);

	ui_mainwindow_restore_position(window);
	
	return mw;
}

static GdkPixbuf* ui_mainwindow_get_theme_icon(GtkIconTheme *icon_theme, const gchar *name, gint size) {
	GError *error = NULL;
	GdkPixbuf *pixbuf;

	pixbuf = gtk_icon_theme_load_icon(icon_theme,
	                                  name, /* icon name */
	                                  size, /* size */
	                                  0,  /* flags */
	                                  &error);
	if(!pixbuf) {
		debug1(DEBUG_GUI, "Couldn't load icon: %s", error->message);
		g_error_free(error);
	}
	return pixbuf;
}

static void
ui_mainwindow_destroy_cb (gpointer user_data)
{
	liferea_htmlview_plugin_deregister ();
	ui_tray_enable (FALSE);
	notification_enable (FALSE);
}

void
ui_mainwindow_init (int mainwindowState)
{
	GtkWidget	*widget;
	int		i;
	GString		*buffer;
	struct mainwindow *mw;
	GtkIconTheme	*icon_theme;
	
	debug_enter("ui_mainwindow_init");
	
	liferea_shell_create ();
	
	ui_search_init();

	mw = ui_mainwindow_new();
	mainwindow = GTK_WIDGET(mw->window);
	ui_tabs_init();
	
	g_signal_connect (G_OBJECT (mainwindow), "destroy", G_CALLBACK (ui_mainwindow_destroy_cb), NULL);
 
	/* load pane proportions */
	if(0 != getNumericConfValue(LAST_VPANE_POS))
		gtk_paned_set_position(GTK_PANED(liferea_shell_lookup("leftpane")), getNumericConfValue(LAST_VPANE_POS));
	if(0 != getNumericConfValue(LAST_HPANE_POS))
		gtk_paned_set_position(GTK_PANED(liferea_shell_lookup("normalViewPane")), getNumericConfValue(LAST_HPANE_POS));
	if(0 != getNumericConfValue(LAST_WPANE_POS))
		gtk_paned_set_position(GTK_PANED(liferea_shell_lookup("wideViewPane")), getNumericConfValue(LAST_WPANE_POS));

	/* order important !!! */
	ui_feedlist_init(liferea_shell_lookup("feedlist"));
	
	mw->itemlistContainer = ui_itemlist_new();
	mw->itemlist = gtk_bin_get_child (GTK_BIN (mw->itemlistContainer));
	/* initially we pack the item list in the normal view pane,
	   which is later changed in ui_mainwindow_set_layout() */
	gtk_container_add(GTK_CONTAINER(liferea_shell_lookup("normalViewItems")), mw->itemlistContainer);
	
	itemview_init();
	
	/* necessary to prevent selection signals when filling the feed list
	   and setting the 2/3 pane mode view */
	gtk_widget_set_sensitive(GTK_WIDGET(liferea_shell_lookup("feedlist")), FALSE);

	/* first try to load icons from theme */
	icon_theme = gtk_icon_theme_get_default();
	for(i = 0; i < MAX_ICONS; i++)
		if(iconThemeNames[i] != NULL)
			icons[i] = ui_mainwindow_get_theme_icon(icon_theme, iconThemeNames[i], 16);

	/* and then load own default icons */
	for(i = 0; i < MAX_ICONS; i++)
		if(NULL == icons[i])
			icons[i] = create_pixbuf(iconNames[i]);

	/* set up icons that are build from stock */
	widget = gtk_button_new();
	icons[ICON_UNAVAILABLE] = gtk_widget_render_icon(widget, GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU, "");
	gtk_widget_destroy(widget);

	ui_mainwindow_update_toolbar();
	ui_mainwindow_update_menubar();
	ui_mainwindow_online_status_changed(update_is_online());
	
	ui_tray_enable(getBooleanConfValue(SHOW_TRAY_ICON));			/* init tray icon */
	ui_dnd_setup_URL_receiver(mainwindow);	/* setup URL dropping support */
	ui_enclosure_init();

	feedlist_init();
	
	ui_popup_update_menues();		/* create popup menues */
			
	if(mainwindowState == MAINWINDOW_ICONIFIED || 
	   (mainwindowState == MAINWINDOW_HIDDEN && ui_tray_get_count() == 0)) {
		gtk_window_iconify(GTK_WINDOW(mainwindow));
		gtk_widget_show(mainwindow);
	} else if(mainwindowState == MAINWINDOW_SHOWN) {
		gtk_widget_show(mainwindow);
	} else {
		/* Needed so that the window structure can be
		   accessed... otherwise will GTK warning when window is
		   shown by clicking on notification icon. */
		// gtk_widget_realize(GTK_WIDGET(mainwindow)); 
		// Does not work with gtkmozembed...
		
		gtk_widget_show(mainwindow);
		gtk_widget_hide(mainwindow);
	}

	/* force two pane mode */
	/*   For some reason, this causes the first item to be selected and then
	     unselected... strange. */
	ui_feedlist_select(NULL);
	/* Initialize the UI with respect to the viewing mode */
	ui_mainwindow_set_layout(2);	/* FIXME: set user defined default viewing mode */
	
	/* set zooming properties */	
	mainwindow_priv->zoom = getNumericConfValue(LAST_ZOOMLEVEL);

	if(0 == mainwindow_priv->zoom) {	/* workaround for scheme problem with the last releases */
		mainwindow_priv->zoom = 100;
		setNumericConfValue(LAST_ZOOMLEVEL, 100);
	}
	liferea_htmlview_set_zoom (mainwindow_priv->htmlview, mainwindow_priv->zoom/100.);
	
	/* create welcome text */
	buffer = g_string_new(NULL);
	htmlview_start_output(buffer, NULL, TRUE, FALSE);
	g_string_append(buffer,    "<div style=\"padding:8px\">"
				   "<table class=\"headmeta\" style=\"border:solid 1px #aaa;font-size:120%\" border=\"0\" cellspacing=\"0\" cellpadding=\"5px\"><tr><td>"
				   // add application icon
				   "<img src=\""
				   PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "pixmaps" G_DIR_SEPARATOR_S
				   "liferea.png\" />"
				   "</td><td><h3>");
	g_string_append(buffer,    _("Liferea - Linux Feed Reader"));
	g_string_append(buffer,    "</h3></td></tr><tr><td colspan=\"2\">");
	g_string_append(buffer,    _("<p>Welcome to <b>Liferea</b>, a desktop news aggregator for online news "
				   "feeds.</p>"
				   "<p>The left pane contains the list of your subscriptions. To add a "
				   "subscription select Feeds -&gt; New Subscription. To browse the headlines "
				   "of a feed select it in the feed list and the headlines will be loaded "
				   "into the right pane.</p>"));
	g_string_append(buffer,    "</td></tr></table>");

	g_string_append(buffer,    "</div>");
				   
	htmlview_finish_output(buffer);
	liferea_htmlview_write (ui_mainwindow_get_active_htmlview(), buffer->str, NULL);
	g_string_free(buffer, TRUE);

	gtk_widget_set_sensitive(GTK_WIDGET(liferea_shell_lookup("feedlist")), TRUE);
	
	script_run_for_hook(SCRIPT_HOOK_STARTUP);

	debug_exit("ui_mainwindow_init");
}

void ui_mainwindow_update_toolbar(void) {
	
	/* to avoid "locking out" the user */
	if(getBooleanConfValue(DISABLE_MENUBAR) && getBooleanConfValue(DISABLE_TOOLBAR))
		setBooleanConfValue(DISABLE_TOOLBAR, FALSE);
	
	if(getBooleanConfValue(DISABLE_TOOLBAR))
		gtk_widget_hide(mainwindow_priv->toolbar);
	else
		gtk_widget_show(mainwindow_priv->toolbar);
}

void
ui_mainwindow_update_feed_menu (gboolean feedActions, gboolean readWrite)
{	
	gtk_action_group_set_sensitive (mainwindow_priv->addActions, readWrite);
	gtk_action_group_set_sensitive (mainwindow_priv->feedActions, feedActions);
	gtk_action_group_set_sensitive (mainwindow_priv->readWriteActions, readWrite);
}

void
ui_mainwindow_update_item_menu (gboolean itemActions)
{	
	gtk_action_group_set_sensitive (mainwindow_priv->itemActions, itemActions);
}

void ui_mainwindow_update_menubar(void) {

	if(getBooleanConfValue(DISABLE_MENUBAR))
		gtk_widget_hide(mainwindow_priv->menubar);
	else
		gtk_widget_show(mainwindow_priv->menubar);
}

void ui_mainwindow_online_status_changed (int online) {
	GtkWidget	*widget;

	widget = liferea_shell_lookup("onlineimage");
	
	if(online) {
		ui_mainwindow_set_status_bar(_("Liferea is now online"));
		gtk_image_set_from_pixbuf(GTK_IMAGE(widget), icons[ICON_ONLINE]);
		atk_object_set_name(gtk_widget_get_accessible(widget), _("Work Offline"));		
	} else {
		ui_mainwindow_set_status_bar(_("Liferea is now offline"));
		gtk_image_set_from_pixbuf(GTK_IMAGE(widget), icons[ICON_OFFLINE]);
		atk_object_set_name(gtk_widget_get_accessible(widget), _("Work Online"));	
	}
	gtk_toggle_action_set_active(
	    GTK_TOGGLE_ACTION(gtk_action_group_get_action(mainwindow_priv->generalActions,"ToggleOfflineMode")),
	    !online);
}

void on_onlinebtn_clicked(GtkButton *button, gpointer user_data) {
	
	update_set_online(!update_is_online());
}

static void on_work_offline_activate(GtkToggleAction *menuitem, gpointer user_data) {
	
	update_set_online(!gtk_toggle_action_get_active(menuitem));
}

gboolean
ui_mainwindow_set_status_bar_idle_cb (gpointer user_data)
{
	gchar	*text = (gchar *)user_data;
	
	gtk_label_set_text (GTK_LABEL (GTK_STATUSBAR (liferea_shell_lookup ("statusbar"))->label), text);
	g_free(text);
	
	return FALSE;
}

void
ui_mainwindow_set_status_bar (const char *format, ...)
{
	va_list		args;
	gchar 		*text;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	text = g_strdup_vprintf (format, args);
	va_end (args);

	g_idle_add ((GSourceFunc)ui_mainwindow_set_status_bar_idle_cb, (gpointer)text);
}

void
ui_mainwindow_save_position (void)
{
	GtkWidget	*pane;
	gint		x, y, w, h;

	/* save pane proportions */
	if (NULL != (pane = liferea_shell_lookup ("leftpane"))) {
		x = gtk_paned_get_position (GTK_PANED (pane));
		setNumericConfValue (LAST_VPANE_POS, x);
	}
	
	if (NULL != (pane = liferea_shell_lookup ("normalViewPane"))) {
		y = gtk_paned_get_position (GTK_PANED (pane));
		setNumericConfValue (LAST_HPANE_POS, y);
	}
	
	if (NULL != (pane = liferea_shell_lookup ("wideViewPane"))) {
		y = gtk_paned_get_position (GTK_PANED (pane));
		setNumericConfValue (LAST_WPANE_POS, y);
	}
	
	/* save itemlist properties */
	setNumericConfValue (LAST_ZOOMLEVEL, (gint)(100.* liferea_htmlview_get_zoom (ui_mainwindow_get_active_htmlview ())));

	/* The following needs to be skipped when the window is not visible */
	if (!GTK_WIDGET_VISIBLE (mainwindow))
		return;
	
	if (getBooleanConfValue (LAST_WINDOW_MAXIMIZED))
		return;

	gtk_window_get_position (GTK_WINDOW (mainwindow), &x, &y);
	gtk_window_get_size (GTK_WINDOW (mainwindow), &w, &h);

	if (x+w<0 || y+h<0 ||
	    x > gdk_screen_width () ||
	    y > gdk_screen_height ())
		return;
	
	/* save window position */
	setNumericConfValue (LAST_WINDOW_X, x);
	setNumericConfValue (LAST_WINDOW_Y, y);	

	/* save window size */
	setNumericConfValue (LAST_WINDOW_WIDTH, w);
	setNumericConfValue (LAST_WINDOW_HEIGHT, h);
}

void ui_mainwindow_tray_add() {

}

void ui_mainwindow_tray_remove() {
	
	if (ui_tray_get_count() == 0)
		if (!GTK_WIDGET_VISIBLE(mainwindow)) {
			ui_mainwindow_restore_position(mainwindow);
			gtk_window_present(GTK_WINDOW(mainwindow));
		}
}

/**
 * Restore the window position from the values saved into gconf. Note
 * that this does not display/present/show the mainwindow.
 */
static void ui_mainwindow_restore_position(GtkWidget *window) {
	/* load window position */
	int x, y, w, h;
	
	x = getNumericConfValue(LAST_WINDOW_X);
	y = getNumericConfValue(LAST_WINDOW_Y);
	
	w = getNumericConfValue(LAST_WINDOW_WIDTH);
	h = getNumericConfValue(LAST_WINDOW_HEIGHT);
	
	/* Restore position only if the width and height were saved */
	if(w != 0 && h != 0) {
	
		if(x >= gdk_screen_width())
			x = gdk_screen_width() - 100;
		else if(x + w < 0)
			x  = 100;

		if(y >= gdk_screen_height())
			y = gdk_screen_height() - 100;
		else if(y + w < 0)
			y  = 100;
	
		gtk_window_move(GTK_WINDOW(window), x, y);

		/* load window size */
		gtk_window_resize(GTK_WINDOW(window), w, h);
	}

	if(getBooleanConfValue(LAST_WINDOW_MAXIMIZED))
		gtk_window_maximize(GTK_WINDOW(window));
	else
		gtk_window_unmaximize(GTK_WINDOW(window));

}

/**
 * Function to present the main window
 */
void ui_mainwindow_show() {

	if((gdk_window_get_state(GTK_WIDGET(mainwindow)->window) & GDK_WINDOW_STATE_ICONIFIED) || !GTK_WIDGET_VISIBLE(mainwindow)) {
		ui_mainwindow_restore_position(mainwindow);
	}
	gtk_window_present(GTK_WINDOW(mainwindow));
}

/*
 * Main menu and tray icon callbacks
 */

static gboolean on_close(GtkWidget *widget, GdkEvent *event, struct mainwindow *mw) {
	
	if((ui_tray_get_count() == 0) || (getBooleanConfValue(DONT_MINIMIZE_TO_TRAY)))
		return on_quit(widget, event, mw);
	ui_mainwindow_save_position();
	gtk_widget_hide(GTK_WIDGET(mw->window));
	return TRUE;
}

void ui_mainwindow_toggle_visibility(GtkMenuItem *menuitem, gpointer data) {
	
	if((gdk_window_get_state(GTK_WIDGET(mainwindow)->window) & GDK_WINDOW_STATE_ICONIFIED) || !GTK_WIDGET_VISIBLE(mainwindow)) {
		ui_mainwindow_restore_position(mainwindow);
		gtk_window_present(GTK_WINDOW(mainwindow));
	} else {
		ui_mainwindow_save_position();
		gtk_widget_hide(mainwindow);
	}
}

gboolean on_mainwindow_window_state_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	if((event->type) == (GDK_WINDOW_STATE)) {
		GdkWindowState changed = ((GdkEventWindowState*)event)->changed_mask, state = ((GdkEventWindowState*)event)->new_window_state;
		
		if (changed == GDK_WINDOW_STATE_MAXIMIZED && !(state & GDK_WINDOW_STATE_WITHDRAWN)) {
			if(state & GDK_WINDOW_STATE_MAXIMIZED)
				setBooleanConfValue(LAST_WINDOW_MAXIMIZED, TRUE);
			else
				setBooleanConfValue(LAST_WINDOW_MAXIMIZED, FALSE);
		}
		if(state & GDK_WINDOW_STATE_ICONIFIED)
			session_set_cmd(NULL, MAINWINDOW_ICONIFIED);
		else if(state & GDK_WINDOW_STATE_WITHDRAWN)
			session_set_cmd(NULL, MAINWINDOW_HIDDEN);
		else
			session_set_cmd(NULL, MAINWINDOW_SHOWN);
	}
	return FALSE;
}

struct file_chooser_tuple {
	GtkWidget *dialog;
	fileChoosenCallback func;
	gpointer user_data;
};

static void ui_choose_file_save_cb(GtkDialog *dialog, gint response_id, gpointer user_data) {
	struct file_chooser_tuple *tuple = (struct file_chooser_tuple*)user_data;
	gchar *filename;

	if(response_id == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		tuple->func(filename, tuple->user_data);
		g_free(filename);
	} else {
		tuple->func(NULL, tuple->user_data);
	}
	
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_free(tuple);
}

static void ui_choose_file_or_dir(gchar *title, gchar *buttonName, gboolean saving, gboolean directory, fileChoosenCallback callback, const gchar *currentPath, const gchar *defaultFilename, gpointer user_data) {
	GtkWidget			*dialog;
	struct file_chooser_tuple	*tuple;
	GtkWidget			*button;

	g_assert(!(saving & directory));
	g_assert(!(defaultFilename && !saving));
	
	dialog = gtk_file_chooser_dialog_new(title, GTK_WINDOW (mainwindow),
	                                     (directory?GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
					      (saving ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN)),
	                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                     NULL);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	tuple = g_new0(struct file_chooser_tuple, 1);
	tuple->dialog = dialog;
	tuple->func = callback;
	tuple->user_data = user_data;
	
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), buttonName, GTK_RESPONSE_ACCEPT);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(button);
	
	g_signal_connect(G_OBJECT(dialog), "response",
	                 G_CALLBACK(ui_choose_file_save_cb), tuple);
	if(currentPath != NULL && g_file_test(currentPath, G_FILE_TEST_EXISTS)) {
		if (directory)
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), currentPath);
		else 
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), currentPath);
	}
	if(defaultFilename)
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), defaultFilename);
	
	gtk_widget_show_all(dialog);
}

void ui_choose_file(gchar *title, gchar *buttonName, gboolean saving, fileChoosenCallback callback, const gchar *currentPath, const gchar *defaultFilename, gpointer user_data) {

	ui_choose_file_or_dir(title, buttonName, saving, FALSE, callback, currentPath, defaultFilename, user_data);
}

void ui_choose_directory(gchar *title, gchar *buttonName, fileChoosenCallback callback, const gchar *currentPath, gpointer user_data) {

	ui_choose_file_or_dir(title, buttonName, FALSE, TRUE, callback, currentPath, NULL, user_data);
}

static const GtkActionEntry ui_mainwindow_action_entries[] = {
	{"SubscriptionsMenu", NULL, N_("_Subscriptions")},
	{"UpdateAll", "gtk-refresh", N_("Update _All"), "<control>A", N_("Updates all subscriptions. This does not update OCS directories."),
	 G_CALLBACK(on_menu_update_all)},
	{"MarkAllFeedsAsRead", "gtk-apply", N_("Mark All As _Read"), NULL, N_("Marks read every item of every subscription."),
	 G_CALLBACK(on_menu_allfeedsread)},
	{"ImportFeedList", "gtk-open", N_("_Import Feed List..."), NULL, N_("Imports an OPML feed list."), G_CALLBACK(on_import_activate)},
	{"ExportFeedList", "gtk-save-as", N_("_Export Feed List..."), NULL, N_("Exports the feed list as OPML."), G_CALLBACK(on_export_activate)},
	{"Quit",GTK_STOCK_QUIT, N_("_Quit"), "<control>Q", NULL, G_CALLBACK(on_quit)},

	{"FeedMenu", NULL, N_("_Feed")},
	{"RemoveAllItems", "gtk-delete", N_("Remove _All Items"), NULL, N_("Removes all items of the currently selected feed."),
	 G_CALLBACK(on_remove_items_activate)},

	{"ItemMenu", NULL, N_("_Item")},
	{"NextUnreadItem", GTK_STOCK_GO_FORWARD, N_("_Next Unread Item"), "<control>N", N_("Jumps to the next unread item. If necessary selects the next feed with unread items."),
	 G_CALLBACK(on_next_unread_item_activate)},

	{"ViewMenu", NULL, N_("_View")},
	{"ZoomIn", "gtk-zoom-in", N_("_Increase Text Size"), "<control>plus", N_("Increases the text size of the item view."),
	 G_CALLBACK(on_popup_zoomin_selected)},
	{"ZoomOut", "gtk-zoom-out", N_("_Decrease Text Size"), "<control>minus", N_("Decreases the text size of the item view."),
	 G_CALLBACK(on_popup_zoomout_selected)},

	{"ToolsMenu", NULL, N_("_Tools")},
	{"ShowUpdateMonitor", NULL, N_("_Update Monitor"), NULL, N_("Show a list of all feeds currently in the update queue"),
	 G_CALLBACK(on_menu_show_update_monitor)},
	{"ShowScriptManager", NULL, N_("_Script Manager"), NULL, N_("Allows to configure and edit LUA hook scripts"),
	 G_CALLBACK(on_menu_show_script_manager)},
	{"ShowPreferences", GTK_STOCK_PREFERENCES, N_("_Preferences"), NULL, N_("Edit Preferences."),
	 G_CALLBACK(on_prefbtn_clicked)},
	 
	{"SearchMenu", NULL, N_("_Search")},
	{"SearchFeeds", "gtk-find", N_("Search All Feeds..."), "<control>F", N_("Show the search dialog."), G_CALLBACK(on_searchbtn_clicked)},
	{"CreateEngineSearch", NULL, N_("Search With ...")},
	
	{"HelpMenu", NULL, N_("_Help")},
	{"ShowHelpContents", "gtk-help", N_("_Contents"), "F1", N_("View help for this application."), G_CALLBACK(on_topics_activate)},
	{"ShowHelpQuickReference", NULL, N_("_Quick Reference"), NULL, N_("View a list of all Liferea shortcuts."),
	 G_CALLBACK(on_quick_reference_activate)},
	{"ShowHelpFAQ", NULL, N_("_FAQ"), NULL, N_("View the FAQ for this application."), G_CALLBACK(on_faq_activate)},
	{"ShowAbout", "gtk-about", N_("_About"), NULL, N_("Shows an about dialog."), G_CALLBACK(on_about_activate)}
};

static const GtkRadioActionEntry ui_mainwindow_view_radio_entries[] = {
	{"NormalView", NULL, N_("_Normal View"), NULL, N_("Set view mode to mail client mode."),
	 0},
	{"WideView", NULL, N_("_Wide View"), NULL, N_("Set view mode to use three vertical panes."),
	 1},
	{"CombinedView", NULL, N_("_Combined View"), NULL, N_("Set view mode to two pane mode."),
	 2}
};

static const GtkActionEntry ui_mainwindow_add_action_entries[] = {
	{"NewSubscription", "gtk-add", N_("_New Subscription..."), NULL, N_("Adds a subscription to the feed list."),
	 G_CALLBACK(on_menu_feed_new)},
	{"NewFolder", NULL, N_("New _Folder..."), NULL, N_("Adds a folder to the feed list."), G_CALLBACK(on_menu_folder_new)},
	{"NewVFolder", NULL, N_("New S_earch Folder..."), NULL, N_("Adds a new search folder to the feed list."), G_CALLBACK(on_new_vfolder_activate)},
	{"NewPlugin", NULL, N_("New _Source..."), NULL, N_("Adds a new feed list source."), G_CALLBACK(on_new_plugin_activate)},
	{"NewNewsBin", NULL, N_("New _News Bin..."), NULL, N_("Adds a new news bin."), G_CALLBACK(on_new_newsbin_activate)}
};

static const GtkActionEntry ui_mainwindow_feed_action_entries[] = {
	{"MarkFeedAsRead", "gtk-apply", N_("_Mark As Read"), "<control>R", N_("Marks all items of the selected subscription or of all subscriptions of the selected folder as read."), 
	 G_CALLBACK(on_menu_allread)},
	{"UpdateSelected", "gtk-refresh", N_("_Update"), NULL, N_("Updates the selected subscription or all subscriptions of the selected folder."),
	 G_CALLBACK(on_menu_update)}
};

static const GtkActionEntry ui_mainwindow_read_write_action_entries[] = {
	{"Properties", "gtk-properties", N_("_Properties..."), NULL, N_("Opens the property dialog for the selected subscription."), G_CALLBACK(on_menu_properties)},
	{"DeleteSelected", "gtk-delete", N_("_Remove"), NULL, N_("Removes the selected subscription."), G_CALLBACK(on_menu_delete)}
};

static const GtkActionEntry ui_mainwindow_item_action_entries[] = {
	{"ToggleItemReadStatus", "gtk-apply", N_("Toggle _Read Status"), "<control>U", N_("Toggles the read status of the selected item."),
	 G_CALLBACK(on_toggle_unread_status)},
	{"ToggleItemFlag", NULL, N_("Toggle Item _Flag"), "<control>T", N_("Toggles the flag status of the selected item."),
	 G_CALLBACK(on_toggle_item_flag)},
	{"RemoveSelectedItem", "gtk-delete", N_("R_emove"), NULL, N_("Removes the selected item."),
	 G_CALLBACK(on_remove_item_activate)},
	{"LaunchItemInBrowser", NULL, N_("_Launch In Browser"), NULL, N_("Launches the item's link in the configured browser."),
	 G_CALLBACK(on_popup_launchitem_selected)}
};

static const GtkToggleActionEntry ui_mainwindow_action_toggle_entries[] = {
	{"ToggleOfflineMode", NULL, N_("_Work Offline"), NULL, N_("This option allows you to disable subscription updating."),
	 G_CALLBACK(on_work_offline_activate)}
};

static const char *ui_mainwindow_ui_desc =
"<ui>"
"  <menubar name='MainwindowMenubar'>"
"    <menu action='SubscriptionsMenu'>"
"      <menuitem action='UpdateAll'/>"
"      <menuitem action='MarkAllFeedsAsRead'/>"
"      <separator/>"
"      <menuitem action='NewSubscription'/>"
"      <menuitem action='NewFolder'/>"
"      <menuitem action='NewVFolder'/>"
"      <menuitem action='NewPlugin'/>"
"      <menuitem action='NewNewsBin'/>"
"      <separator/>"
"      <menuitem action='ImportFeedList'/>"
"      <menuitem action='ExportFeedList'/>"
"      <separator/>"
"      <menuitem action='ToggleOfflineMode'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='FeedMenu'>"
"      <menuitem action='UpdateSelected'/>"
"      <menuitem action='MarkFeedAsRead'/>"
"      <separator/>"
"      <menuitem action='RemoveAllItems'/>"
"      <menuitem action='DeleteSelected'/>"
"      <separator/>"
"      <menuitem action='Properties'/>"
"    </menu>"
"    <menu action='ItemMenu'>"
"      <menuitem action='NextUnreadItem'/>"
"      <separator/>"
"      <menuitem action='ToggleItemReadStatus'/>"
"      <menuitem action='ToggleItemFlag'/>"
"      <menuitem action='RemoveSelectedItem'/>"
"      <separator/>"
"      <menuitem action='LaunchItemInBrowser'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='ZoomIn'/>"
"      <menuitem action='ZoomOut'/>"
"      <separator/>"
"      <menuitem action='NormalView'/>"
"      <menuitem action='WideView'/>"
"      <menuitem action='CombinedView'/>"
"    </menu>"
"    <menu action='ToolsMenu'>"
"      <menuitem action='ShowUpdateMonitor'/>"
"      <menuitem action='ShowScriptManager'/>"
"      <menuitem action='ShowPreferences'/>"
"    </menu>"
"    <menu action='SearchMenu'>"
"      <menuitem action='SearchFeeds'/>"
"      <menu action='CreateEngineSearch'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='ShowHelpContents'/>"
"      <menuitem action='ShowHelpQuickReference'/>"
"      <menuitem action='ShowHelpFAQ'/>"
"      <separator/>"
"      <menuitem action='ShowAbout'/>"
"    </menu>"
"  </menubar>"
"  <toolbar action='maintoolbar'>"
"    <separator/>"
"    <toolitem name='newFeedButton' action='NewSubscription'/>"
"    <toolitem name='nextUnreadButton' action='NextUnreadItem'/>"
"    <toolitem name='MarkAsReadButton' action='MarkFeedAsRead'/>"
"    <toolitem name='UpdateAllButton' action='UpdateAll'/>"
"    <toolitem name='SearchButton' action='SearchFeeds'/>"
//"    <toolitem name='ViewingModeButton' action='ToggleCondensedMode'/>"
"    <toolitem name='PreferencesButton' action='ShowPreferences'/>"
"    <separator/>"
"  </toolbar>"
"</ui>";

static void ui_mainwindow_create_menus(struct mainwindow *mw) {
	GtkUIManager	*ui_manager;
	GtkAccelGroup	*accel_group;
	GError		*error = NULL;
	
	//register_my_stock_icons ();
	//gtk_container_add (GTK_CONTAINER (window), vbox);
	ui_manager = gtk_ui_manager_new ();

	mw->generalActions = gtk_action_group_new ("GeneralActions");
	gtk_action_group_set_translation_domain (mw->generalActions, PACKAGE);
	gtk_action_group_add_actions (mw->generalActions, ui_mainwindow_action_entries, G_N_ELEMENTS (ui_mainwindow_action_entries), mw);
	gtk_action_group_add_toggle_actions (mw->generalActions, ui_mainwindow_action_toggle_entries, G_N_ELEMENTS (ui_mainwindow_action_toggle_entries), mw);
	gtk_action_group_add_radio_actions (mw->generalActions, ui_mainwindow_view_radio_entries, G_N_ELEMENTS (ui_mainwindow_view_radio_entries), itemlist_get_view_mode (), (GCallback)on_view_activate, (gpointer)TRUE);
	gtk_ui_manager_insert_action_group (ui_manager, mw->generalActions, 0);

	mw->addActions = gtk_action_group_new ("AddActions");
	gtk_action_group_set_translation_domain (mw->addActions, PACKAGE);
	gtk_action_group_add_actions (mw->addActions, ui_mainwindow_add_action_entries, G_N_ELEMENTS (ui_mainwindow_add_action_entries), mw);
	gtk_ui_manager_insert_action_group (ui_manager, mw->addActions, 0);
	
	mw->feedActions = gtk_action_group_new ("FeedActions");
	gtk_action_group_set_translation_domain (mw->feedActions, PACKAGE);
	gtk_action_group_add_actions (mw->feedActions, ui_mainwindow_feed_action_entries, G_N_ELEMENTS (ui_mainwindow_feed_action_entries), mw);
	gtk_ui_manager_insert_action_group (ui_manager, mw->feedActions, 0);
	
	mw->readWriteActions = gtk_action_group_new("ReadWriteActions");
	gtk_action_group_set_translation_domain (mw->readWriteActions, PACKAGE);
	gtk_action_group_add_actions (mw->readWriteActions, ui_mainwindow_read_write_action_entries, G_N_ELEMENTS (ui_mainwindow_read_write_action_entries), mw);
	gtk_ui_manager_insert_action_group (ui_manager, mw->readWriteActions, 0);
	
	mw->itemActions = gtk_action_group_new ("ItemActions");
	gtk_action_group_set_translation_domain (mw->itemActions, PACKAGE);
	gtk_action_group_add_actions (mw->itemActions, ui_mainwindow_item_action_entries, G_N_ELEMENTS (ui_mainwindow_item_action_entries), mw);
	gtk_ui_manager_insert_action_group (ui_manager, mw->itemActions, 0);

#ifndef MAEMO_CHANGES
	accel_group = gtk_ui_manager_get_accel_group (ui_manager);
	gtk_window_add_accel_group (mw->window, accel_group);
	g_object_unref (accel_group);
#endif

	if(!gtk_ui_manager_add_ui_from_string(ui_manager, ui_mainwindow_ui_desc, -1, &error))
		g_error("building menus failed: %s", error->message);
	
	ui_search_engines_setup_menu(ui_manager);

	mw->menubar = gtk_ui_manager_get_widget (ui_manager, "/MainwindowMenubar");
	mw->toolbar = gtk_ui_manager_get_widget (ui_manager, "/maintoolbar");


#ifdef MAEMO_CHANGES
	GtkWidget *main_menu;
	GList *iter;

	/* Create new main menu */
	main_menu = gtk_menu_new();

	iter = gtk_container_get_children (GTK_CONTAINER (mw->menubar));
	while (iter) {
		GtkWidget *menu;

		menu = GTK_WIDGET (iter->data);
		gtk_widget_reparent(menu, main_menu);

		iter = g_list_next (iter);
	}

	hildon_window_set_menu(HILDON_WINDOW(mw->window), GTK_MENU(main_menu));
#endif
	
	/* what a pain, why is there no markup for this option? */
	g_object_set (G_OBJECT (gtk_ui_manager_get_widget (ui_manager, "/maintoolbar/newFeedButton")), "is_important", TRUE, NULL);
	g_object_set (G_OBJECT (gtk_ui_manager_get_widget (ui_manager, "/maintoolbar/nextUnreadButton")), "is_important", TRUE, NULL);
	g_object_set (G_OBJECT (gtk_ui_manager_get_widget (ui_manager, "/maintoolbar/MarkAsReadButton")), "is_important", TRUE, NULL);
	g_object_set (G_OBJECT (gtk_ui_manager_get_widget (ui_manager, "/maintoolbar/UpdateAllButton")), "is_important", TRUE, NULL);
	g_object_set (G_OBJECT (gtk_ui_manager_get_widget (ui_manager, "/maintoolbar/SearchButton")), "is_important", TRUE, NULL);
	g_object_set (G_OBJECT (gtk_ui_manager_get_widget (ui_manager, "/maintoolbar/PreferencesButton")), "is_important", TRUE, NULL);
}

void ui_mainwindow_update_feedsinfo(void) {
	gint	new_items, unread_items;
	gchar	*msg, *tmp;

	if(mainwindow == NULL)
		return;
	
	new_items = feedlist_get_new_item_count();
	unread_items = feedlist_get_unread_item_count();
	
	if(new_items != 0) {
		msg = g_strdup_printf(ngettext(" (%d new)", " (%d new)", new_items), new_items);
	} else {
		msg = g_strdup("");
	}
		
	if(unread_items != 0) {
		tmp = g_strdup_printf(ngettext("%d unread%s", "%d unread%s", unread_items), unread_items, msg);
	} else {
		tmp = g_strdup("");
	}

	gtk_label_set_text(GTK_LABEL(mainwindow_priv->statusbar_feedsinfo), tmp);
	g_free(tmp);
	g_free(msg);
}
