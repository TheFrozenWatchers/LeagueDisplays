
#ifndef LD_UIBASE_H
#define LD_UIBASE_H

#pragma once

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <string>
#include <vector>

#include "xscreensaver.h"

#ifdef LD_USE_APPINDICATOR
#include <libappindicator/app-indicator.h>
#endif // LD_USE_APPINDICATOR

#define LD_ICON_PATH "icon.png"
#define LD_APPNAME   "League Displays"

namespace LeagueDisplays {
    namespace UI {
        void _on_sig_destroy(int sig);
        int XIOErrorHandlerImpl(Display *display);
        int XErrorHandlerImpl(Display *display, XErrorEvent *event);

        class UIManager {
            public:
                static void Create();
                static void CreateAgent();
                static void CloseGracefully();
                static bool SaveFileDialog(const std::string& filename, const std::string& current_name, std::string& res);
                static void ShowDownloadFinishedDialog();
                
                static int                          mWindowWidth;
                static int                          mWindowHeight;

                static GtkWidget*                   mMainWindow;
                static GtkWidget*                   mVbox;
                static GdkGeometry                  mAppGeomentry;

#ifdef LD_USE_APPINDICATOR
                static AppIndicator*                mStatusIcon;
#else
                static GtkStatusIcon*               mStatusIcon;
#endif // LD_USE_APPINDICATOR

                static Window                       mX11WindowID;
                static Display*                     mX11Display;
                static Screen*                      mX11Screen;
                static GtkMenu*                     mStatusIconMenu;
                static bool                         mIsAllowedToExit;
                static bool                         mIsScreensaver;
                static bool                         mAllowMultipleInstances;
                static unsigned short               mHostPID;
                static CefRefPtr<XScreensaver>      mScreensaverHandler;
        };
    }
}

#endif // LD_UIBASE_H
