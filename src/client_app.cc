
#include "client_app.h"
#include "client_handler.h"

#include <string>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>

#include "uibase.h"
#include "scheme_handlers.h"
#include "log.h"
#include "crossde.h"
#include "filesystem.h"

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

#include "leaguedisplays_version.h"

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif

#define LOGGING_SOURCE "ClientApp"

#define LD_CEF_BIND_FUNC(name, handler) \
    obj->SetValue(name, CefV8Value::CreateFunction(name, new JavascriptFuncWrapper(handler)), \
    V8_PROPERTY_ATTRIBUTE_NONE)

namespace LeagueDisplays {
    namespace CEF {
        ClientApp::ClientApp() { }

        void ClientApp::OnBeforeCommandLineProcessing(const CefString& process_type,
              CefRefPtr<CefCommandLine> command_line) {

            if (!command_line->HasSwitch("allow-running-insecure-content"))
                command_line->AppendSwitch("allow-running-insecure-content");

            if (!command_line->HasSwitch("v8-natives-passed-by-fd"))
                command_line->AppendSwitch("v8-natives-passed-by-fd");

            if (command_line->HasSwitch("ld-host-pid")) {
                const char* pids = command_line->GetSwitchValue("ld-host-pid").ToString().c_str();
                UI::UIManager::mHostPID = atoi(pids);
                DEBUG("Host pid: %s", pids);
            }

            unsigned short pid = Filesystem::PIDFileGet("cefapp.pid");
            
            DEBUG("PID from file: %d", pid);

            if (pid && pid != UI::UIManager::mHostPID && !kill(pid, 0)) {
                ERROR("CEF root process is already running!");
                exit(0);
                return;
            }

            Filesystem::PIDFileSet("cefapp.pid", UI::UIManager::mHostPID);
        }

        void ClientApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) {
            command_line->AppendSwitchWithValue("ld-host-pid", std::to_string(UI::UIManager::mHostPID));
            command_line->AppendSwitchWithValue("ld-mode", "cef-app");

            if (CefCommandLine::GetGlobalCommandLine()->HasSwitch("silent-logging")) {
                command_line->AppendSwitch("silent-logging");
            }
        }

        void ClientApp::OnContextInitialized() {
            CEF_REQUIRE_UI_THREAD();

            CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();

            // ClientHandler implements browser-level callbacks.
            CefRefPtr<ClientHandler> handler(new ClientHandler());

            // Specify CEF browser settings here.
            CefBrowserSettings browser_settings;
            browser_settings.local_storage = STATE_ENABLED;
            browser_settings.databases = STATE_ENABLED;
            browser_settings.application_cache = STATE_ENABLED;
            browser_settings.webgl = STATE_ENABLED;
            browser_settings.web_security = STATE_DISABLED;
            CefString(&browser_settings.default_encoding).FromASCII("UTF-8");

            CefRegisterSchemeHandlerFactory("http", "filesystem",
                new LeagueDisplays::CEF::BasicSchemeHandlerFactory<LeagueDisplays::CEF::FilesystemResourceHandler>());

            /*CefRegisterSchemeHandlerFactory("http", "notifications",
                new LeagueDisplays::CEF::BasicSchemeHandlerFactory<LeagueDisplays::CEF::NotificationsResourceHandler>());*/

            CefRegisterSchemeHandlerFactory("http", "themes",
                new LeagueDisplays::CEF::BasicSchemeHandlerFactory<LeagueDisplays::CEF::ThemesResourceHandler>());

            std::string url;

            url = command_line->GetSwitchValue("url-override");
            if (url.empty())
                url = "https://screensaver.riotgames.com/v2/latest/index.html#/configurator?src=";


            LeagueDisplays::UI::UIManager::Create();

            CefWindowInfo window_info;

            window_info.parent_window = LeagueDisplays::UI::UIManager::mX11WindowID;
            window_info.height = LeagueDisplays::UI::UIManager::mWindowHeight;
            window_info.width  = LeagueDisplays::UI::UIManager::mWindowWidth;

            // Create the first browser window.
            CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, NULL);
            INFO("CEF initialization completed!");
        }

        // JavaScript bindings:
        JavascriptFuncWrapper::JavascriptFuncWrapper(std::function<bool(CefRefPtr<CefV8Value>&, const CefV8ValueList&)> func) {
            this->mFunc = func;
        }

        bool JavascriptFuncWrapper::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments,
                    CefRefPtr<CefV8Value>& retval, CefString& exception) {

            return this->mFunc(retval, arguments);
        }

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "JSBindings"

#define LOGGING_FUNCTION_NAME_OVERRIDE "Execute"

        bool _LolScreenSaverCapabilities_func_(CefRefPtr<CefV8Value>& retval, const CefV8ValueList& arguments) {
            int bitfield = 0;

            bitfield |= FeatureFlags::LD_FEAUTURE_FILESYSTEM;
            // bitfield |= FeatureFlags::LD_FEAUTURE_NOTIFICATIONS;
            bitfield |= FeatureFlags::LD_FEAUTURE_THEMES;
            bitfield |= FeatureFlags::LD_FEAUTURE_OFFLINE_SS;
            // bitfield |= FeatureFlags::LD_FEAUTURE_MP4;
            bitfield |= FeatureFlags::LD_FEAUTURE_ASSISTANT;
            bitfield |= FeatureFlags::LD_FEAUTURE_SCREENSAVER;
            // bitfield |= FeatureFlags::LD_FEAUTURE_POWER_SETTINGS;
            bitfield |= FeatureFlags::LD_FEAUTURE_LOCKSCREEN;

            DEBUG_F("LolScreenSaverCapabilities - %d", bitfield);
            retval = CefV8Value::CreateInt(bitfield);
            return true;
        }

        bool _LolScreenSaverClientVersion_func_(CefRefPtr<CefV8Value>& retval, const CefV8ValueList& arguments) {
            DEBUG_F("LolScreenSaverClientVersion - %s", LD_APPVERSION);
            retval = CefV8Value::CreateString(LD_APPVERSION);
            return true;
        }

        bool _LolScreenSaverOSLanguage_func_(CefRefPtr<CefV8Value>& retval, const CefV8ValueList& arguments) {
            DEBUG_F("LolScreenSaverOSLanguage - %s", std::locale("").name().c_str());
            retval = CefV8Value::CreateString(std::locale("").name());
            return true;
        }

        bool _LolScreenSaverCloseConfigurator_func_(CefRefPtr<CefV8Value>& retval, const CefV8ValueList& arguments) {
            DEBUG_F("LolScreenSaverCloseConfigurator");
            UI::UIManager::CloseGracefully();
            return true;
        }

        bool _LolScreenSaverOpenUrl_func_(CefRefPtr<CefV8Value>& retval, const CefV8ValueList& arguments) {
            std::string url = arguments[0]->GetStringValue().ToString();
            DEBUG_F("LolScreenSaverOpenUrl - %s", url.c_str());
            DesktopEnvApi::OpenUrlInDefaultBrowser(url); // <- crashes CEF
            return true;
        }

        void ClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
            CefRefPtr<CefV8Context> context) {

            CefRefPtr<CefV8Value> obj = context->GetGlobal();

            DEBUG("Binding functions...");

            LD_CEF_BIND_FUNC("LolScreenSaverCapabilities",      _LolScreenSaverCapabilities_func_       );
            LD_CEF_BIND_FUNC("LolScreenSaverClientVersion",     _LolScreenSaverClientVersion_func_      );
            LD_CEF_BIND_FUNC("LolScreenSaverOSLanguage",        _LolScreenSaverOSLanguage_func_         );
            LD_CEF_BIND_FUNC("LolScreenSaverCloseConfigurator", _LolScreenSaverCloseConfigurator_func_  );
            // LD_CEF_BIND_FUNC("LolScreenSaverOpenUrl",           _LolScreenSaverOpenUrl_func_            );
        }

#undef LOGGING_FUNCTION_NAME_OVERRIDE
    }
}
