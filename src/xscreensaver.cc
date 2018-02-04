
#include "uibase.h"
#include "log.h"
#include "filesystem.h"
#include "xscreensaver_config.h"

#include <stdio.h>
#include <iostream>
#include <sstream>

#include <sys/stat.h>

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"
#include "include/wrapper/cef_helpers.h"

#include "vroot.h"

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "XScreensaver"

namespace LeagueDisplays {
    namespace UI {
        int XSSErrorHandlerImpl(Display* display, XErrorEvent* event) {
            Display* dpy = XOpenDisplay(NULL);
            char errormsg[128];
            XGetErrorText(dpy, event->error_code, errormsg, 127);
            XCloseDisplay(dpy);

            fprintf(stderr, "X11 error: type=%d, serial=%lu, code=%d [%s]\n",
                event->type, event->serial, (int)event->error_code, errormsg);

            exit(0);
            return 0;
        }

        int XSSIOErrorHandlerImpl(Display* display) {
            exit(0);
            return 0;
        }

        void XScreensaver::Create(int argc, char* argv[]) {
            UIManager::mScreensaverHandler = this;
            UIManager::mIsScreensaver = true;
            window_override = 0;
            onRoot = false;
            bool clearRoot = true;

            for (int i = 0; i < argc; i++) {
                if (!strcmp(argv[i], "-window-id") && i < argc - 1) {
                    sscanf(argv[i + 1], "0x%X", &window_override);
                } else if (!strcmp(argv[i], "-root")) {
                    onRoot = true;
                } else if (!strcmp(argv[i], "-dontClearRoot")) {
                    clearRoot = false;
                }
            }

            XtAppContext ctx;
            XrmOptionDescRec opts;
            Widget toplevel = XtAppInitialize(&ctx, "LoLScreensaver", &opts, 0, &argc, argv, NULL, 0, 0);

            XSetErrorHandler(XSSErrorHandlerImpl);
            XSetIOErrorHandler(XSSIOErrorHandlerImpl);

            pixbuf = NULL;
            pixbuf_size = 0;

            dpy = XtDisplay(toplevel);
            XMatchVisualInfo(dpy, DefaultScreen(dpy), 32, TrueColor, &vi);

            XWindowAttributes wa;

            DEBUG("Creating renderer: window=0x%x,onRoot=%s,clearRoot=%s\n", window_override, onRoot ? "true" : "false", clearRoot ? "true" : "false");
            
            if (window_override) {
                win = window_override;
                XtDestroyWidget(toplevel);
                XGetWindowAttributes(dpy, win, &wa);
                wa.your_event_mask |= KeyPressMask | StructureNotifyMask | VisibilityChangeMask | FocusChangeMask;
                XSelectInput(dpy, win, wa.your_event_mask);

                if (!(wa.all_event_masks & (ButtonPressMask | ButtonReleaseMask)))
                    XSelectInput(dpy, win, wa.your_event_mask | ButtonPressMask | ButtonReleaseMask);
            } else if (onRoot) {
                win = VirtualRootWindowOfScreen(XtScreen(toplevel));
                XtDestroyWidget(toplevel);

                XGetWindowAttributes(dpy, win, &wa);
                XSelectInput(dpy, win, wa.your_event_mask | StructureNotifyMask | VisibilityChangeMask | FocusChangeMask);
            } else {
                win = RootWindowOfScreen(XtScreen(toplevel));
                XtDestroyWidget(toplevel);

                int screenid = DefaultScreen(dpy);
                win = XCreateSimpleWindow(dpy, win, 0, 0, 800, 600, 1, WhitePixel(dpy, screenid), BlackPixel(dpy, screenid));
                XMapWindow(dpy, win);
                XMoveResizeWindow(dpy, win, 0, 0, 800, 600);
                XGetWindowAttributes(dpy, win, &wa);

                XSelectInput(dpy, win, wa.your_event_mask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | FocusChangeMask);
                XGetWindowAttributes(dpy, win, &wa);
            }

            if (clearRoot) {
                XClearWindow(dpy, win);
            }

            x = wa.x;
            y = wa.y;
            height = wa.height;
            width = wa.width;
            CheckFrameBuffer(width, height);
            DEBUG("Target window: 0x%lx [%d %d %d %d]\n", win, height, width, x, y);
    
            gc = XCreateGC(dpy, win, 0, NULL);
        }

        void XScreensaver::RunEventLoop() {
            XEvent event;
            
            while (true) {
                while (XPending(dpy)) {
                    XNextEvent(dpy, &event);
        
                    DEBUG("Event: %d\n", event.type);

                    if (event.type == ClientMessage ||
                        event.type == DestroyNotify) {
                        goto shutdown;
                    }

                    if (onRoot && !window_override && event.type == UnmapNotify) {
                        goto shutdown;
                    }
                }
        
                XWindowAttributes wa;
                XGetWindowAttributes(dpy, win, &wa);
        
                if (height != wa.height || width != wa.width) {
                    DEBUG("Window resized [%dx%d] -> [%dx%d]\n", width, height, wa.width, wa.height);
                    height = wa.height;
                    width = wa.width;
                    CheckFrameBuffer(width, height);
                }
                
                CefDoMessageLoopWork();
                usleep(10);
            }

            shutdown:
            CefShutdown();
        }

        Window XScreensaver::GetWindow() {
            return win;
        }

        /*static*/ void XScreensaver::CreateLaunchScript() {
            if (Filesystem::mExecutablePath == "/proc/self/exe") {
                return;
            }

            std::string path = Filesystem::mRootDir + "LoLScreensaver";
            FILE* f = fopen(path.c_str(), "w");
            assert(f);

            if (Filesystem::Exists("/bin/sh")) {
                fprintf(f, "#!/bin/sh\n");
            } else if (Filesystem::Exists("/bin/bash")) {
                fprintf(f, "#!/bin/bash\n");
            } else if (Filesystem::Exists("/usr/bin/sh")) {
                fprintf(f, "#!/usr/bin/sh\n");
            } else if (Filesystem::Exists("/usr/bin/bash")) {
                fprintf(f, "#!/usr/bin/bash\n");
            } else {
                const char *env_shell = getenv("SHELL");
                if (env_shell) {
                    fprintf(f, "#!%s\n", env_shell);
                } else {
                    ERROR("Could not figure out user's shell, this could cause screensaver malfunction");
                }
            }

            fprintf(f, "\ncd %s\n", Filesystem::mWorkingDirectory.c_str());
            fprintf(f, "\"%s\" --silent-logging --ld-mode=screensaver $@\n", Filesystem::mExecutablePath.c_str());
            fprintf(f, "exit $?\n");

            fclose(f);
            chmod(path.c_str(), 0700);
        }

        /*static*/ void XScreensaver::ModifyXscreensaverFile() {
            std::string path = Filesystem::mUserHome + "/.xscreensaver";

            if (!Filesystem::Exists(path)) {
                INFO("xscreensaver config file does not exist, starting 'xscreensaver-demo' to create one");
                system("xscreensaver-demo &\nsleep 2 && killall xscreensaver-demo");
            }

            if (!Filesystem::Exists(path)) {
                ERROR("Could not create xscreensaver config file");
                exit(-1);
            }

            XScreensaverConfig* cfg = new XScreensaverConfig(path);
            cfg->AddOrModifyScreensaver("LoLScreensaver", Filesystem::mRootDir + "LoLScreensaver -root", "GL", true);
            cfg->Save();

            delete cfg;
        }

        // CefRenderHandler
        bool XScreensaver::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
            DEBUG("GetViewRect -> %d %d %dx%d\n", x, y, width, height);
            rect = CefRect(x, y, width, height);
            return true;
        }

        void XScreensaver::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void * buffer, int w, int h) {
            CheckFrameBuffer(w, h);
            memcpy(pixbuf, buffer, w * h * 4);

            XImage* img = XCreateImage(dpy, vi.visual, 24, ZPixmap, 0, pixbuf, w, h, 32, 0);

            XPutImage(dpy, win, gc, img, 0, 0, 0, 0, width, height);
        }

        void XScreensaver::CheckFrameBuffer(int w, int h) {
            if (!pixbuf || pixbuf_size < w * h * 4) {
                if (pixbuf) {
                    DEBUG("Freeing old frame buffer\n");
                    free(pixbuf);
                }

                pixbuf = (char*)malloc(w * h * 4);
                assert(pixbuf);

                pixbuf_size = w * h * 4;
                DEBUG("Allocating new framebuffer %dx%d (%d bytes)\n", w, h, pixbuf_size);
            }
        }
    }
}
