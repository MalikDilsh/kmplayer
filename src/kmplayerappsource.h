/***************************************************************************
                          kmplayersource.h  -  description
                             -------------------
    begin                : Sat Mar  24 16:14:51 CET 2003
    copyright            : (C) 2003 by Koos Vriezen
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KMPLAYERAPPSOURCE_H
#define KMPLAYERAPPSOURCE_H
 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qptrlist.h>
#include <qmap.h>

#include <kurl.h>

#include "kmplayersource.h"

class KMPlayerApp;
class QPopupMenu;
class QMenuItem;
class TVInput;
class TVChannel;

class KMPlayerAppURLSource : public KMPlayerURLSource {
    Q_OBJECT
public:
    KMPlayerAppURLSource (KMPlayerApp * app);
    virtual ~KMPlayerAppURLSource ();
public slots:
    virtual void activate ();
private:
    KMPlayerApp * m_app;
};


class KMPlayerMenuSource : public KMPlayerSource {
    Q_OBJECT
public:
    KMPlayerMenuSource (KMPlayerApp * app, QPopupMenu * m);
    virtual ~KMPlayerMenuSource ();
protected:
    void menuItemClicked (QPopupMenu * menu, int id);
    QPopupMenu * m_menu;
    KMPlayerApp * m_app;
};


class KMPlayerDVDSource : public KMPlayerMenuSource {
    Q_OBJECT
public:
    KMPlayerDVDSource (KMPlayerApp * app, QPopupMenu * m);
    virtual ~KMPlayerDVDSource ();
    virtual bool processOutput (const QString & line);
    virtual QString filterOptions ();
    virtual void setIdentified (bool b = true);
    virtual QString prettyName ();
public slots:
    virtual void activate ();
    virtual void deactivate ();

    void titleMenuClicked (int id);
    void subtitleMenuClicked (int id);
    void languageMenuClicked (int id);
    void chapterMenuClicked (int id);
private:
    void buildArguments ();
    void play ();
    QRegExp langRegExp;
    QRegExp subtitleRegExp;
    QRegExp titleRegExp;
    QRegExp chapterRegExp;
    QPopupMenu * m_dvdtitlemenu;
    QPopupMenu * m_dvdchaptermenu;
    QPopupMenu * m_dvdlanguagemenu;
    QPopupMenu * m_dvdsubtitlemenu;
    int m_current_title;
    bool m_start_play;
};


class KMPlayerDVDNavSource : public KMPlayerMenuSource {
    Q_OBJECT
public:
    KMPlayerDVDNavSource (KMPlayerApp * app, QPopupMenu * m);
    virtual ~KMPlayerDVDNavSource ();
    virtual QString prettyName ();
public slots:
    virtual void activate ();
    virtual void deactivate ();
    virtual void play ();

    void finished ();
    void navMenuClicked (int id);
};


class KMPlayerVCDSource : public KMPlayerMenuSource {
    Q_OBJECT
public:
    KMPlayerVCDSource (KMPlayerApp * app, QPopupMenu * m);
    virtual ~KMPlayerVCDSource ();
    virtual bool processOutput (const QString & line);
    virtual void setIdentified (bool b = true);
    virtual QString prettyName ();
public slots:
    virtual void activate ();
    virtual void deactivate ();

private slots:
    void trackMenuClicked (int id);
private:
    void buildArguments ();
    QRegExp trackRegExp;
    QPopupMenu * m_vcdtrackmenu;
    int m_current_title;
    bool m_start_play;
};


class KMPlayerPipeSource : public KMPlayerSource {
    Q_OBJECT
public:
    KMPlayerPipeSource (KMPlayerApp * app);
    virtual ~KMPlayerPipeSource ();
    virtual bool hasLength ();
    virtual bool isSeekable ();
    void setCommand (const QString & cmd) { m_pipecmd = cmd; }
    virtual QString prettyName ();
public slots:
    virtual void activate ();
    virtual void deactivate ();
private:
    KMPlayerApp * m_app;
};


class KMPlayerTVSource : public KMPlayerMenuSource {
    Q_OBJECT
public:
    struct TVSource {
        QSize size;
        QString command;
        QString videodevice;
        QString audiodevice;
        QString title;
        QString norm;
        int frequency;
        bool noplayback;
    };
    KMPlayerTVSource (KMPlayerApp * app, QPopupMenu * m);
    virtual ~KMPlayerTVSource ();
    virtual QString filterOptions ();
    virtual bool hasLength ();
    virtual bool isSeekable ();
    void buildMenu ();
    TVSource * tvsource () const { return m_tvsource; }
    virtual QString prettyName ();
public slots:
    virtual void activate ();
    virtual void deactivate ();

    void menuClicked (int id);
private:
    void buildArguments ();
    typedef QMap <int, TVSource *> CommandMap;
    TVSource * m_tvsource;
    CommandMap commands;
    QPopupMenu * m_channelmenu;
};

#endif // KMPLAYERAPPSOURCE_H
