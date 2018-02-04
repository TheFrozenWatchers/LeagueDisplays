
#include "uibase.h"
#include "log.h"
#include "background_daemon.h"
#include "filesystem.h"
#include "crossde.h"

#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"

#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "UIManager"

namespace LeagueDisplays {
    namespace UI {
        typedef struct {
            unsigned long   flags;
            unsigned long   functions;
            unsigned long   decorations;
            long            inputMode;
            unsigned long   status;
        } Hints; // X11 window hints

        /*static*/ int UIManager::mWindowWidth = 640;
        /*static*/ int UIManager::mWindowHeight = 480;

        /*static*/ GtkWidget* UIManager::mMainWindow = NULL;
        /*static*/ GtkWidget* UIManager::mVbox = NULL;
        /*static*/ GdkGeometry UIManager::mAppGeomentry = {0};

#ifdef LD_USE_APPINDICATOR
        /*static*/ AppIndicator* UIManager::mStatusIcon = NULL;
#else
        /*static*/ GtkStatusIcon* UIManager::mStatusIcon = NULL;
#endif

        /*static*/ GtkMenu* UIManager::mStatusIconMenu = NULL;

        /*static*/ Window UIManager::mX11WindowID = NULL;
        /*static*/ Display* UIManager::mX11Display = NULL;
        /*static*/ Screen* UIManager::mX11Screen = NULL;

        /*static*/ CefRefPtr<XScreensaver> UIManager::mScreensaverHandler = NULL;

        /*static*/ bool UIManager::mIsAllowedToExit = false;
        /*static*/ bool UIManager::mIsScreensaver = false;
        /*static*/ bool UIManager::mAllowMultipleInstances = false;

        /*static*/ unsigned short UIManager::mHostPID = 0xFFFF;

        int XErrorHandlerImpl(Display* display, XErrorEvent* event) {
            ERROR("X11 error: type=%d, serial=%lu, code=%d",
                event->type, event->serial, (int)event->error_code);
            return 0;
        }

        int XIOErrorHandlerImpl(Display* display) {
            return 0;
        }

        void agent_action(gchar* data) {
            if (!strcmp(data, "sigquit")) {
                unsigned short pid = LeagueDisplays::Filesystem::PIDFileGet("cefapp.pid");

                if (pid && !kill(pid, 0)) {
                    kill(pid, SIGTERM);
                    sleep(2);
                    kill(pid, SIGKILL);
                }

                exit(0);
            } else if (!strcmp(data, "sigopen")) {
                unsigned short pid = LeagueDisplays::Filesystem::PIDFileGet("cefapp.pid");
            
                if (!pid || kill(pid, 0)) {
                    std::string cmd = "\"" + LeagueDisplays::Filesystem::mExecutablePath + "\" --ld-mode=cef-app --ld-fork";

                    int r = system(cmd.c_str());

                    if (r < 0) {
                        ERROR("Could not start CEF process!");
                    }
                }
            }
        }

        void _on_sig_destroy(int sig) {
            if (getpid() != UIManager::mHostPID) {
                DEBUG("We are not the host, transfering signal to %d", UIManager::mHostPID);
                kill(UIManager::mHostPID, SIGTERM);
                sleep(2);
                kill(UIManager::mHostPID, SIGKILL);
                return;
            }

            // To overwrite the ugly ^C in terminal emulators
            usleep(200 * 1000);
            printf("\r");
            fflush(stdout);

            DEBUG("Terminating via signal: %d", sig);
            UIManager::CloseGracefully();
        }

        gint window_delete_signal(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
            UIManager::CloseGracefully();
            return FALSE;
        }

        gint tray_icon_on_click(GtkStatusIcon *status_icon, GdkEventButton *event, gpointer user_data) {
            if (event->type == GDK_2BUTTON_PRESS) {
                agent_action("sigopen");
                return TRUE;
            }
            
            return FALSE;
        }

        gint tray_icon_on_menu(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data) {
            gtk_menu_popup(UIManager::mStatusIconMenu, NULL, NULL, NULL, user_data, button, activate_time);

            return FALSE;
        }

        void sig_open_cef(int sig) {
            agent_action("sigopen");
        }

        /*static*/ void UIManager::Create() {
            INFO("Starting app...");

            gtk_init(0, NULL);
            signal(SIGINT, _on_sig_destroy);
            signal(SIGTERM, _on_sig_destroy);
            signal(SIGUSR1, sig_open_cef);
            
            mX11Display = XOpenDisplay(NULL);
            mX11Screen = DefaultScreenOfDisplay(mX11Display);

            INFO("Max resolution is $$6%d$$rx$$6%d", mX11Screen->width, mX11Screen->height);

            double height = mX11Screen->height / 1.25; 
            double width = height * (16.0 / 9.0);

            if (width > mX11Screen->width) {
                width = height * (4.0 / 3.0);
            }

            mWindowWidth  = floor(width);
            mWindowHeight = floor(height);

            INFO("Window target size is $$6%d$$rx$$6%d", mWindowWidth, mWindowHeight);

            // Create window
            mMainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
            g_signal_connect(G_OBJECT(mMainWindow), "delete_event",
                G_CALLBACK(window_delete_signal), NULL);

            gtk_window_set_default_size(GTK_WINDOW(mMainWindow), mWindowWidth, mWindowHeight);
            gtk_window_set_position    (GTK_WINDOW(mMainWindow), GTK_WIN_POS_CENTER         );
            gtk_window_set_title       (GTK_WINDOW(mMainWindow), LD_APPNAME                 );

            // Set icon
            gtk_window_set_icon(GTK_WINDOW(mMainWindow), gdk_pixbuf_new_from_file(LD_ICON_PATH, NULL));

            // Set geometry to prevent resizing
            mAppGeomentry.min_width = mWindowWidth;
            mAppGeomentry.max_width = mWindowWidth;
            mAppGeomentry.min_height = mWindowHeight;
            mAppGeomentry.max_height = mWindowHeight;

            gtk_window_set_geometry_hints(
                GTK_WINDOW(mMainWindow),
                mMainWindow,
                &mAppGeomentry,
                (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));

            // Create vbox for CEF (it requires a container)
            mVbox = gtk_vbox_new(0, 0);
            gtk_container_add(GTK_CONTAINER(mMainWindow), mVbox);

            // Show window
            gtk_widget_show_all(mMainWindow);

            INFO("Main window is ready");

            // Add X11 error handlers
            XSetErrorHandler(XErrorHandlerImpl);
            XSetIOErrorHandler(XIOErrorHandlerImpl);

            mX11WindowID = gdk_x11_drawable_get_xid(gtk_widget_get_window(mVbox));
        }

        /*static*/ void UIManager::CreateAgent() {
            signal(SIGUSR1, sig_open_cef);

            // Create context menu
            GtkWidget* widget = gtk_menu_new();
            GtkWidget* menu_items;

            // The "widget" is the menu that was supplied when 
            // g_signal_connect_swapped() was called.
            mStatusIconMenu = GTK_MENU(widget);

            menu_items = gtk_menu_item_new_with_label("LeagueDisplays");
            gtk_widget_set_sensitive(menu_items, FALSE);
            gtk_menu_shell_append (GTK_MENU_SHELL(mStatusIconMenu), menu_items);
            gtk_widget_show(menu_items);

            menu_items = gtk_separator_menu_item_new();
            gtk_menu_shell_append (GTK_MENU_SHELL(mStatusIconMenu), menu_items);
            gtk_widget_show(menu_items);

            menu_items = gtk_menu_item_new_with_label("Open configurator");
            gtk_menu_shell_append(GTK_MENU_SHELL(mStatusIconMenu), menu_items);
            g_signal_connect_swapped(menu_items, "activate", G_CALLBACK(agent_action), (gpointer) g_strdup("sigopen"));
            gtk_widget_show(menu_items);

            menu_items = gtk_menu_item_new_with_label("Quit");
            gtk_menu_shell_append (GTK_MENU_SHELL(mStatusIconMenu), menu_items);
            g_signal_connect_swapped(menu_items, "activate", G_CALLBACK(agent_action), (gpointer) g_strdup("sigquit"));
            gtk_widget_show(menu_items);

            // Create status icon
#ifdef LD_USE_APPINDICATOR
            INFO("This build uses AppIndicator");
            mStatusIcon = app_indicator_new(LD_APPNAME, (Filesystem::mWorkingDirectory + LD_ICON_PATH).c_str(),
                APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
 
            app_indicator_set_status(mStatusIcon, APP_INDICATOR_STATUS_ACTIVE);
            app_indicator_set_menu(mStatusIcon, GTK_MENU(mStatusIconMenu));
#else
            mStatusIcon = gtk_status_icon_new_from_file(LD_ICON_PATH);

            g_signal_connect(G_OBJECT(mStatusIcon), "button_press_event",
                G_CALLBACK(tray_icon_on_click), NULL);

            g_signal_connect(G_OBJECT(mStatusIcon), "popup-menu",
                G_CALLBACK(tray_icon_on_menu), NULL);

            gtk_status_icon_set_tooltip(mStatusIcon, LD_APPNAME);
            gtk_status_icon_set_visible(mStatusIcon, TRUE);
#endif // LD_USE_APPINDICATOR

        }

        /*static*/ void UIManager::CloseGracefully() {
            INFO("Called CloseGracefully - Shutting down!");

            Filesystem::PIDFileSet("cefapp.pid", 0);
            CefQuitMessageLoop();
            INFO("Exited message loop");
            CefShutdown();
            INFO("Successfully destroyed CEF");
            exit(0);
        }

        /*static*/ bool UIManager::SaveFileDialog(const std::string& filename, const std::string& current_name, std::string& res) {
            GtkWidget* dialog;
            GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;

            dialog = gtk_file_chooser_dialog_new("Save File", GTK_WINDOW(mMainWindow), action,
                "_Cancel", GTK_RESPONSE_CANCEL,
                "_Save", GTK_RESPONSE_ACCEPT,
                NULL
            );

            GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
            gtk_file_chooser_set_filename(chooser, filename.c_str());
            gtk_file_chooser_set_current_name(chooser, current_name.c_str());

            bool retval = false;

            if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
                char* selected_path = gtk_file_chooser_get_filename(chooser);
                res = std::string(selected_path);
                g_free(selected_path);
                retval = true;
            }

            gtk_widget_destroy(dialog);

            return retval;
        }

        /*static*/ void UIManager::ShowDownloadFinishedDialog() {
            GtkWidget* dialog;

            dialog = gtk_message_dialog_new(
                GTK_WINDOW(mMainWindow),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_CLOSE,
                "Download successed!");

            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }
}
