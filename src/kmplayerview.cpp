/**
 * Copyright (C) 2002-2003 by Koos Vriezen <koos ! vriezen ? xs4all ! nl>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/

#include <stdio.h>
#include <math.h>

#include <config.h>
// include files for Qt
#include <qstyle.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qmetaobject.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtextedit.h>
#include <qtooltip.h>
#include <qapplication.h>
#include <qiconset.h>
#include <qaccel.h>
#include <qcursor.h>
#include <qkeysequence.h>
#include <qslider.h>
#include <qlabel.h>
#include <qdatastream.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qwidgetstack.h>
#include <qheader.h>
#include <qcursor.h>
#include <qclipboard.h>

#include <kiconloader.h>

#include "kmplayerview.h"
#include "kmplayersource.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

static const int XKeyPress = KeyPress;
#undef KeyPress
#undef Always
#undef Never
#undef Status
#undef Unsorted
#undef Bool

static const int button_height_with_slider = 15;
static const int button_height_only_buttons = 11;

// application specific includes
//#include "kmplayer.h"

#include <kstaticdeleter.h>
#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>
#include <kactioncollection.h>
#include <kshortcut.h>
#include <kurldrag.h>
#include <dcopclient.h>
#include <kglobalsettings.h>


/* mouse invisible: define the time (in 1/1000 seconds) before mouse goes invisible */
#define MOUSE_INVISIBLE_DELAY 2000


using namespace KMPlayer;

//-------------------------------------------------------------------------

static char xpm_fg_color [32] = ".      c #000000";

static const char * stop_xpm[] = {
    "5 7 2 1",
    "       c None",
    xpm_fg_color,
    "     ",
    ".....",
    ".....",
    ".....",
    ".....",
    ".....",
    "     "};

static const char * play_xpm[] = {
    "5 9 2 1",
    "       c None",
    xpm_fg_color,
    ".    ",
    "..   ",
    "...  ",
    ".... ",
    ".....",
    ".... ",
    "...  ",
    "..   ",
    ".    "};

static const char * pause_xpm[] = {
    "7 9 2 1",
    "       c None",
    xpm_fg_color,
    "       ",
    "..   ..",
    "..   ..",
    "..   ..",
    "..   ..",
    "..   ..",
    "..   ..",
    "..   ..",
    "       "};

static const char * forward_xpm[] = {
    "11 9 2 1",
    "       c None",
    xpm_fg_color,
    ".     .    ",
    "..    ..   ",
    "...   ...  ",
    "....  .... ",
    "..... .....",
    "....  .... ",
    "...   ...  ",
    "..    ..   ",
    ".     .    "};

static const char * back_xpm[] = {
    "11 9 2 1",
    "       c None",
    xpm_fg_color,
    "    .     .",
    "   ..    ..",
    "  ...   ...",
    " ....  ....",
    "..... .....",
    " ....  ....",
    "  ...   ...",
    "   ..    ..",
    "    .     ."};

static const char * config_xpm[] = {
    "11 8 2 1",
    "       c None",
    xpm_fg_color,
    "           ",
    "           ",
    "...........",
    " ......... ",
    "  .......  ",
    "   .....   ",
    "    ...    ",
    "     .     "};

static const char * record_xpm[] = {
    "7 7 3 1",
    "       c None",
    xpm_fg_color,
    "+      c #FF0000",
    "       ",
    ".......",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".......",
    "       "};

static const char * broadcast_xpm[] = {
"21 9 2 1",
"       c None",
xpm_fg_color,
"                     ",
" ..  ..       ..  .. ",
"..  ..   ...   ..  ..",
"..  ..  .....  ..  ..",
"..  ..  .....  ..  ..",
"..  ..  .....  ..  ..",
"..  ..   ...   ..  ..",
" ..  ..       ..  .. ",
"                     "};

static const char * red_xpm[] = {
    "7 9 3 1",
    "       c None",
    xpm_fg_color,
    "+      c #FF0000",
    "       ",
    ".......",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".......",
    "       "};

static const char * green_xpm[] = {
    "7 9 3 1",
    "       c None",
    xpm_fg_color,
    "+      c #00FF00",
    "       ",
    ".......",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".......",
    "       "};

static const char * yellow_xpm[] = {
    "7 9 3 1",
    "       c None",
    xpm_fg_color,
    "+      c #FFFF00",
    "       ",
    ".......",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".......",
    "       "};

static const char * blue_xpm[] = {
    "7 9 3 1",
    "       c None",
    xpm_fg_color,
    "+      c #0080FF00",
    "       ",
    ".......",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".+++++.",
    ".......",
    "       "};

//-----------------------------------------------------------------------------

static bool isDragValid (QDropEvent * de) {
    if (KURLDrag::canDecode (de))
        return true;
    if (QTextDrag::canDecode (de)) {
        QString text;
        if (KURL (QTextDrag::decode (de, text)).isValid ())
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT ViewArea::ViewArea (QWidget * parent, View * view)
 : QWidget (parent),
   m_parent (parent),
   m_view (view),
   m_painter (0L),
   m_paint_buffer (0L),
   m_collection (new KActionCollection (this)),
   m_mouse_invisible_timer (0),
   m_repaint_timer (0),
   m_fullscreen (false) {
    setEraseColor (QColor (0, 0, 0));
    setAcceptDrops (true);
    new KAction (i18n ("Escape"), KShortcut (Qt::Key_Escape), this, SLOT (accelActivated ()), m_collection, "view_fullscreen_escape");
    new KAction (i18n ("Fullscreen"), KShortcut (Qt::Key_F), this, SLOT (accelActivated ()), m_collection, "view_fullscreen_toggle");
    setMouseTracking (true);
    setWFlags (getWFlags () | WRepaintNoErase);
}

KDE_NO_CDTOR_EXPORT ViewArea::~ViewArea () {
    delete m_painter;
    delete m_paint_buffer;
}

KDE_NO_EXPORT void ViewArea::fullScreen () {
    if (m_fullscreen) {
        showNormal ();
        reparent (m_parent, 0, QPoint (0, 0), true);
        static_cast <KDockWidget *> (m_parent)->setWidget (this);
        for (unsigned i = 0; i < m_collection->count (); ++i)
            m_collection->action (i)->setEnabled (false);
    } else {
        reparent (0L, 0, qApp->desktop()->screenGeometry(this).topLeft(), true);
        showFullScreen ();
        for (unsigned i = 0; i < m_collection->count (); ++i)
            m_collection->action (i)->setEnabled (true);

    }
    m_fullscreen = !m_fullscreen;
    m_view->controlPanel()->popupMenu ()->setItemChecked (ControlPanel::menu_fullscreen, m_fullscreen);

    if (m_fullscreen) {
        m_mouse_invisible_timer = startTimer(MOUSE_INVISIBLE_DELAY);
    } else {
        if (m_mouse_invisible_timer) {
            killTimer (m_mouse_invisible_timer);
            m_mouse_invisible_timer = 0;
        }
        unsetCursor();
    }
}

KDE_NO_EXPORT void ViewArea::accelActivated () {
    m_view->controlPanel()->popupMenu ()->activateItemAt (m_view->controlPanel()->popupMenu ()->indexOf (ControlPanel::menu_fullscreen)); 
}

KDE_NO_EXPORT void ViewArea::mousePressEvent (QMouseEvent * e) {
    if (rootLayout && rootLayout->handleEvent((new PointerEvent(event_pointer_clicked,e->x(), e->y()))->self()))
        e->accept ();
}

KDE_NO_EXPORT void ViewArea::mouseMoveEvent (QMouseEvent * e) {
    if (e->state () == Qt::NoButton) {
        int vert_buttons_pos = height ();
        int cp_height = m_view->controlPanel ()->maximumSize ().height ();
        m_view->delayedShowButtons (e->y() > vert_buttons_pos-cp_height &&
                                    e->y() < vert_buttons_pos);
    }
    if (rootLayout && rootLayout->handleEvent((new PointerEvent(event_pointer_moved,e->x(), e->y()))->self()))
        e->accept ();
    mouseMoved (); // for m_mouse_invisible_timer
}

KDE_NO_EXPORT void ViewArea::paintEvent (QPaintEvent * pe) {
#define PAINT_BUFFER_HEIGHT 128
    if (rootLayout) {
        if (!m_paint_buffer) {
            m_paint_buffer = new QPixmap (width (), PAINT_BUFFER_HEIGHT);
            m_painter = new QPainter ();
        } else if (m_paint_buffer->width () < width ())
            m_paint_buffer->resize (width (), PAINT_BUFFER_HEIGHT);
        int py=0;
        int ex = pe->rect ().x ();
        int ey = pe->rect ().y ();
        int ew = pe->rect ().width ();
        int eh = pe->rect ().height ();
        //QPainter p;
        //p.begin (this);
        while (py < eh) {
            int ph = eh-py < PAINT_BUFFER_HEIGHT ? eh-py : PAINT_BUFFER_HEIGHT;
            m_painter->begin (m_paint_buffer);
            m_painter->translate(-ex, -ey-py);
            m_painter->fillRect (ex, ey+py, ew, ph, QBrush (paletteBackgroundColor ()));
            rootLayout->handleEvent ((new PaintEvent (*m_painter, ex, ey+py, ew, ph))->self ());
            m_painter->end();
            //p.drawPixmap (ex, ey+py, *m_paint_buffer, 0, 0, ew, ph);
            bitBlt (this, ex, ey+py, m_paint_buffer, 0, 0, ew, ph);
            py += PAINT_BUFFER_HEIGHT;
        }
        //p.end ();
    } else
        QWidget::paintEvent (pe);
}

KDE_NO_EXPORT void ViewArea::resizeEvent (QResizeEvent *) {
    if (!m_view->controlPanel ()) return;
    int x =0, y = 0;
    int w = width ();
    int h = height ();
    int hcp = m_view->controlPanel ()->isVisible () ? (m_view->controlPanelMode () == View::CP_Only ? h : m_view->controlPanel()->maximumSize ().height ()) : 0;
    int wws = w;
    // move controlpanel over video when autohiding and playing
    int hws = h - (m_view->controlPanelMode () == View::CP_AutoHide && m_view->widgetStack ()->visibleWidget () == m_view->viewer () ? 0 : hcp);

    // now scale the regions and check if video region is already sized
    bool av_geometry_changed = false;
    if (rootLayout && wws > 0 && hws > 0) {
        m_av_geometry = QRect (0, 0, 0, 0);
        rootLayout->handleEvent ((new SizeEvent (x, y, wws, hws, m_view->keepSizeRatio ()))->self ());
        av_geometry_changed = (m_av_geometry != QRect (0, 0, 0, 0));
        x = m_av_geometry.x ();
        y = m_av_geometry.y ();
        wws = m_av_geometry.width ();
        hws = m_av_geometry.height ();
            //m_view->viewer ()->setAspect (region->w / region->h);
    } else
        m_av_geometry = QRect (x, y, wws, hws);

    // finally resize controlpanel and video widget
    if (m_view->controlPanel ()->isVisible ())
        m_view->controlPanel ()->setGeometry (0, h-hcp, w, hcp);
    if (!av_geometry_changed)
        setAudioVideoGeometry (x, y, wws, hws, 0L);
}

KDE_NO_EXPORT
void ViewArea::setAudioVideoGeometry (int x, int y, int w, int h, unsigned int * bg_color) {
    if (m_view->controlPanelMode() == View::CP_Only) {
        w = h = 0;
    } else if (m_view->keepSizeRatio ()) { // scale video widget inside region
        int hfw = m_view->viewer ()->heightForWidth (w);
        if (hfw > 0)
            if (hfw > h) {
                int old_w = w;
                w = int ((1.0 * h * w)/(1.0 * hfw));
                x += (old_w - w) / 2;
            } else {
                y += (h - hfw) / 2;
                h = hfw;
            }
    }
    m_av_geometry = QRect (x, y, w, h);
    if (m_av_geometry != m_view->widgetStack ()->geometry ()) {
        m_view->widgetStack ()->setGeometry (x, y, w, h);
        scheduleRepaint (0, 0, width (), height ());
    }
    if (bg_color)
        if (QColor (QRgb (*bg_color)) != (m_view->viewer ()->paletteBackgroundColor ())) {
            m_view->viewer()->setPaletteBackgroundColor (QColor (QRgb (*bg_color)));
            scheduleRepaint (x, y, w, h);
        }
}

KDE_NO_EXPORT void ViewArea::setRootLayout (NodePtr rl) {
    if (rootLayout != rl) {
        rootLayout = rl;
        resizeEvent (0L);
    }
}

KDE_NO_EXPORT void ViewArea::showEvent (QShowEvent *) {
    resizeEvent (0L);
}

KDE_NO_EXPORT void ViewArea::dropEvent (QDropEvent * de) {
    m_view->dropEvent (de);
}

KDE_NO_EXPORT void ViewArea::dragEnterEvent (QDragEnterEvent* dee) {
    m_view->dragEnterEvent (dee);
}

KDE_NO_EXPORT void ViewArea::contextMenuEvent (QContextMenuEvent * e) {
    m_view->controlPanel ()->popupMenu ()->exec (e->globalPos ());
}

KDE_NO_EXPORT void ViewArea::mouseMoved () {
    if (m_fullscreen) {
        if (m_mouse_invisible_timer)
            killTimer (m_mouse_invisible_timer);
        unsetCursor ();
        m_mouse_invisible_timer = startTimer (MOUSE_INVISIBLE_DELAY);
    }
}

KDE_NO_EXPORT void ViewArea::scheduleRepaint (int x, int y, int w, int h) {
    if (m_repaint_timer)
        m_repaint_rect = m_repaint_rect.unite (QRect (x, y, w, h));
    else {
        m_repaint_rect = QRect (x, y, w, h);
        m_repaint_timer = startTimer (40); // 25 per sec should do
    }
}

KDE_NO_EXPORT
void ViewArea::moveRect (int x, int y, int w, int h, int x1, int y1) {
    QRect r (x, y, w, h);
    if ((m_repaint_timer && m_repaint_rect.intersects (r)) ||
            m_view->viewer()->frameGeometry ().intersects (r)) {
            m_repaint_rect = m_repaint_rect.unite(QRect(x1, y1, w, h).unite(r));
    } else {
        bitBlt (this, x1, y1, this, x, y, w, h);
        if (x1 > x) {
            repaint (x, y, x1 - x, h, false);
        } else if (x > x1) {
            repaint (x1 + w, y, x - x1, h, false);
        }
        if (y1 > y) {
            repaint (x, y, w, y1 - y, false);
        } else if (y > y1) {
            repaint (x, y1 + h, w, y - y1, false);
        }
    }
}

KDE_NO_EXPORT void ViewArea::timerEvent (QTimerEvent * e) {
    if (e->timerId () == m_mouse_invisible_timer) {
        m_mouse_invisible_timer = 0;
        if (m_fullscreen)
            setCursor (BlankCursor);
    } else if (e->timerId () == m_repaint_timer) {
        m_repaint_timer = 0;
        repaint (m_repaint_rect, false);
    }
    killTimer (e->timerId ());
}



//-----------------------------------------------------------------------------

static QPushButton * ctrlButton (QWidget * w, QBoxLayout * l, const char ** p, int key = 0) {
    QPushButton * b = new QPushButton (QIconSet (QPixmap(p)), QString::null, w);
    b->setFocusPolicy (QWidget::NoFocus);
    b->setFlat (true);
    if (key)
        b->setAccel (QKeySequence (key));
    l->addWidget (b);
    return b;
}

KDE_NO_CDTOR_EXPORT
KMPlayerControlButton::KMPlayerControlButton (QWidget * parent, QBoxLayout * l, const char ** p, int key)
 : QPushButton (QIconSet (QPixmap(p)), QString::null, parent, "kde_kmplayer_control_button") {
   setFocusPolicy (QWidget::NoFocus);
   setFlat (true);
   if (key)
       setAccel (QKeySequence (key));
   l->addWidget (this);
}

KDE_NO_EXPORT void KMPlayerControlButton::enterEvent (QEvent *) {
    emit mouseEntered ();
}
        
//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT VolumeBar::VolumeBar (QWidget * parent, View * view)
 : QWidget (parent), m_view (view), m_value (100) {
    setSizePolicy( QSizePolicy (QSizePolicy::Minimum, QSizePolicy::Fixed));
    setMinimumSize (QSize (51, button_height_only_buttons + 2));
    QToolTip::add (this, i18n ("Volume is %1").arg (m_value));
}

KDE_NO_CDTOR_EXPORT VolumeBar::~VolumeBar () {
}

void VolumeBar::setValue (int v) {
    m_value = v;
    if (m_value < 0) m_value = 0;
    if (m_value > 100) m_value = 100;
    QToolTip::remove (this);
    QToolTip::add (this, i18n ("Volume is %1").arg (m_value));
    repaint (true);
    emit volumeChanged (m_value);
}

void VolumeBar::wheelEvent (QWheelEvent * e) {
    setValue (m_value + (e->delta () > 0 ? 2 : -2));
    e->accept ();
}

void VolumeBar::paintEvent (QPaintEvent * e) {
    QWidget::paintEvent (e);
    QPainter p;
    p.begin (this);
    QColor color = paletteForegroundColor ();
    p.setPen (color);
    int w = width () - 6;
    int vx = m_value * w / 100;
    p.fillRect (3, 3, vx, 7, color);
    p.drawRect (vx + 3, 3, w - vx, 7);
    p.end ();
    //kdDebug () << "w=" << w << " vx=" << vx << endl;
}

void VolumeBar::mousePressEvent (QMouseEvent * e) {
    setValue (100 * (e->x () - 3) / (width () - 6));
    e->accept ();
}

void VolumeBar::mouseMoveEvent (QMouseEvent * e) {
    setValue (100 * (e->x () - 3) / (width () - 6));
    e->accept ();
}

//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT ControlPanel::ControlPanel(QWidget * parent, View * view)
 : QWidget (parent),
   m_progress_mode (progress_playing),
   m_progress_length (0),
   m_view (view),
   m_auto_controls (true) {
    m_buttonbox = new QHBoxLayout (this, 5, 4);
    QColor c = paletteForegroundColor ();
    strncpy (xpm_fg_color, QString().sprintf(".      c #%02x%02x%02x", c.red(), c.green(),c.blue()).ascii(), 31);
    xpm_fg_color[31] = 0;
    m_buttons[button_config] = new KMPlayerControlButton (this, m_buttonbox, config_xpm);
    m_buttons[button_back] = ctrlButton (this, m_buttonbox, back_xpm);
    m_buttons[button_play] = ctrlButton (this, m_buttonbox, play_xpm, Qt::Key_R);
    m_buttons[button_forward] = ctrlButton (this, m_buttonbox, forward_xpm);
    m_buttons[button_stop] = ctrlButton (this, m_buttonbox, stop_xpm, Qt::Key_S);
    m_buttons[button_pause] = ctrlButton (this, m_buttonbox, pause_xpm, Qt::Key_P);
    m_buttons[button_record] = ctrlButton (this, m_buttonbox, record_xpm);
    m_buttons[button_broadcast] = ctrlButton (this, m_buttonbox, broadcast_xpm);
    m_buttons[button_red] = ctrlButton (this, m_buttonbox, red_xpm);
    m_buttons[button_green] = ctrlButton (this, m_buttonbox, green_xpm);
    m_buttons[button_yellow] = ctrlButton (this, m_buttonbox, yellow_xpm);
    m_buttons[button_blue] = ctrlButton (this, m_buttonbox, blue_xpm);
    m_buttons[button_play]->setToggleButton (true);
    m_buttons[button_stop]->setToggleButton (true);
    m_buttons[button_record]->setToggleButton (true);
    m_buttons[button_broadcast]->setToggleButton (true);
    m_posSlider = new QSlider (Qt::Horizontal, this);
    m_posSlider->setEnabled (false);
    m_buttonbox->addWidget (m_posSlider);
    showPositionSlider (true);
    m_volume = new VolumeBar (this, m_view);
    m_buttonbox->addWidget (m_volume);
    m_popupMenu = new KMPlayerPopupMenu (this);
    m_playerMenu = new KMPlayerPopupMenu (this);
    m_popupMenu->insertItem (i18n ("&Play with"), m_playerMenu, menu_player);
    m_bookmarkMenu = new KMPlayerPopupMenu (this);
    m_popupMenu->insertItem (i18n("&Bookmarks"), m_bookmarkMenu, menu_bookmark);
    m_viewMenu = new KMPlayerPopupMenu (this);
    m_viewMenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("video"), KIcon::Small, 0, true), i18n ("V&ideo"), menu_video);
    m_viewMenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("player_playlist"), KIcon::Small, 0, true), i18n ("Pla&y List"), menu_playlist);
    m_viewMenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("konsole"), KIcon::Small, 0, true), i18n ("Con&sole"), menu_console);
    m_popupMenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("window"), KIcon::Small, 0, false), i18n ("&View"), m_viewMenu, menu_view);
    m_zoomMenu = new KMPlayerPopupMenu (this);
    m_zoomMenu->insertItem (i18n ("50%"), menu_zoom50);
    m_zoomMenu->insertItem (i18n ("100%"), menu_zoom100);
    m_zoomMenu->insertItem (i18n ("150%"), menu_zoom150);
    m_popupMenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("viewmag"), KIcon::Small, 0, false), i18n ("&Zoom"), m_zoomMenu, menu_zoom);
    m_popupMenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("window_fullscreen"), KIcon::Small, 0, true), i18n ("&Full Screen"), menu_fullscreen);
    m_popupMenu->setAccel (QKeySequence (Qt::Key_F), menu_fullscreen);
    m_popupMenu->insertSeparator ();
    m_colorMenu = new KMPlayerPopupMenu (this);
    QLabel * label = new QLabel (i18n ("Contrast:"), m_colorMenu);
    m_colorMenu->insertItem (label);
    m_contrastSlider = new QSlider (-100, 100, 10, 0, Qt::Horizontal, m_colorMenu);
    m_colorMenu->insertItem (m_contrastSlider);
    label = new QLabel (i18n ("Brightness:"), m_colorMenu);
    m_colorMenu->insertItem (label);
    m_brightnessSlider = new QSlider (-100, 100, 10, 0, Qt::Horizontal, m_colorMenu);
    m_colorMenu->insertItem (m_brightnessSlider);
    label = new QLabel (i18n ("Hue:"), m_colorMenu);
    m_colorMenu->insertItem (label);
    m_hueSlider = new QSlider (-100, 100, 10, 0, Qt::Horizontal, m_colorMenu);
    m_colorMenu->insertItem (m_hueSlider);
    label = new QLabel (i18n ("Saturation:"), m_colorMenu);
    m_colorMenu->insertItem (label);
    m_saturationSlider = new QSlider (-100, 100, 10, 0, Qt::Horizontal, m_colorMenu);
    m_colorMenu->insertItem (m_saturationSlider);
    m_popupMenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("colorize"), KIcon::Small, 0, true), i18n ("Co&lors"), m_colorMenu);
    m_popupMenu->insertSeparator ();
    m_popupMenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("configure"), KIcon::Small, 0, true), i18n ("&Configure KMPlayer..."), menu_config);
    setAutoControls (true);
}

void ControlPanel::setAutoControls (bool b) {
    m_auto_controls = b;
    if (m_auto_controls) {
        for (int i = 0; i < (int) button_broadcast; i++)
            m_buttons [i]->show ();
        for (int i = button_broadcast; i < (int) button_last; i++)
            m_buttons [i]->hide ();
        showPositionSlider (true);
        m_volume->show ();
        if (m_buttons [button_broadcast]->isOn ()) // still broadcasting
            m_buttons [button_broadcast]->show ();
    } else { // hide everything
        for (int i = 0; i < (int) button_last; i++)
            m_buttons [i]->hide ();
        m_posSlider->hide ();
        m_volume->hide ();
    }
    m_view->updateLayout ();
}

void ControlPanel::showPositionSlider (bool show) {
    if (!m_auto_controls) return;
    int h = show ? button_height_with_slider : button_height_only_buttons;
    m_posSlider->setEnabled (false);
    m_posSlider->setValue (0);
    if (show) {
        m_posSlider->show ();
        m_buttonbox->setMargin (4);
        m_buttonbox->setSpacing (4);
        setEraseColor (m_view->topLevelWidget ()->paletteBackgroundColor ());
    } else {
        m_posSlider->hide ();
        m_buttonbox->setMargin (1);
        m_buttonbox->setSpacing (1);
        setEraseColor (QColor (0, 0, 0));
    }
    for (int i = 0; i < (int) button_last; i++) {
        m_buttons[i]->setMinimumSize (15, h-1);
        m_buttons[i]->setMaximumSize (750, h);
    }
    setMaximumSize (2500, h + (show ? 8 : 2 ));
    m_view->updateLayout ();
}

void ControlPanel::enableSeekButtons (bool enable) {
    if (!m_auto_controls) return;
    if (enable) {
        m_buttons[button_back]->show ();
        m_buttons[button_forward]->show ();
    } else {
        m_buttons[button_back]->hide ();
        m_buttons[button_forward]->hide ();
    }
}

void ControlPanel::enableRecordButtons (bool enable) {
    if (!m_auto_controls) return;
    if (enable)
        m_buttons[button_record]->show ();
    else
        m_buttons[button_record]->hide ();
}

KDE_NO_EXPORT void ControlPanel::setPlaying (bool play) {
    if (play != m_buttons[button_play]->isOn ())
        m_buttons[button_play]->toggle ();
    m_posSlider->setEnabled (false);
    m_posSlider->setValue (0);
    if (!play) {
        showPositionSlider (true);
        enableSeekButtons (true);
    }
}

KDE_NO_EXPORT void ControlPanel::setRecording (bool record) {
    if (record != m_buttons[button_record]->isOn ())
        m_buttons[button_record]->toggle ();
}

KDE_NO_EXPORT void ControlPanel::setPlayingProgress (int pos) {
    m_posSlider->setEnabled (false);
    if (m_progress_mode != progress_playing) {
        m_posSlider->setMaxValue (m_progress_length);
        m_progress_mode = progress_playing;
    }
    if (m_progress_length <= 0 && pos > 7 * m_posSlider->maxValue ()/8)
        m_posSlider->setMaxValue (m_posSlider->maxValue() * 2);
    else if (m_posSlider->maxValue() < pos)
        m_posSlider->setMaxValue (int (1.4 * m_posSlider->maxValue()));
    m_posSlider->setValue (pos);
    m_posSlider->setEnabled (true);
}

KDE_NO_EXPORT void ControlPanel::setLoadingProgress (int pos) {
    m_posSlider->setEnabled (false);
    if (m_progress_mode != progress_loading) {
        m_posSlider->setMaxValue (100);
        m_progress_mode = progress_loading;
    }
    m_posSlider->setValue (pos);
}

KDE_NO_EXPORT void ControlPanel::setPlayingLength (int len) {
    m_posSlider->setEnabled (false);
    m_progress_length = len;
    m_posSlider->setMaxValue (m_progress_length);
    m_progress_mode = progress_playing;
    m_posSlider->setEnabled (true);
}

//-----------------------------------------------------------------------------

namespace KMPlayer {

class KMPlayerPictureWidget : public QWidget {
    View * m_view;
public:
    KDE_NO_CDTOR_EXPORT KMPlayerPictureWidget (QWidget * parent, View * view)
        : QWidget (parent), m_view (view) {}
    KDE_NO_CDTOR_EXPORT ~KMPlayerPictureWidget () {}
protected:
    void mousePressEvent (QMouseEvent *);
};

} // namespace

KDE_NO_EXPORT void KMPlayerPictureWidget::mousePressEvent (QMouseEvent *) {
    m_view->emitPictureClicked ();
}

//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT KMPlayerPopupMenu::KMPlayerPopupMenu (QWidget * parent)
 : KPopupMenu (parent, "kde_kmplayer_popupmenu") {}

KDE_NO_EXPORT void KMPlayerPopupMenu::leaveEvent (QEvent *) {
    emit mouseLeft ();
}

//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT ListViewItem::ListViewItem (QListViewItem *p, const NodePtr & e, PlayListView * lv) : QListViewItem (p), m_elm (e), listview (lv) {}

KDE_NO_CDTOR_EXPORT ListViewItem::ListViewItem (QListViewItem *p, const AttributePtr & a, PlayListView * lv) : QListViewItem (p), m_attr (a), listview (lv) {}

KDE_NO_CDTOR_EXPORT ListViewItem::ListViewItem (PlayListView *v, const NodePtr & e) : QListViewItem (v), m_elm (e), listview (v) {}

KDE_NO_CDTOR_EXPORT PlayListView::PlayListView (QWidget * parent, View * view)
 : KListView (parent, "kde_kmplayer_playlist"),
   m_view (view),
   m_show_all_nodes (false),
   m_ignore_expanded (false) {
    addColumn (QString::null);
    header()->hide ();
    setTreeStepSize (15);
    setRootIsDecorated (true);
    setSorting (-1);
    setAcceptDrops (true);
    setDropVisualizer (true);
    m_itemmenu = new QPopupMenu (this);
    folder_pix = KGlobal::iconLoader ()->loadIcon (QString ("folder"), KIcon::Small);
    auxiliary_pix = KGlobal::iconLoader ()->loadIcon (QString ("folder_grey"), KIcon::Small);
    video_pix = KGlobal::iconLoader ()->loadIcon (QString ("video"), KIcon::Small);
    unknown_pix = KGlobal::iconLoader ()->loadIcon (QString ("unknown"), KIcon::Small);
    menu_pix = KGlobal::iconLoader ()->loadIcon (QString ("showmenu"), KIcon::Small);
    config_pix = KGlobal::iconLoader ()->loadIcon (QString ("configure"), KIcon::Small);
    m_itemmenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("editcopy"), KIcon::Small, 0, true), i18n ("&Copy to Clipboard"), this, SLOT (copyToClipboard ()), 0, 0);
    m_itemmenu->insertItem (KGlobal::iconLoader ()->loadIconSet (QString ("bookmark_add"), KIcon::Small, 0, true), i18n ("&Add Bookmark"), this, SLOT (addBookMark ()), 0, 1);
    m_itemmenu->insertItem (i18n ("&Show all"), this, SLOT (toggleShowAllNodes ()), 0, 2);
    connect (this, SIGNAL (contextMenuRequested (QListViewItem *, const QPoint &, int)), this, SLOT (contextMenuItem (QListViewItem *, const QPoint &, int)));
    connect (this, SIGNAL (expanded (QListViewItem *)),
             this, SLOT (itemExpanded (QListViewItem *)));
    connect (this, SIGNAL (dropped (QDropEvent *, QListViewItem *)),
             this, SLOT (itemDropped (QDropEvent *, QListViewItem *)));
    QFont fnt = font ();
    fnt.setPointSize (fnt.pointSize () - 1);
    fnt.setWeight (QFont::DemiBold);
    setFont (fnt);
}

KDE_NO_CDTOR_EXPORT PlayListView::~PlayListView () {
}

void PlayListView::populate (NodePtr e, NodePtr focus, QListViewItem * item, QListViewItem ** curitem) {
    Mrl * mrl = e->mrl ();
    QString text (e->nodeName());
    if (mrl && !m_show_all_nodes) {
        if (mrl->pretty_name.isEmpty ()) {
            if (!mrl->src.isEmpty())
                text = KURL(mrl->src).prettyURL();
            else if (e->isDocument ())
                text = e->hasChildNodes () ? i18n ("unnamed") : i18n ("empty");
        } else
            text = mrl->pretty_name;
    } else if (!strcmp (e->nodeName (), "#text"))
        text = e->nodeValue ();
    QPixmap & pix = e->isMrl() ? video_pix : (e->hasChildNodes ()) ? (e->auxiliaryNode () ? auxiliary_pix : folder_pix) : unknown_pix;
    item->setText(0, text);
    item->setPixmap (0, pix);
    if (focus == e)
        *curitem = item;
    for (NodePtr c = e->lastChild (); c; c = c->previousSibling ()) {
        m_have_dark_nodes |= !c->expose ();
        if (m_show_all_nodes || c->expose ())
            populate (c, focus, new ListViewItem (item, c, this), curitem);
    }
    if (m_show_all_nodes && e->isElementNode ()) {
        AttributePtr a = convertNode<Element> (e)->attributes ()->first ();
        if (a) {
            ListViewItem * as = new ListViewItem (item, e, this);
            as->setText (0, i18n ("[attributes]"));
            as->setPixmap (0, menu_pix);
            for (; a; a = a->nextSibling ()) {
                ListViewItem * ai = new ListViewItem (as, a, this);
                ai->setText (0, QString ("%1=%2").arg (a->nodeName ()).arg (a->nodeValue ()));
                ai->setPixmap (0, config_pix);
            }
        }
    }
}

void PlayListView::updateTree (NodePtr root, NodePtr active) {
    m_ignore_expanded = true;
    m_have_dark_nodes = false;
    clear ();
    if (!root) return;
    QListViewItem * curitem = 0L;
    populate (root, active, new ListViewItem (this, root), &curitem);
    if (curitem) {
        setSelected (curitem, true);
        ensureItemVisible (curitem);
    }
    m_itemmenu->setItemEnabled (2, m_have_dark_nodes);
    m_ignore_expanded = false;
}

void PlayListView::selectItem (const QString & txt) {
    QListViewItem * item = selectedItem ();
    if (item && item->text (0) == txt)
        return;
    item = findItem (txt, 0);
    if (item) {
        setSelected (item, true);
        ensureItemVisible (item);
    }
}

KDE_NO_EXPORT void PlayListView::contextMenuItem (QListViewItem * vi, const QPoint & p, int) {
    if (vi) {
        ListViewItem * item = static_cast <ListViewItem *> (vi);
        if (item->m_elm) {
            m_itemmenu->setItemEnabled (1, item->m_elm && (item->m_elm->isMrl () || item->m_elm->isDocument ()) && item->m_elm->mrl ()->bookmarkable);
            m_itemmenu->exec (p);
        }
    } else
        m_view->controlPanel ()->popupMenu ()->exec (p);
}

void PlayListView::itemExpanded (QListViewItem * item) {
    if (!m_ignore_expanded && item->childCount () == 1) {
        ListViewItem * child_item = static_cast<ListViewItem*>(item->firstChild ());
        child_item->setOpen (m_show_all_nodes || (child_item->m_elm && child_item->m_elm->expose ()));
    }
}

void PlayListView::copyToClipboard () {
    ListViewItem * item = static_cast <ListViewItem *> (currentItem ());
    QString text = item->text (0);
    if (item->m_elm) {
        Mrl * mrl = item->m_elm->mrl ();
        if (mrl)
            text = mrl->src;
    }
    QApplication::clipboard()->setText (text);
}

void PlayListView::addBookMark () {
    ListViewItem * item = static_cast <ListViewItem *> (currentItem ());
    if (item->m_elm) {
        Mrl * mrl = item->m_elm->mrl ();
        KURL url (mrl ? mrl->src : QString (item->m_elm->nodeName ()));
        emit addBookMark (mrl->pretty_name.isEmpty () ? url.prettyURL () : mrl->pretty_name, url.url ());
    }
}

void PlayListView::toggleShowAllNodes () {
    m_show_all_nodes = !m_show_all_nodes;
    m_itemmenu->setItemChecked (2, m_show_all_nodes);
    ListViewItem * first_item = static_cast <ListViewItem *> (firstChild ());
    if (first_item) {
        NodePtr root = first_item->m_elm;
        NodePtr cur;
        ListViewItem * cur_item = static_cast <ListViewItem *> (currentItem ());
        if (cur_item)
            cur = cur_item->m_elm;
        updateTree (root, cur);
    }
}

KDE_NO_EXPORT bool PlayListView::acceptDrag (QDropEvent * de) const {
    return isDragValid (de);
}

KDE_NO_EXPORT void PlayListView::itemDropped (QDropEvent * e, QListViewItem *) {
    m_view->dropEvent (e);
}

//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT Console::Console (QWidget * parent, View * view) : QTextEdit (parent, "kde_kmplayer_console"), m_view (view) {
    setTextFormat (Qt::PlainText);
    setReadOnly (true);
    QFont fnt = KGlobalSettings::fixedFont ();
    setFont (fnt);
}

KDE_NO_EXPORT void Console::contextMenuEvent (QContextMenuEvent * e) {
    m_view->controlPanel ()->popupMenu ()->exec (e->globalPos ());
}

//-----------------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT View::View (QWidget *parent, const char *name)
  : KMediaPlayer::View (parent, name),
    m_image (0L),
    m_control_panel (0L),
    m_volume_slider (0L),
    m_mixer_object ("kicker"),
    m_controlpanel_mode (CP_Show),
    m_old_controlpanel_mode (CP_Show),
    controlbar_timer (0),
    popup_timer (0),
    popdown_timer (0),
    m_keepsizeratio (false),
    m_playing (false),
    m_mixer_init (false),
    m_inVolumeUpdate (false),
    m_tmplog_needs_eol (false),
    m_revert_fullscreen (false),
    m_popup_clicked (false)
{
    setEraseColor (QColor (0, 0, 255));
}

KDE_NO_EXPORT void View::dropEvent (QDropEvent * de) {
    KURL::List sl;
    if (KURLDrag::canDecode (de)) {
        KURLDrag::decode (de, sl);
    } else if (QTextDrag::canDecode (de)) {
        QString text;
        QTextDrag::decode (de, text);
        sl.push_back (KURL (text));
    }
    if (sl.size () > 0) {
        m_widgetstack->visibleWidget ()->setFocus ();
        emit urlDropped (sl);
        de->accept ();
    }
}

KDE_NO_EXPORT void View::ctrlButtonMouseEntered () {
    if (!popup_timer && !m_control_panel->popupMenu ()->isVisible ()) {
        m_popup_clicked = false;
        popup_timer = startTimer (400);
    }
}

KDE_NO_EXPORT void View::ctrlButtonClicked () {
    if (popup_timer) {
        killTimer (popup_timer);
        popup_timer = 0;
    }
    m_popup_clicked = true;
    showPopupMenu ();
}

KDE_NO_EXPORT void View::popupMenuMouseLeft () {
    if (!popdown_timer && !m_popup_clicked)
        popdown_timer = startTimer (400);
}

KDE_NO_EXPORT void View::dragEnterEvent (QDragEnterEvent* dee) {
    if (isDragValid (dee))
        dee->accept ();
}

KDE_NO_EXPORT void View::init () {
    //setBackgroundMode(Qt::NoBackground);
    QPalette pal (QColor (64, 64,64), QColor (32, 32, 32));
    QVBoxLayout * viewbox = new QVBoxLayout (this, 0, 0);
    m_dockarea = new KDockArea (this, "kde_kmplayer_dock_area");
    m_dock_video = new KDockWidget (m_dockarea->manager (), 0, KGlobal::iconLoader ()->loadIcon (QString ("kmplayer"), KIcon::Small), m_dockarea);
    m_dock_video->setDockSite (KDockWidget::DockLeft | KDockWidget::DockBottom | KDockWidget::DockRight | KDockWidget::DockTop);
    m_dock_video->setEnableDocking(KDockWidget::DockNone);
    m_view_area = new ViewArea (m_dock_video, this);
    m_dock_video->setWidget (m_view_area);
    m_dockarea->setMainDockWidget (m_dock_video);
    m_dock_playlist = m_dockarea->createDockWidget (QString ("PlayList"), KGlobal::iconLoader ()->loadIcon (QString ("player_playlist"), KIcon::Small));
    m_playlist = new PlayListView (m_dock_playlist, this);
    m_playlist->setPaletteBackgroundColor (QColor (0, 0, 0));
    m_playlist->setPaletteForegroundColor (QColor (0xB2, 0xB2, 0xB2));
    m_dock_playlist->setWidget (m_playlist);
    viewbox->addWidget (m_dockarea);
    m_widgetstack = new QWidgetStack (m_view_area);
    m_control_panel = new ControlPanel (m_view_area, this);
    m_control_panel->setMaximumSize (2500, controlPanel ()->maximumSize ().height ());
    m_viewer = new Viewer (m_widgetstack, this);
    m_widgettypes [WT_Video] = m_viewer;
#if KDE_IS_VERSION(3,1,90)
    setVideoWidget (m_view_area);
#endif

    m_multiedit = new Console (m_widgetstack, this);
    m_multiedit->setPaper (QBrush (QColor (0, 0, 0)));
    m_multiedit->setColor (QColor (0xB2, 0xB2, 0xB2));
    m_widgettypes[WT_Console] = m_multiedit;
    m_widgettypes[WT_Picture] = new KMPlayerPictureWidget (m_widgetstack, this);

    m_widgetstack->addWidget (m_viewer);
    m_widgetstack->addWidget (m_multiedit);
    m_widgetstack->addWidget (m_widgettypes[WT_Picture]);

    setFocusPolicy (QWidget::ClickFocus);

    connect (m_control_panel->button (ControlPanel::button_config), SIGNAL (clicked ()), this, SLOT (ctrlButtonClicked ()));
    connect (m_control_panel->button (ControlPanel::button_config), SIGNAL (mouseEntered ()), this, SLOT (ctrlButtonMouseEntered ()));
    connect (m_control_panel->popupMenu(), SIGNAL (mouseLeft ()), this, SLOT (popupMenuMouseLeft ()));
    connect (m_control_panel->playerMenu(), SIGNAL (mouseLeft ()), this, SLOT (popupMenuMouseLeft ()));
    connect (m_control_panel->zoomMenu(), SIGNAL (mouseLeft ()), this, SLOT (popupMenuMouseLeft ()));
    connect (m_control_panel->viewMenu(), SIGNAL (mouseLeft ()), this, SLOT (popupMenuMouseLeft ()));
    connect (m_control_panel->colorMenu(), SIGNAL (mouseLeft ()), this, SLOT (popupMenuMouseLeft ()));
    setAcceptDrops (true);
    m_view_area->resizeEvent (0L);
    kdDebug() << "View " << (unsigned long) (m_viewer->embeddedWinId()) << endl;

    XSelectInput (qt_xdisplay (), m_viewer->embeddedWinId (), 
               //KeyPressMask | KeyReleaseMask |
               KeyPressMask |
               //EnterWindowMask | LeaveWindowMask |
               //FocusChangeMask |
               ExposureMask |
               StructureNotifyMask |
               PointerMotionMask
              );
    kapp->installX11EventFilter (this);
}

KDE_NO_CDTOR_EXPORT View::~View () {
    delete m_image;
    if (m_view_area->parent () != this)
        delete m_view_area;
}

void View::showPlaylist () {
    bool horz = true;
    QStyle & style = m_playlist->style ();
    int h = style.pixelMetric (QStyle::PM_ScrollBarExtent, m_playlist);
    h += style.pixelMetric (QStyle::PM_DockWindowFrameWidth, m_playlist);
    h += style.pixelMetric (QStyle::PM_DockWindowHandleExtent, m_playlist);
    for (QListViewItem * i = m_playlist->firstChild (); i; i = i->itemBelow()) {
        h += i->height ();
        if (h > int (0.5 * height ())) {
            horz = false;
            break;
        }
    }
    int perc = 30;
    if (horz && 100 * h / height () < perc)
        perc = 100 * h / height ();
    m_dock_playlist->manualDock (m_dock_video, horz ? KDockWidget::DockTop : KDockWidget::DockLeft, perc);
}

bool View::setPicture (const QString & path) {
    delete m_image;
    if (path.isEmpty ())
        m_image = 0L;
    else {
        m_image = new QPixmap (path);
        if (m_image->isNull ()) {
            delete m_image;
            m_image = 0L;
            kdDebug() << "View::setPicture failed " << path << endl;
        }
    }
    if (!m_image) {
        m_widgetstack->raiseWidget (m_viewer);
    } else {
        m_widgettypes[WT_Picture]->setPaletteBackgroundPixmap (*m_image);
        m_widgetstack->raiseWidget (m_widgettypes[WT_Picture]);
    }
    return m_image;
}

KDE_NO_EXPORT void View::updateVolume () {
    if (m_mixer_init && !m_volume_slider)
        return;
    QByteArray data, replydata;
    QCString replyType;
    int volume;
    bool has_mixer = kapp->dcopClient ()->call (m_mixer_object, "Mixer0",
            "masterVolume()", data, replyType, replydata);
    if (!has_mixer) {
        m_mixer_object = "kmix";
        has_mixer = kapp->dcopClient ()->call (m_mixer_object, "Mixer0",
                "masterVolume()", data, replyType, replydata);
    }
    if (has_mixer) {
        QDataStream replystream (replydata, IO_ReadOnly);
        replystream >> volume;
        if (!m_mixer_init) {
            m_mixer_label = new QLabel (i18n ("Volume:"), m_control_panel->popupMenu ());
            m_control_panel->popupMenu ()->insertItem (m_mixer_label, -1, 4);
            m_volume_slider = new QSlider (0, 100, 10, volume, Qt::Horizontal, m_control_panel->popupMenu ());
            connect(m_volume_slider, SIGNAL(valueChanged(int)), this,SLOT(setVolume(int)));
            m_control_panel->popupMenu ()->insertItem (m_volume_slider, ControlPanel::menu_volume, 5);
            m_control_panel->popupMenu ()->insertSeparator (6);
        } else {
            m_inVolumeUpdate = true;
            m_volume_slider->setValue (volume);
            m_inVolumeUpdate = false;
        }
    } else if (m_volume_slider) {
        m_control_panel->popupMenu ()->removeItemAt (6);
        m_control_panel->popupMenu ()->removeItemAt (5);
        m_control_panel->popupMenu ()->removeItemAt (4);
        m_volume_slider = 0L;
    }
    m_mixer_init = true;
}

void View::showWidget (WidgetType wt) {
    m_widgetstack->raiseWidget (m_widgettypes [wt]);
    if (m_widgetstack->visibleWidget () == m_widgettypes[WT_Console])
        addText (QString (""), false);
    updateLayout ();
}

void View::setControlPanelMode (ControlPanelMode m) {
    killTimer (controlbar_timer);
    m_old_controlpanel_mode = m_controlpanel_mode = m;
    if (m_playing && isFullScreen())
        m_controlpanel_mode = CP_AutoHide;
    if (m_control_panel)
        if (m_controlpanel_mode == CP_Show || m_controlpanel_mode == CP_Only)
            m_control_panel->show ();
        else if (m_controlpanel_mode == CP_AutoHide) { 
            if (m_playing)
                delayedShowButtons (false);
            else
                m_control_panel->show ();
        } else
            m_control_panel->hide ();
    //m_view_area->setMouseTracking (m_controlpanel_mode == CP_AutoHide && m_playing);
    m_view_area->resizeEvent (0L);
}

KDE_NO_EXPORT void View::delayedShowButtons (bool show) {
    if (m_controlpanel_mode != CP_AutoHide || controlbar_timer ||
        (m_control_panel &&
         (show && m_control_panel->isVisible ()) || 
         (!show && !m_control_panel->isVisible ())))
        return;
    controlbar_timer = startTimer (500);
}

KDE_NO_EXPORT void View::setVolume (int vol) {
    if (m_inVolumeUpdate) return;
    QByteArray data;
    QDataStream arg( data, IO_WriteOnly );
    arg << vol;
    if (!kapp->dcopClient()->send (m_mixer_object, "Mixer0", "setMasterVolume(int)", data))
        kdWarning() << "Failed to update volume" << endl;
}

KDE_NO_EXPORT void  View::updateLayout () {
    if (m_controlpanel_mode == CP_Only)
        m_control_panel->setMaximumSize (2500, height ());
    m_view_area->resizeEvent (0L);
}

void View::setKeepSizeRatio (bool b) {
    if (m_keepsizeratio != b) {
        m_keepsizeratio = b;
        updateLayout ();
        m_view_area->update ();
    }
}

KDE_NO_EXPORT void View::timerEvent (QTimerEvent * e) {
    if (e->timerId () == controlbar_timer) {
        controlbar_timer = 0;
        if (!m_playing)
            return;
        int vert_buttons_pos = m_view_area->height ();
        int mouse_pos = m_view_area->mapFromGlobal (QCursor::pos ()).y();
        int cp_height = m_control_panel->maximumSize ().height ();
        bool mouse_on_buttons = (//m_view_area->hasMouse () && 
                mouse_pos >= vert_buttons_pos-cp_height &&
                mouse_pos <= vert_buttons_pos);
        if (m_control_panel)
            if (mouse_on_buttons && !m_control_panel->isVisible ())
                m_control_panel->show ();
            else if (!mouse_on_buttons && m_control_panel->isVisible ())
                m_control_panel->hide ();
    } else if (e->timerId () == popup_timer) {
        popup_timer = 0;
        if (m_control_panel->button (ControlPanel::button_config)->hasMouse () && !m_control_panel->popupMenu ()->isVisible ())
            showPopupMenu ();
    } else if (e->timerId () == popdown_timer) {
        popdown_timer = 0;
        if (m_control_panel->popupMenu ()->isVisible () && !m_control_panel->popupMenu ()->hasMouse () && !m_control_panel->playerMenu ()->hasMouse () && !m_control_panel->viewMenu ()->hasMouse () && !m_control_panel->zoomMenu ()->hasMouse () && !m_control_panel->colorMenu ()->hasMouse () && !m_control_panel->bookmarkMenu ()->hasMouse ())
            if (!(m_control_panel->bookmarkMenu ()->isVisible () && static_cast <QWidget *> (m_control_panel->bookmarkMenu ()) != QWidget::keyboardGrabber ()))
                // not if user entered the bookmark sub menu or if I forgot one
                m_control_panel->popupMenu ()->hide ();
    }
    killTimer (e->timerId ());
    m_view_area->resizeEvent (0L);
}

void View::addText (const QString & str, bool eol) {
    if (m_tmplog_needs_eol)
        tmplog += QChar ('\n');
    tmplog += str;
    m_tmplog_needs_eol = eol;
    if (m_widgetstack->visibleWidget () != m_widgettypes[WT_Console] &&
            tmplog.length () < 7500)
        return;
    if (eol) {
        m_multiedit->append (tmplog);
        tmplog.truncate (0);
        m_tmplog_needs_eol = false;
    } else {
        int pos = tmplog.findRev (QChar ('\n'));
        if (pos >= 0) {
            m_multiedit->append (tmplog.left (pos));
            tmplog = tmplog.mid (pos+1);
        }
    }
    int p = m_multiedit->paragraphs ();
    if (5000 < p) {
        m_multiedit->setSelection (0, 0, p - 4499, 0);
        m_multiedit->removeSelectedText ();
    }
    m_multiedit->setCursorPosition (m_multiedit->paragraphs () - 1, 0);
}

/* void View::print (QPrinter *pPrinter)
{
    QPainter printpainter;
    printpainter.begin (pPrinter);

    // TODO: add your printing code here

    printpainter.end ();
}*/

KDE_NO_EXPORT void View::videoStart () {
    if (m_playing) return; //FIXME: make symetric with videoStop
    if (m_widgetstack->visibleWidget () == m_widgettypes[WT_Picture])
        m_widgetstack->raiseWidget (m_viewer);
    m_playing = true;
    m_revert_fullscreen = !isFullScreen();
    setControlPanelMode (m_old_controlpanel_mode);
}

KDE_NO_EXPORT void View::videoStop () {
    if (m_control_panel && m_controlpanel_mode == CP_AutoHide) {
        m_control_panel->show ();
        //m_view_area->setMouseTracking (false);
    }
    m_playing = false;
    XClearWindow (qt_xdisplay(), m_viewer->embeddedWinId ());
    m_view_area->resizeEvent (0L);
}

KDE_NO_EXPORT void View::showPopupMenu () {
    updateVolume ();
    int cp_height = m_control_panel->maximumSize ().height ();
    m_control_panel->popupMenu ()->exec (m_control_panel->button (ControlPanel::button_config)->mapToGlobal (QPoint (0, cp_height)));
}

KDE_NO_EXPORT void View::leaveEvent (QEvent *) {
    if (m_controlpanel_mode == CP_AutoHide && m_playing)
        delayedShowButtons (false);
}

KDE_NO_EXPORT void View::reset () {
    if (m_revert_fullscreen && isFullScreen())
        m_control_panel->popupMenu ()->activateItemAt (m_control_panel->popupMenu ()->indexOf (ControlPanel::menu_fullscreen)); 
        //m_view_area->fullScreen ();
    videoStop ();
    m_viewer->show ();
}

bool View::isFullScreen () const {
    return m_view_area->isFullScreen ();
}

void View::fullScreen () {
    if (!m_view_area->isFullScreen()) {
        m_sreensaver_disabled = false;
        QByteArray data, replydata;
        QCString replyType;
        if (kapp->dcopClient ()->call ("kdesktop", "KScreensaverIface",
                    "isEnabled()", data, replyType, replydata)) {
            bool enabled;
            QDataStream replystream (replydata, IO_ReadOnly);
            replystream >> enabled;
            if (enabled)
                m_sreensaver_disabled = kapp->dcopClient()->send
                    ("kdesktop", "KScreensaverIface", "enable(bool)", "false");
        }
        //if (m_keepsizeratio && m_viewer->aspect () < 0.01)
        //    m_viewer->setAspect (1.0 * m_viewer->width() / m_viewer->height());
        m_view_area->fullScreen();
        m_control_panel->popupMenu ()->setItemVisible (ControlPanel::menu_zoom, false);
        m_widgetstack->visibleWidget ()->setFocus ();
    } else {
        if (m_sreensaver_disabled)
            m_sreensaver_disabled = !kapp->dcopClient()->send
                ("kdesktop", "KScreensaverIface", "enable(bool)", "true");
        m_view_area->fullScreen();
        m_control_panel->popupMenu ()->setItemVisible (ControlPanel::menu_zoom, true);
    }
    setControlPanelMode (m_old_controlpanel_mode);
    emit fullScreenChanged ();
}

KDE_NO_EXPORT bool View::x11Event (XEvent * e) {
    switch (e->type) {
        case UnmapNotify:
            if (e->xunmap.event == m_viewer->embeddedWinId ()) {
                videoStart ();
                //hide();
            }
            break;
        case XKeyPress:
            if (e->xkey.window == m_viewer->embeddedWinId ()) {
                KeySym ksym;
                char kbuf[16];
                XLookupString (&e->xkey, kbuf, sizeof(kbuf), &ksym, NULL);
                switch (ksym) {
                    case XK_f:
                    case XK_F:
                        //fullScreen ();
                        break;
                };
            }
            break;
        /*case ColormapNotify:
            fprintf (stderr, "colormap notify\n");
            return true;*/
        case MotionNotify:
            if (m_playing && e->xmotion.window == m_viewer->embeddedWinId ())
                delayedShowButtons (e->xmotion.y > m_viewer->height () -
                                    m_control_panel->maximumSize ().height ());
            m_view_area->mouseMoved ();
            break;
        case MapNotify:
            if (e->xmap.event == m_viewer->embeddedWinId ()) {
                show ();
                QTimer::singleShot (10, m_viewer, SLOT (sendConfigureEvent ()));
            }
            break;
        /*case ConfigureNotify:
            break;
            //return true;*/
        default:
            break;
    }
    return false;
}

//----------------------------------------------------------------------

KDE_NO_CDTOR_EXPORT Viewer::Viewer (QWidget *parent, View * view)
  : QXEmbed (parent), m_bgcolor (0), m_aspect (0.0),
    m_view (view) {
    /*XWindowAttributes xwa;
    XGetWindowAttributes (qt_xdisplay(), winId (), &xwa);
    XSetWindowAttributes xswa;
    xswa.background_pixel = 0;
    xswa.border_pixel = 0;
    xswa.colormap = xwa.colormap;
    create (XCreateWindow (qt_xdisplay (), parent->winId (), 0, 0, 10, 10, 0, 
                           x11Depth (), InputOutput, (Visual*)x11Visual (),
                           CWBackPixel | CWBorderPixel | CWColormap, &xswa));*/
    setAcceptDrops (true);
#if KDE_IS_VERSION(3,1,1)
    setProtocol(QXEmbed::XPLAIN);
#endif
    embed (XCreateSimpleWindow (qt_xdisplay(), view->winId (), 0, 0, width(), height(), 1, 0, 0));
    XClearWindow (qt_xdisplay(), embeddedWinId ());
}

KDE_NO_CDTOR_EXPORT Viewer::~Viewer () {
}
    
KDE_NO_EXPORT void Viewer::mouseMoveEvent (QMouseEvent * e) {
    if (e->state () == Qt::NoButton) {
        int cp_height = m_view->controlPanel ()->maximumSize ().height ();
        m_view->delayedShowButtons (e->y () > height () - cp_height);
    }
    m_view->m_view_area->mouseMoved ();
}

void Viewer::setAspect (float a) {
    float da = m_aspect - a;
    if (da < 0) da *= -1;
    if (da < 0.0001)
        return;
    m_aspect = a;
}

KDE_NO_EXPORT int Viewer::heightForWidth (int w) const {
    if (m_aspect <= 0.01)
        return 0;
    return int (w/m_aspect); 
}

KDE_NO_EXPORT void Viewer::dropEvent (QDropEvent * de) {
    m_view->dropEvent (de);
}

KDE_NO_EXPORT void Viewer::dragEnterEvent (QDragEnterEvent* dee) {
    m_view->dragEnterEvent (dee);
}
/*
*/
void Viewer::sendKeyEvent (int key) {
    char buf[2] = { char (key), '\0' }; 
    KeySym keysym = XStringToKeysym (buf);
    XKeyEvent event = {
        XKeyPress, 0, true,
        qt_xdisplay (), embeddedWinId (), qt_xrootwin(), embeddedWinId (),
        /*time*/ 0, 0, 0, 0, 0,
        0, XKeysymToKeycode (qt_xdisplay (), keysym), true
    };
    XSendEvent (qt_xdisplay(), embeddedWinId (), FALSE, KeyPressMask, (XEvent *) &event);
    XFlush (qt_xdisplay ());
}

KDE_NO_EXPORT void Viewer::sendConfigureEvent () {
    XConfigureEvent c = {
        ConfigureNotify, 0UL, True,
        qt_xdisplay (), embeddedWinId (), winId (),
        x (), y (), width (), height (),
        0, None, False
    };
    XSendEvent(qt_xdisplay(), c.event, TRUE, StructureNotifyMask, (XEvent*) &c);
    XFlush (qt_xdisplay ());
}

KDE_NO_EXPORT void Viewer::contextMenuEvent (QContextMenuEvent * e) {
    m_view->controlPanel ()->popupMenu ()->exec (e->globalPos ());
}

KDE_NO_EXPORT void Viewer::setBackgroundColor (const QColor & c) {
    if (m_bgcolor != c.rgb ()) {
        m_bgcolor = c.rgb ();
        XSetWindowBackground (qt_xdisplay (), embeddedWinId (), m_bgcolor);
        XFlush (qt_xdisplay ());
    }
}

#include "kmplayerview.moc"
