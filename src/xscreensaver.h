
#ifndef LD_XSCREENSAVER_H
#define LD_XSCREENSAVER_H

#include "include/cef_render_handler.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>

namespace LeagueDisplays {
    namespace UI {
        int XSSErrorHandlerImpl(Display *display, XErrorEvent *event);
        int XSSIOErrorHandlerImpl(Display *display);

        class XScreensaver : public CefRenderHandler {
            public:
                void Create(int argc, char *argv[]);

                void RunEventLoop();
                Window GetWindow();

                static void CreateLaunchScript();
                static void ModifyXscreensaverFile();

                // CefRenderHandler
                virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) OVERRIDE;
                virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
                    const RectList &dirtyRects, const void* buffer, int w, int h) OVERRIDE;
            private:
                int width,
                    height,
                    x,
                    y;
                
                char *pixbuf;
                size_t pixbuf_size;
                
                Window                  root, win, window_override;
                Display*                dpy;
                XVisualInfo             vi;
                Colormap                cmap;
                XSetWindowAttributes    swa;
                GC                      gc;
                bool                    onRoot;

                void CheckFrameBuffer(int w, int h);

                IMPLEMENT_REFCOUNTING(XScreensaver);
        };
    }
}

#endif // LD_XSCREENSAVER_H
