
#include <mcheck.h>
#include <cstring>
#include <X11/Xlib.h>

#include "include/base/cef_logging.h"

#include "client_app.h"
#include "filesystem.h"
#include "log.h"
#include "background_daemon.h"
#include "crossde.h"
#include "uibase.h"
#include "themes_service.h"
#include "screensaver_app.h"
#include "client_handler.h"
#include "scheme_handlers.h"

int cef_app_main(int argc, char *argv[]) {
    CefMainArgs main_args(argc, argv);

    CefRefPtr<LeagueDisplays::CEF::ClientApp> app(new LeagueDisplays::CEF::ClientApp);
    int exit_code = CefExecuteProcess(main_args, app.get(), NULL);

    if (exit_code >= 0)
        return exit_code;

    // Specify CEF global settings here.
    CefSettings settings;
    settings.no_sandbox = 1;
    settings.log_severity = LOGSEVERITY_DISABLE;
    settings.remote_debugging_port = 9222;
    
    CefString(&settings.user_agent).FromASCII("LeagueDisplays");
    CefString(&settings.log_file).FromString(LeagueDisplays::Filesystem::mDebugLogPath);

    // NOTE: If the path ends with '/', CEF will crash with 
    // "Check failed: (partition_path == profile_path) || profile_path.IsParent(partition_path)."
    std::string cache_path =
        LeagueDisplays::Filesystem::mCefCacheDir.substr(0, LeagueDisplays::Filesystem::mCefCacheDir.size() - 1);
    CefString(&settings.cache_path).FromString(cache_path);

    CefInitialize(main_args, settings, app.get(), NULL);

    DEBUG("Started message loop");
    CefRunMessageLoop();
    DEBUG("Finished message loop");
    return 0;
}

int agent_app_main(int argc, char *argv[]) {
    LeagueDisplays::DesktopEnvApi::LogDesktopEnv();
    LeagueDisplays::BackgroundDaemon::Start();

    gtk_init(&argc, &argv);

    LeagueDisplays::UI::UIManager::CreateAgent();
    gtk_main();

    return 0;
}

int screensaver_app_main(int argc, char *argv[]) {
    char *argv2[argc];
    for (int i = 0; i < argc; i++) {
        argv2[i] = (char*)malloc(strlen(argv[i]) + 1);
        strcpy(argv2[i], argv[i]);
    }

    LeagueDisplays::Filesystem::ScreensaverInit();
    CefMainArgs main_args(argc, argv);

    CefRefPtr<LeagueDisplays::CEF::ScreensaverApp> app(new LeagueDisplays::CEF::ScreensaverApp);
    int exit_code = CefExecuteProcess(main_args, app.get(), NULL);

    if (exit_code >= 0)
        return exit_code;

    // Specify CEF global settings here.
    CefSettings settings;
    settings.no_sandbox = 1;
    settings.windowless_rendering_enabled = 1;
    settings.background_color = 0;
    settings.log_severity = LOGSEVERITY_DISABLE;
    
    CefString(&settings.user_agent).FromASCII("LeagueDisplays");
    CefString(&settings.log_file).FromString(LeagueDisplays::Filesystem::mDebugLogPath);

    // NOTE: If the path ends with '/', CEF will crash with 
    // "Check failed: (partition_path == profile_path) || profile_path.IsParent(partition_path)."
    std::string cache_path =
        LeagueDisplays::Filesystem::mCefCacheDir.substr(0, LeagueDisplays::Filesystem::mCefCacheDir.size() - 1);
    CefString(&settings.cache_path).FromString(cache_path);

    if (!CefInitialize(main_args, settings, app.get(), NULL)) {
        return -1;
    }

    if (!LeagueDisplays::Filesystem::Exists(LeagueDisplays::Filesystem::mFilesystemCacheDir + "screensaver/")) {
        if (!LeagueDisplays::Filesystem::Unzip("screensaver-webapp.bin", LeagueDisplays::Filesystem::mFilesystemCacheDir + "screensaver/")) {
            ERROR("Screensaver assets are missing!");
            return -1;
        }
    }

    LeagueDisplays::UI::XScreensaver *xscr = new LeagueDisplays::UI::XScreensaver();
    xscr->Create(argc, argv2);

    std::string url;
    
    url = CefCommandLine::GetGlobalCommandLine()->GetSwitchValue("url-override");
    if (url.empty())
        url = "http://filesystem/screensaver/index.html";

    // ClientHandler implements browser-level callbacks.
    CefRefPtr<LeagueDisplays::CEF::ClientHandler> handler(new LeagueDisplays::CEF::ClientHandler());

    // Specify CEF browser settings here.
    CefBrowserSettings browser_settings;
    browser_settings.web_security = STATE_DISABLED;
    browser_settings.windowless_frame_rate = 60;
    CefString(&browser_settings.default_encoding).FromASCII("UTF-8");

    CefRegisterSchemeHandlerFactory("http", "filesystem",
        new LeagueDisplays::CEF::BasicSchemeHandlerFactory<LeagueDisplays::CEF::FilesystemResourceHandler>());

    CefWindowInfo window_info;
    window_info.SetAsWindowless(xscr->GetWindow());

    CefBrowserHost::CreateBrowserSync(window_info, handler, url, browser_settings, NULL);
    
    DEBUG("Started message loop");
    xscr->RunEventLoop();
    DEBUG("Finished message loop");

    return 0;
}

int main_proxy(int argc, char *argv[], CefRefPtr<CefCommandLine> cmdline) {
    if (!cmdline->HasSwitch("ld-host-pid")) {
        LeagueDisplays::UI::UIManager::mHostPID = getpid();
    }

    if (cmdline->HasSwitch("allow-multiple-instances")) {
        LeagueDisplays::UI::UIManager::mAllowMultipleInstances = true;
    }

    LeagueDisplays::UI::XScreensaver::CreateLaunchScript();
    LeagueDisplays::UI::XScreensaver::ModifyXscreensaverFile();

    std::string mode = "launch";

    // Alias for --ld-mode=agent
    if (cmdline->HasSwitch("tray")) {
        mode = "agent";
    } else if (cmdline->HasSwitch("ld-mode")) {
        mode = cmdline->GetSwitchValue("ld-mode");
        DEBUG("--ld-mode switch is present");
    }

    switch (LeagueDisplays::fnvHash(mode.c_str())) {
        case LeagueDisplays::fnvHashConst("cef-app"):
            cef_app_main(argc, argv);
            break;
        case LeagueDisplays::fnvHashConst("screensaver"):
            screensaver_app_main(argc, argv);
            break;
        case LeagueDisplays::fnvHashConst("agent"): {
            unsigned short pid = LeagueDisplays::Filesystem::PIDFileGet("agent.pid");
            
            if (pid && pid != getpid() && !kill(pid, 0)) {
                ERROR("Agent process is already running!");
                return 0;
            }

            LeagueDisplays::Filesystem::PIDFileSet("agent.pid", getpid());

            agent_app_main(argc, argv);

            LeagueDisplays::Filesystem::PIDFileSet("agent.pid", 0);
            break;
        }
        case LeagueDisplays::fnvHashConst("launch"): {
            unsigned short pid = LeagueDisplays::Filesystem::PIDFileGet("agent.pid");
            
            if (!pid || kill(pid, 0)) {
                INFO("Agent is not running, starting it!");
                
                std::string cmd = "\"" + LeagueDisplays::Filesystem::mExecutablePath + "\" --ld-mode=agent --ld-fork";

                int r = system(cmd.c_str());

                if (r < 0) {
                    ERROR("Could not start agent!");
                    return 0;
                }

                sleep(1);
                pid = LeagueDisplays::Filesystem::PIDFileGet("agent.pid");
            }

            kill(pid, SIGUSR1);
            break;
        }
        default:
            WARN("Unknown mode: %s", mode.c_str());
            break;
    }

    return 0;
}

// Entry point function for all processes.
int main(int argc, char *argv[]) {
    mtrace();

    CefRefPtr<CefCommandLine> cmdline = CefCommandLine::CreateCommandLine();
    cmdline->InitFromArgv(argc, argv);

    if (cmdline->HasSwitch("silent-logging")) {
        LeagueDisplays::Logger::DisableConsoleLogging();
    }

    LeagueDisplays::Logger::mEnableFileLogging = !cmdline->HasSwitch("type");

    LeagueDisplays::Logger::SetVerboseLevel(10); // TODO: command line switch
    LeagueDisplays::Filesystem::Initialize();

    DEBUG("-- Command line dump start ---");

    for (int i = 1; i < argc; i++) {
        DEBUG(" @%02d  %s", i, argv[i]);
        usleep(100);
    }

    DEBUG("-- Command line dump end ---");

    char *absolute_path = (char*)malloc(PATH_MAX);
    realpath(cmdline->GetProgram().ToString().c_str(), absolute_path);

    LeagueDisplays::Filesystem::mExecutablePath = absolute_path;
    free(absolute_path);

    if (cmdline->HasSwitch("get-desktop-env")) {
        LeagueDisplays::DesktopEnvApi::LogDesktopEnv();
        return 0;
    }

    if (cmdline->HasSwitch("ld-fork")) {
        DEBUG("Running in daemon mode");
        pid_t pid = fork();

        if (pid == 0) {
            return main_proxy(argc, argv, cmdline);
        }
    } else {
        return main_proxy(argc, argv, cmdline);
    }

    return 0;
}
