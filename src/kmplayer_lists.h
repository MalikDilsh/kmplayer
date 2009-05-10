/**
  This file belong to the KMPlayer project, a movie player plugin for Konqueror
  Copyright (C) 2009  Koos Vriezen <koos.vriezen@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**/

#ifndef _KMPLAYER_LISTS_H_
#define _KMPLAYER_LISTS_H_

#include <config-kmplayer.h>

#include "kmplayer_def.h"
#include "kmplayerplaylist.h"
#include "kmplayersource.h"
#include "kmplayerpartbase.h"

static const short id_node_recent_document = 31;
static const short id_node_recent_node = 32;
static const short id_node_disk_document = 33;
static const short id_node_disk_node = 34;

class KMPlayerApp;

class KMPLAYER_NO_EXPORT ListsSource : public KMPlayer::URLSource {
public:
    KDE_NO_CDTOR_EXPORT ListsSource (KMPlayer::PartBase * p)
        : KMPlayer::URLSource (p, KUrl ("lists://")) {}
    void play (KMPlayer::Mrl *);
    void activate ();
    QString prettyName () { return m_document->caption (); }
};

class KMPLAYER_NO_EXPORT FileDocument : public KMPlayer::SourceDocument {
public:
    FileDocument (short id, const QString&, KMPlayer::Source *source = 0L);
    KMPlayer::Node *childFromTag (const QString &tag);
    void readFromFile (const QString &file);
    void writeToFile (const QString &file);
};

class KMPLAYER_NO_EXPORT Recents : public FileDocument {
public:
    Recents (KMPlayerApp *a);
    void defer ();
    void activate ();
    void message (KMPlayer::MessageType msg, void *content=NULL);
    KMPlayer::Node *childFromTag (const QString &tag);
    KDE_NO_EXPORT const char *nodeName () const { return "playlist"; }
    KMPlayerApp *app;
};

class KMPLAYER_NO_EXPORT Recent : public KMPlayer::Mrl {
public:
    Recent (KMPlayer::NodePtr & doc, KMPlayerApp *a, const QString &url = QString());
    void activate ();
    void closed ();
    KDE_NO_EXPORT const char *nodeName () const { return "item"; }
    KMPlayerApp *app;
};

class KMPLAYER_NO_EXPORT Group : public KMPlayer::Title {
public:
    Group (KMPlayer::NodePtr &doc, KMPlayerApp *a, const QString &pn=QString());
    KMPlayer::Node *childFromTag (const QString &tag);
    void defer () {} // TODO lazy loading of largish sub trees
    void closed ();
    KDE_NO_EXPORT const char *nodeName () const { return "group"; }
    KMPlayerApp *app;
};

class KMPLAYER_NO_EXPORT Playlist : public FileDocument {
public:
    Playlist (KMPlayerApp *a, KMPlayer::Source *s, bool plmod = false);
    void message (KMPlayer::MessageType msg, void *content=NULL);
    void defer ();
    void activate ();
    KMPlayer::Node *childFromTag (const QString &tag);
    KDE_NO_EXPORT const char * nodeName () const { return "playlist"; }
    KMPlayerApp *app;
    bool playmode;
};

class KMPLAYER_NO_EXPORT PlaylistItemBase : public KMPlayer::Mrl {
public:
    PlaylistItemBase (KMPlayer::NodePtr &d, short id, KMPlayerApp *a, bool pm);
    void activate ();
    void closed ();
    KMPlayerApp *app;
    bool playmode;
};

class KMPLAYER_NO_EXPORT PlaylistItem : public PlaylistItemBase {
public:
    PlaylistItem (KMPlayer::NodePtr & doc, KMPlayerApp *a, bool playmode, const QString &url = QString());
    void closed ();
    void begin ();
    void setNodeName (const QString&);
    const char *nodeName () const KDE_NO_EXPORT { return "item"; }
};

class KMPLAYER_NO_EXPORT PlaylistGroup : public KMPlayer::Title {
public:
    PlaylistGroup (KMPlayer::NodePtr &doc, KMPlayerApp *a, const QString &pn);
    PlaylistGroup (KMPlayer::NodePtr &doc, KMPlayerApp *a, bool plmode=false);
    KMPlayer::Node *childFromTag (const QString &tag);
    void closed ();
    void setNodeName (const QString&);
    KDE_NO_EXPORT const char *nodeName () const { return "group"; }
    KMPlayerApp *app;
    bool playmode;
};

class KMPLAYER_NO_EXPORT HtmlObject : public PlaylistItemBase {
public:
    HtmlObject (KMPlayer::NodePtr &doc, KMPlayerApp *a, bool playmode);
    void activate ();
    void closed ();
    KMPlayer::Node *childFromTag (const QString &tag);
    const char *nodeName () const KDE_NO_EXPORT { return "object"; }
};

#endif
