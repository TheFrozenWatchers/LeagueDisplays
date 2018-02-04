
#ifndef LD_CLIENT_APP_H
#define LD_CLIENT_APP_H

#include "include/cef_app.h"
#include "include/cef_v8.h"

#include <functional>

namespace LeagueDisplays {
    namespace CEF {
        // Implement application-level callbacks for the browser process.
        class ClientApp : public CefApp,
                          public CefBrowserProcessHandler,
                          public CefRenderProcessHandler {
            public:
                ClientApp();

                // CefApp methods:
                virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() OVERRIDE {
                    return this;
                }

                virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE {
                    return this;
                }

                virtual void OnBeforeCommandLineProcessing(const CefString& process_type,
                  CefRefPtr<CefCommandLine> command_line) OVERRIDE;

                // CefBrowserProcessHandler methods:
                virtual void OnContextInitialized() OVERRIDE;

                virtual void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) OVERRIDE;

                // CefRenderProcessHandler methods:
                virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                    CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) OVERRIDE;

            private:
                // Include the default reference counting implementation.
                IMPLEMENT_REFCOUNTING(ClientApp);
        };

        class JavascriptFuncWrapper : public CefV8Handler {
            public:
                explicit JavascriptFuncWrapper(std::function<bool(CefRefPtr<CefV8Value>&,
                    const CefV8ValueList&)> func);

            private:
                std::function<bool(CefRefPtr<CefV8Value>&, const CefV8ValueList&)> mFunc;

                virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object,
                    const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval,
                    CefString& exception) OVERRIDE;

                IMPLEMENT_REFCOUNTING(JavascriptFuncWrapper);
        };
    }
}

#endif // LD_CLIENT_APP_H
