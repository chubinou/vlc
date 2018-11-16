/*****************************************************************************
 * main_interface.hpp : Main Interface
 ****************************************************************************
 * Copyright (C) 2006-2010 VideoLAN and AUTHORS
 * $Id$
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef QVLC_MAIN_INTERFACE_H_
#define QVLC_MAIN_INTERFACE_H_

#include "qt.hpp"

#include "util/qvlcframe.hpp"
#include "input_manager.hpp"

#include <QSystemTrayIcon>
#include <QStackedWidget>
#include <QQuickView>
#include <QQuickWidget>

#ifdef _WIN32
# include <shobjidl.h>
#endif

class QSettings;
class QCloseEvent;
class QKeyEvent;
class QLabel;
class QEvent;
class VideoWidget;
class VisualSelector;
class QVBoxLayout;
class QMenu;
class QSize;
class QScreen;
class QTimer;
class StandardPLPanel;
struct vout_window_t;

class MainInterface : public QVLCMW
{
    Q_OBJECT

public:
    /* tors */
    MainInterface( intf_thread_t *);
    virtual ~MainInterface();

    static const QEvent::Type ToolbarsNeedRebuild;

    /* Video requests from core */
    bool getVideo( struct vout_window_t *,
                   unsigned int i_width, unsigned int i_height, bool );
    void releaseVideo( void );
    int  controlVideo( int i_query, va_list args );

    /* Getters */
    QSystemTrayIcon *getSysTray() { return sysTray; }
    QMenu *getSysTrayMenu() { return systrayMenu; }
    enum
    {
        CONTROLS_VISIBLE  = 0x1,
        CONTROLS_HIDDEN   = 0x2,
        CONTROLS_ADVANCED = 0x4,
    };
    enum
    {
        RAISE_NEVER,
        RAISE_VIDEO,
        RAISE_AUDIO,
        RAISE_AUDIOVIDEO,
    };
    bool isInterfaceFullScreen() { return b_interfaceFullScreen; }
    bool isInterfaceAlwaysOnTop() { return b_interfaceOnTop; }

protected:
    void dropEventPlay( QDropEvent* event, bool b_play );
    void changeEvent( QEvent * ) Q_DECL_OVERRIDE;
    void dropEvent( QDropEvent *) Q_DECL_OVERRIDE;
    void dragEnterEvent( QDragEnterEvent * ) Q_DECL_OVERRIDE;
    void dragMoveEvent( QDragMoveEvent * ) Q_DECL_OVERRIDE;
    void dragLeaveEvent( QDragLeaveEvent * ) Q_DECL_OVERRIDE;
    void closeEvent( QCloseEvent *) Q_DECL_OVERRIDE;
    void keyPressEvent( QKeyEvent *) Q_DECL_OVERRIDE;
    void wheelEvent( QWheelEvent * ) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;
    virtual void toggleUpdateSystrayMenuWhenVisible();
    void resizeWindow(int width, int height);

protected:
    /* Main Widgets Creation */
    void createMainWidget( QSettings* );

    /* Systray */
    void createSystray();
    void initSystray();
    void handleSystray();

    /* */
    void setInterfaceFullScreen( bool );
    void computeMinimumSize();

    /* */
    QSettings           *settings;
    QSystemTrayIcon     *sysTray;
    QMenu               *systrayMenu;

    QString              input_name;
    QVBoxLayout         *mainLayout;

    VideoWidget         *videoWidget;
    QQuickWidget        *mediacenterView;
    QWidget             *mediacenterWrapper;
    QQuickWidget        *toolbarView;


    /* Status Bar */
    QLabel              *nameLabel;
    QLabel              *cryptedLabel;

    /* Status and flags */
    QWidget             *stackCentralOldWidget;
    QPoint              lastWinPosition;
    QSize               lastWinSize;  /// To restore the same window size when leaving fullscreen
    QScreen             *lastWinScreen;

    QSize               pendingResize; // to be applied when fullscreen is disabled

    QMap<QWidget *, QSize> stackWidgetsSizes;

    /* Flags */
    unsigned             i_notificationSetting; /// Systray Notifications
    bool                 b_autoresize;          ///< persistent resizable window
    bool                 b_videoFullScreen;     ///< --fullscreen
    bool                 b_hideAfterCreation;
    bool                 b_minimalView;         ///< Minimal video
    bool                 b_interfaceFullScreen;
    bool                 b_interfaceOnTop;      ///keep UI on top
    bool                 b_pauseOnMinimize;
    bool                 b_maximizedView;
    bool                 b_isWindowTiled;
#ifdef QT5_HAS_WAYLAND
    bool                 b_hasWayland;
#endif
    /* States */
    bool                 playlistVisible;       ///< Is the playlist visible ?
//    bool                 videoIsActive;       ///< Having a video now / THEMIM->hasV
//    bool                 b_visualSelectorEnabled;
    bool                 b_plDocked;            ///< Is the playlist docked ?

    bool                 b_hasPausedWhenMinimized;
    bool                 b_statusbarVisible;

    static const Qt::Key kc[10]; /* easter eggs */
    int i_kc_offset;

public slots:
    void toggleUpdateSystrayMenu();
    void showUpdateSystrayMenu();
    void hideUpdateSystrayMenu();
    void toggleInterfaceFullScreen();
    void setInterfaceAlwaysOnTop( bool );

    /* Manage the Video Functions from the vout threads */
    void getVideoSlot( struct vout_window_t *,
                       unsigned i_width, unsigned i_height, bool, bool * );
    void releaseVideoSlot( void );

    void emitBoss();
    void emitRaise();

    virtual void reloadPrefs();
    void toolBarConfUpdated();

protected slots:
    void debug();
    void setVLCWindowsTitle( const QString& title = "" );
    void handleSystrayClick( QSystemTrayIcon::ActivationReason );
    void updateSystrayTooltipName( const QString& );
    void updateSystrayTooltipStatus( InputManager::PlayingState );

    void handleKeyPress( QKeyEvent * );

    void showBuffering( float );

    void setVideoSize( unsigned int, unsigned int );
    void videoSizeChanged( int, int );
    void setVideoOnTop( bool );
    void setBoss();
    void setRaise();
    void voutReleaseMouseEvents();

    void onInputChanged( bool );

    void onToolbarVisibilityChanged( bool );

signals:
    void askGetVideo( struct vout_window_t *, unsigned, unsigned, bool,
                      bool * );
    void askReleaseVideo( );
    void askVideoToResize( unsigned int, unsigned int );
    void askVideoSetFullScreen( bool );
    void askVideoOnTop( bool );
    void minimalViewToggled( bool );
    void fullscreenInterfaceToggled( bool );
    void askToQuit();
    void askBoss();
    void askRaise();
    void kc_pressed(); /* easter eggs */
};

#endif
