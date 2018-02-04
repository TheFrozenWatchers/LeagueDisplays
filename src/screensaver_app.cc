
#include "screensaver_app.h"
#include "client_handler.h"

#include <string>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "uibase.h"
#include "scheme_handlers.h"
#include "log.h"
#include "filesystem.h"

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "ScreensaverApp"

namespace LeagueDisplays {
    namespace CEF {
        ScreensaverApp::ScreensaverApp() {}

        void ScreensaverApp::OnBeforeCommandLineProcessing(const CefString& process_type,
              CefRefPtr<CefCommandLine> command_line) {

            if (!command_line->HasSwitch("allow-running-insecure-content"))
                command_line->AppendSwitch("allow-running-insecure-content");

            if (!command_line->HasSwitch("v8-natives-passed-by-fd"))
                command_line->AppendSwitch("v8-natives-passed-by-fd");

            if (command_line->HasSwitch("ld-host-pid")) {
                const char *pids = command_line->GetSwitchValue("ld-host-pid").ToString().c_str();
                UI::UIManager::mHostPID = atoi(pids);
                DEBUG("Host pid: %s", pids);
            }

            unsigned short pid = Filesystem::PIDFileGet("screensaver.pid");
            DEBUG("PID from file: %d", pid);
            Filesystem::PIDFileSet("screensaver.pid", UI::UIManager::mHostPID);
        }

        void ScreensaverApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) {
            command_line->AppendSwitchWithValue("ld-host-pid", std::to_string(UI::UIManager::mHostPID));
            command_line->AppendSwitchWithValue("ld-mode", "screensaver");

            if (CefCommandLine::GetGlobalCommandLine()->HasSwitch("silent-logging")) {
                command_line->AppendSwitch("silent-logging");
            }
        }
    }
}
