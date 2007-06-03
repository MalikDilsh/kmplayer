/*
* Copyright (C) 2007  Koos Vriezen <koos.vriezen@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/* gcc -o knpplayer `pkg-config --libs --cflags gtk+-x11-2.0` `pkg-config --libs --cflags dbus-glib-1` `nspr-config --libs --cflags` npplayer.c

http://devedge-temp.mozilla.org/library/manuals/2002/plugin/1.0/
http://dbus.freedesktop.org/doc/dbus/libdbus-tutorial.html
*/

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>

#include <glib/gprintf.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#define XP_UNIX
#define MOZ_X11
#include "moz-sdk/npupp.h"

typedef const char* (* NP_LOADDS NP_GetMIMEDescriptionUPP)();
typedef NPError (* NP_InitializeUPP)(NPNetscapeFuncs*, NPPluginFuncs*);
typedef NPError (* NP_ShutdownUPP)(void);

static gchar *plugin;
static gchar *url;
static gchar *mimetype;

static DBusConnection *dbus_connection;
static char *service_name;
static gchar *callback_service;
static gchar *callback_path;
static GModule *library;
static GtkWidget *xembed;
static Window socket_id;
static int stdin_read_watch;

static NPPluginFuncs np_funcs;       /* plugin functions              */
static NPP npp;                      /* single instance of the plugin */
static NPSavedData *saved_data;
static GSList *stream_list;
static char stream_buf[4096];
typedef struct _StreamInfo {
    NPStream np_stream;
    void *notify_data;
    unsigned int stream_buf_pos;
    unsigned int stream_pos;
    unsigned int total;
    unsigned int reason;
    char *url;
    char *mimetype;
    char *target;
} StreamInfo;

static NP_GetMIMEDescriptionUPP npGetMIMEDescription;
static NP_InitializeUPP npInitialize;
static NP_ShutdownUPP npShutdown;

static StreamInfo *addStream (const char *url, const char *mime, const char *target, void *notify, int req);
static gboolean requestStream (void * si);
static gboolean destroyStream (void * si);

/*----------------%<---------------------------------------------------------*/

static NPError nsGetURL (NPP instance, const char* url, const char* target) {
    g_printf ("nsGetURL %s %s\n", url, target);
    addStream (url, 0L, target, 0L, 1);
    return NPERR_NO_ERROR;
}

static NPError nsPostURL (NPP instance, const char *url,
        const char *target, uint32 len, const char *buf, NPBool file) {
    g_printf ("nsPostURL %s %s\n", url, target);
    addStream (url, 0L, target, 0L, 1);
    return NPERR_NO_ERROR;
}

static NPError nsRequestRead (NPStream *stream, NPByteRange *rangeList) {
    g_printf ("nsRequestRead\n");
    return NPERR_NO_ERROR;
}

static NPError nsNewStream (NPP instance, NPMIMEType type,
        const char *target, NPStream **stream) {
    g_printf ("nsNewStream\n");
    return NPERR_NO_ERROR;
}

static int32 nsWrite (NPP instance, NPStream* stream, int32 len, void *buf) {
    g_printf ("nsWrite\n");
    return 0;
}

static NPError nsDestroyStream (NPP instance, NPStream *stream, NPError reason) {
    g_printf ("nsDestroyStream\n");
    g_timeout_add (0, destroyStream, stream_list);
    return NPERR_NO_ERROR;
}

static void nsStatus (NPP instance, const char* message) {
    g_printf ("NPN_Status %s\n", message);
}

static const char* nsUserAgent (NPP instance) {
    g_printf ("NPN_UserAgent\n");
    return "";
}

static void *nsAlloc (uint32 size) {
    return malloc (size);
}

static void nsMemFree (void* ptr) {
    free (ptr);
}

static uint32 nsMemFlush (uint32 size) {
    g_printf ("NPN_MemFlush\n");
    return 0;
}

static void nsReloadPlugins (NPBool reloadPages) {
    g_printf ("NPN_ReloadPlugins\n");
}

static NPError nsGetURLNotify (NPP instance, const char* url, const char* target, void *notify) {
    g_printf ("NPN_GetURLNotify %s %s\n", url, target);
    addStream (url, 0L, target, notify, 1);
    return NPERR_NO_ERROR;
}

static NPError nsPostURLNotify (NPP instance, const char* url, const char* target, uint32 len, const char* buf, NPBool file, void *notify) {
    g_printf ("NPN_PostURLNotify\n");
    addStream (url, 0L, target, notify, 1);
    return NPERR_NO_ERROR;
}

static NPError nsGetValue (NPP instance, NPNVariable variable, void *value) {
    g_printf ("NPN_GetValue %d\n", variable & ~NP_ABI_MASK);
    switch (variable) {
        case NPNVxDisplay:
            *(void**)value = (void*)(long)gdk_x11_display_get_xdisplay (gtk_widget_get_display (xembed));
            break;
        case NPNVxtAppContext:
            *(void**)value = NULL;
            break;
        case NPNVnetscapeWindow:
            g_printf ("NPNVnetscapeWindow\n");
            break;
        case NPNVjavascriptEnabledBool:
            *(int*)value = 1;
            break;
        case NPNVasdEnabledBool:
            *(int*)value = 0;
            break;
        case NPNVisOfflineBool:
            *(int*)value = 0;
            break;
        case NPNVserviceManager:
            *(int*)value = 0;
            break;
        case NPNVToolkit:
            *(int*)value = 2; /* ?? */
            break;
        case NPNVSupportsXEmbedBool:
            *(int*)value = 1;
            break;
        default:
            *(int*)value = 0;
            g_printf ("unknown value\n");
    }
    return NPERR_NO_ERROR;
}

static NPError nsSetValue (NPP instance, NPPVariable variable, void *value) {
    /* NPPVpluginWindowBool */
    g_printf ("NPN_SetValue\n");
    return NPERR_NO_ERROR;
}

static void nsInvalidateRect (NPP instance, NPRect *invalidRect) {
    g_printf ("NPN_InvalidateRect\n");
}

static void nsInvalidateRegion (NPP instance, NPRegion invalidRegion) {
    g_printf ("NPN_InvalidateRegion\n");
}

static void nsForceRedraw (NPP instance) {
    g_printf ("NPN_ForceRedraw\n");
}

/*----------------%<---------------------------------------------------------*/

static void shutDownPlugin() {
    if (npShutdown) {
        if (npp) {
            np_funcs.destroy (npp, &saved_data);
            free (npp);
            npp = 0L;
        }
        npShutdown();
        npShutdown = 0;
    }
}

static void removeStream (NPReason reason) {
    StreamInfo *si= (StreamInfo *) g_slist_nth_data (stream_list, 0);

    if (stdin_read_watch)
        gdk_input_remove (stdin_read_watch);
    stdin_read_watch = 0;

    if (si) {
        g_printf ("data received %d\n", si->stream_pos);
        stream_list = g_slist_remove (stream_list, si);
        np_funcs.destroystream (npp, &si->np_stream, reason);
        if (si->notify_data)
            np_funcs.urlnotify (npp, si->url, reason, si->notify_data);
        g_free (si->url);
        if (si->mimetype)
            g_free (si->mimetype);
        if (si->target)
            g_free (si->target);
        free (si);
        si = (StreamInfo*) g_slist_nth_data (stream_list, 0);
        if (si && callback_service)
            g_timeout_add (0, requestStream, si);
    }
}

static StreamInfo *addStream (const char *url, const char *mime, const char *target, void *notify, int req) {
    StreamInfo *si = (StreamInfo*) g_slist_nth_data (stream_list, 0);
    int req1 = !si ? 1 : 0;
    si = (StreamInfo *) malloc (sizeof (StreamInfo));

    g_printf ("new stream\n");
    memset (si, 0, sizeof (StreamInfo));
    si->url = g_strdup (url);
    si->np_stream.url = si->url;
    if (mime)
        si->mimetype = g_strdup (mime);
    if (target)
        si->target = g_strdup (target);
    si->notify_data = notify;
    stream_list = g_slist_append (stream_list, si);

    if (req1 && callback_service)
        g_timeout_add (0, requestStream, si);

    return si;
}

static void readStdin (gpointer d, gint src, GdkInputCondition cond) {
    StreamInfo *si = (StreamInfo*) g_slist_nth_data (stream_list, 0);
    gsize count = read (src,
            stream_buf + si->stream_buf_pos,
            sizeof (stream_buf) - si->stream_buf_pos);
    g_assert (si);
    if (count > 0) {
        int32 sz;
        si->stream_buf_pos += count;
        sz = np_funcs.writeready (npp, &si->np_stream);
        if (sz > 0) {
            sz = np_funcs.write (npp, &si->np_stream, si->stream_pos,
                    si->stream_buf_pos > sz ? sz : si->stream_buf_pos,
                    stream_buf);
            if (sz == si->stream_buf_pos)
                si->stream_buf_pos = 0;
            else if (sz > 0) {
                si->stream_buf_pos -= sz;
                memmove (stream_buf, stream_buf + sz, si->stream_buf_pos);
            }
            si->stream_pos += sz;
        }
        if (si->stream_pos == si->total)
            removeStream (si->reason);
    } else
        removeStream (NPRES_DONE); /* only for 'cat foo | knpplayer' */
}

static int initPlugin (const char *plugin_lib) {
    NPNetscapeFuncs ns_funcs;
    NPError np_err;

    g_printf ("starting %s with %s\n", plugin_lib, url);
    library = g_module_open (plugin_lib, G_MODULE_BIND_LAZY);
    if (!library) {
        g_printf ("failed to load %s\n", plugin_lib);
        return -1;
    }
    if (!g_module_symbol (library,
                "NP_GetMIMEDescription", (gpointer *)&npGetMIMEDescription)) {
        g_printf ("undefined reference to load NP_GetMIMEDescription\n");
        return -1;
    }
    if (!g_module_symbol (library,
                "NP_Initialize", (gpointer *)&npInitialize)) {
        g_printf ("undefined reference to load NP_Initialize\n");
        return -1;
    }
    if (!g_module_symbol (library,
                "NP_Shutdown", (gpointer *)&npShutdown)) {
        g_printf ("undefined reference to load NP_Shutdown\n");
        return -1;
    }
    g_printf ("startup succeeded %s\n", npGetMIMEDescription ());
    memset (&ns_funcs, 0, sizeof (NPNetscapeFuncs));
    ns_funcs.size = sizeof (NPNetscapeFuncs);
    ns_funcs.version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
    ns_funcs.geturl = nsGetURL;
    ns_funcs.posturl = nsPostURL;
    ns_funcs.requestread = nsRequestRead;
    ns_funcs.newstream = nsNewStream;
    ns_funcs.write = nsWrite;
    ns_funcs.destroystream = nsDestroyStream;
    ns_funcs.status = nsStatus;
    ns_funcs.uagent = nsUserAgent;
    ns_funcs.memalloc = nsAlloc;
    ns_funcs.memfree = nsMemFree;
    ns_funcs.memflush = nsMemFlush;
    ns_funcs.reloadplugins = nsReloadPlugins;
    /*ns_funcs.getJavaEnv;
    //ns_funcs.getJavaPeer;*/
    ns_funcs.geturlnotify = nsGetURLNotify;
    ns_funcs.posturlnotify = nsPostURLNotify;
    ns_funcs.getvalue = nsGetValue;
    ns_funcs.setvalue = nsSetValue;
    ns_funcs.invalidaterect = nsInvalidateRect;
    ns_funcs.invalidateregion = nsInvalidateRegion;
    ns_funcs.forceredraw = nsForceRedraw;
    /*ns_funcs.getstringidentifier;
    ns_funcs.getstringidentifiers;
    ns_funcs.getintidentifier;
    ns_funcs.identifierisstring;
    ns_funcs.utf8fromidentifier;
    ns_funcs.intfromidentifier;
    ns_funcs.createobject;
    ns_funcs.retainobject;
    ns_funcs.releaseobject;
    ns_funcs.invoke;
    ns_funcs.invokeDefault;
    ns_funcs.evaluate;
    ns_funcs.getproperty;
    ns_funcs.setproperty;
    ns_funcs.removeproperty;
    ns_funcs.hasproperty;
    ns_funcs.hasmethod;
    ns_funcs.releasevariantvalue;
    ns_funcs.setexception;
    ns_funcs.pushpopupsenabledstate;
    ns_funcs.poppopupsenabledstate;*/

    np_funcs.size = sizeof (NPPluginFuncs);

    np_err = npInitialize (&ns_funcs, &np_funcs);
    if (np_err != NPERR_NO_ERROR) {
        g_printf ("NP_Initialize failure %d\n", np_err);
        npShutdown = 0;
        return -1;
    }
    return 0;
}

static int newPlugin (NPMIMEType mime, int16 argc, char *argn[], char *argv[]) {
    NPWindow window;
    NPSetWindowCallbackStruct ws_info;
    NPError np_err;

    npp = (NPP_t*)malloc (sizeof (NPP_t));
    /*np_err = np_funcs.getvalue ((void*)npp, NPPVpluginNeedsXEmbed, (void*)&np_value);
    if (np_err != NPERR_NO_ERROR || !np_value) {
        g_printf ("NPP_GetValue NPPVpluginNeedsXEmbed failure %d\n", np_err);
        shutDownPlugin();
        return -1;
    }*/
    np_err = np_funcs.newp (mime, npp, NP_EMBED, argc, argn, argv, saved_data);
    if (np_err != NPERR_NO_ERROR) {
        g_printf ("NPP_GetValue NPP_New failure %d\n", np_err);
        return -1;
    }

    memset (&window, 0, sizeof (NPWindow));
    window.x = 0;
    window.y = 0;
    window.width = 320;
    window.height = 240;
    window.window = (void*)socket_id;
    ws_info.type = 1; /*NP_SetWindow;*/
    ws_info.display = (void*)(long)gdk_x11_display_get_xdisplay (gtk_widget_get_display (xembed));
    ws_info.visual = (void*)(long)gdk_x11_visual_get_xvisual (gdk_visual_get_system());
    ws_info.colormap = gdk_x11_colormap_get_xcolormap (gdk_colormap_get_system());
    ws_info.depth = gdk_drawable_get_depth (xembed->window);
    window.ws_info = (void*)&ws_info;

    np_err = np_funcs.setwindow (npp, &window);
    return 0;
}

static gboolean requestStream (void * p) {
    StreamInfo *si = (StreamInfo*) p;
    g_printf ("requestStream\n");
    if (si && si == g_slist_nth_data (stream_list, 0)) {
        uint16 stype = NP_NORMAL;
        g_assert (!stdin_read_watch);
        stdin_read_watch = gdk_input_add (0, GDK_INPUT_READ, readStdin, NULL);
        np_funcs.newstream (npp, si->mimetype, &si->np_stream, 0, &stype);

        DBusMessage *msg = dbus_message_new_method_call (
                callback_service,
                callback_path,
                "org.kde.kmplayer.callback",
                "getUrl");
        dbus_message_append_args (
                msg, DBUS_TYPE_STRING, &si->url, DBUS_TYPE_INVALID);
        dbus_message_set_no_reply (msg, TRUE);
        dbus_connection_send (dbus_connection, msg, NULL);
        dbus_message_unref (msg);
        dbus_connection_flush (dbus_connection);
    }
    return 0; /* single shot */
}

static gboolean destroyStream (void * p) {
    StreamInfo *si = (StreamInfo*) p;
    g_printf ("destroyStream\n");
    if (si && si == g_slist_nth_data (stream_list, 0)) {
        DBusMessage *msg = dbus_message_new_method_call (
                callback_service,
                callback_path,
                "org.kde.kmplayer.callback",
                "finish");
        dbus_message_set_no_reply (msg, TRUE);
        dbus_connection_send (dbus_connection, msg, NULL);
        dbus_message_unref (msg);
        dbus_connection_flush (dbus_connection);
    }
    return 0; /* single shot */
}

static gpointer newStream (const char *url, const char *mime) {
    StreamInfo *si;
    g_printf ("new stream %s %s\n", url, mime);
    if (!npp) {
        char *argn[] = { "WIDTH", "HEIGHT", "SRC", "debug" };
        char *argv[] = { "320", "240", url, "yes" };
        if (initPlugin (plugin) || newPlugin (mimetype, 2, argn, argv))
            return 0L;
    }
    si = addStream (url, mime, 0L, 0L, 0);
    return si;
}

/*----------------%<---------------------------------------------------------*/

static DBusHandlerResult dbusFilter (DBusConnection * connection,
        DBusMessage *message, void * user_data) {
    DBusMessageIter args;
    DBusMessage* reply;
    const char *sender = dbus_message_get_sender (message);
    const char *iface = "org.kde.kmplayer.backend";
    g_printf("dbusFilter %s %s\n", sender,dbus_message_get_interface (message));
    if (dbus_message_is_method_call (message, iface, "play")) {
        char *param = 0;
        if (dbus_message_iter_init (message, &args) && 
                DBUS_TYPE_STRING == dbus_message_iter_get_arg_type (&args)) {
            dbus_message_iter_get_basic (&args, &param);
            url = g_strdup (param);
            if (dbus_message_iter_next (&args) &&
                   DBUS_TYPE_STRING == dbus_message_iter_get_arg_type (&args)) {
                dbus_message_iter_get_basic (&args, &param);
                mimetype = g_strdup (param);
                if (dbus_message_iter_next (&args) && 
                        DBUS_TYPE_STRING == dbus_message_iter_get_arg_type (&args)) {
                    dbus_message_iter_get_basic (&args, &param);
                    plugin = g_strdup (param);
                    g_printf ("play %s %s %s", url, mimetype, plugin);
                    newStream (url, mimetype);
                }
            }
        }
    } else if (dbus_message_is_method_call (message, iface, "getUrlNotify")) {
        StreamInfo *si = (StreamInfo*) g_slist_nth_data (stream_list, 0);
        if (si && dbus_message_iter_init (message, &args) && 
                DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type (&args)) {
            dbus_message_iter_get_basic (&args, &si->total);
            if (dbus_message_iter_next (&args) &&
                   DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type (&args)) {
                dbus_message_iter_get_basic (&args, &si->reason);
                g_printf ("getUrlNotify bytes:%d reason:%d\n", si->total, si->reason);
                if (si->stream_pos == si->total)
                    removeStream (si->reason);
            }
        }
    } else if (dbus_message_is_method_call (message, iface, "quit")) {
        g_printf ("quit\n");
        shutDownPlugin();
        gtk_main_quit();
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

/*----------------%<---------------------------------------------------------*/

static void pluginAdded (GtkSocket *socket, gpointer d) {
    g_printf ("pluginAdded\n");
}

static void windowCreatedEvent (GtkWidget *w, gpointer d) {
    socket_id = gtk_socket_get_id (GTK_SOCKET (xembed));
    if (!callback_service)
        newStream (url, mimetype);
}

static gboolean windowCloseEvent (GtkWidget *w, GdkEvent *e, gpointer d) {
    shutDownPlugin();
    return FALSE;
}

static void windowDestroyEvent (GtkWidget *w, gpointer d) {
    gtk_main_quit();
}

int main (int argc, char **argv) {
    int i;
    gtk_init (&argc, &argv);

    for (i = 1; i < argc; i++) {
        if (!strcmp (argv[i], "-p") && ++i < argc) {
            plugin = g_strdup (argv[i]);
        } else if (!strcmp (argv[i], "-cb") && ++i < argc) {
            gchar *cb = g_strdup (argv[i]);
            gchar *path = strchr(cb, '/');
            if (path) {
                callback_path = g_strdup (path);
                *path = 0;
            }
            callback_service = g_strdup (cb);
            g_free (cb);
        } else if (!strcmp (argv[i], "-m") && ++i < argc) {
            mimetype = g_strdup (argv[i]);
        } else if (!strcmp (argv [i], "-wid") && ++i < argc) {
            socket_id = strtol (argv[i], 0L, 10);
        } else
            url = g_strdup (argv[i]);
    }
    if (!callback_service && !(url && mimetype && plugin)) {
        g_fprintf(stderr, "Usage: %s <-m mimetype -p plugin url|-cb service -wid id>\n", argv[0]);
        return 1;
    }
    /*when called from kmplayer if (!callback_service) {*/
        GtkWidget *window;
        GdkColormap *color_map;
        GdkColor bg_color;

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        g_signal_connect (G_OBJECT (window), "delete_event",
                G_CALLBACK (windowCloseEvent), NULL);
        g_signal_connect (G_OBJECT (window), "destroy",
                G_CALLBACK (windowDestroyEvent), NULL);
        g_signal_connect (G_OBJECT (window), "realize",
                GTK_SIGNAL_FUNC (windowCreatedEvent), NULL);

        xembed = gtk_socket_new();
        g_signal_connect (G_OBJECT (xembed), "plug-added",
                GTK_SIGNAL_FUNC (pluginAdded), NULL);
        color_map = gdk_colormap_get_system();
        gdk_colormap_query_color (color_map, 0, &bg_color);
        gtk_widget_modify_bg (xembed, GTK_STATE_NORMAL, &bg_color);

        gtk_container_add (GTK_CONTAINER (window), xembed);
        gtk_widget_set_size_request (window, 320, 240);
        gtk_widget_show_all (window);
    /*} else {*/
    if (callback_service && callback_path) {
        DBusMessage *msg;
        DBusError dberr;
        const char *serv = "type='method_call',interface='org.kde.kmplayer.backend'";
        char myname[64];

        dbus_error_init (&dberr);
        dbus_connection = dbus_bus_get (DBUS_BUS_SESSION, &dberr);
        if (!dbus_connection) {
            g_printerr ("Failed to open connection to bus: %s\n",
                    dberr.message);
            exit (1);
        }
        g_sprintf (myname, "org.kde.kmplayer.npplayer-%d", getpid ());
        service_name = g_strdup (myname);
        g_printf ("using service %s was '%s'\n", service_name, dbus_bus_get_unique_name (dbus_connection));
        dbus_connection_setup_with_g_main (dbus_connection, 0L);
        dbus_bus_request_name (dbus_connection, service_name, 
                DBUS_NAME_FLAG_REPLACE_EXISTING, &dberr);
        if (dbus_error_is_set (&dberr)) {
            g_printerr ("Failed to register name: %s\n", dberr.message);
            dbus_connection_unref (dbus_connection);
            return -1;
        }
        dbus_bus_add_match (dbus_connection, serv, &dberr);
        if (dbus_error_is_set (&dberr)) {
            g_printerr ("dbus_bus_add_match error: %s\n", dberr.message);
            dbus_connection_unref (dbus_connection);
            return -1;
        }
        dbus_connection_add_filter (dbus_connection, dbusFilter, 0L, 0L);

        /* TODO: remove DBUS_BUS_SESSION and create a private connection */
        msg = dbus_message_new_method_call (
                callback_service,
                callback_path,
                "org.kde.kmplayer.callback",
                "running");
        dbus_message_append_args (
                msg, DBUS_TYPE_STRING, &service_name, DBUS_TYPE_INVALID);
        dbus_message_set_no_reply (msg, TRUE);
        dbus_connection_send (dbus_connection, msg, NULL);
        dbus_message_unref (msg);

        dbus_connection_flush (dbus_connection);
    }

    fcntl (0, F_SETFL, fcntl (0, F_GETFL) | O_NONBLOCK);

    gtk_main();

    if (dbus_connection)
        dbus_connection_unref (dbus_connection);

    return 0;
}