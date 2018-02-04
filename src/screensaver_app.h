
#ifndef LD_SCREENSAVER_APP_H
#define LD_SCREENSAVER_APP_H

#include "include/cef_app.h"
#include "include/cef_v8.h"

#include <functional>

namespace LeagueDisplays {
    namespace CEF {
        // Implement application-level callbacks for the browser process.
        class ScreensaverApp : public CefApp,
                               public CefBrowserProcessHandler {
            public:
                ScreensaverApp();

                // CefApp methods:
                virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE {
                    return this;
                }

                virtual void OnBeforeCommandLineProcessing(const CefString& process_type,
                    CefRefPtr<CefCommandLine> command_line) OVERRIDE;

                // CefBrowserProcessHandler methods:
                virtual void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) OVERRIDE;
            private:
                // Include the default reference counting implementation.
                IMPLEMENT_REFCOUNTING(ScreensaverApp);
        };
    }
}

#endif // LD_SCREENSAVER_APP_H
