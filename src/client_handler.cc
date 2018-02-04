
#include "client_handler.h"
#include "log.h"
#include "uibase.h"
#include "filesystem.h"

#include <sstream>
#include <string>
#include <regex.h>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "ClientHandler"

namespace LeagueDisplays {
    namespace CEF {
        ClientHandler* g_instance = NULL;

        ClientHandler::ClientHandler() : is_closing_(false) {
            DCHECK(!g_instance);
            g_instance = this;
        }

        ClientHandler::~ClientHandler() {
            g_instance = NULL;
        }

        /*static*/ ClientHandler* ClientHandler::GetInstance() {
            return g_instance;
        }

        void ClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
            CEF_REQUIRE_UI_THREAD();

            // Add to the list of existing browsers.
            browser_list_.push_back(browser);
        }

        bool ClientHandler::DoClose(CefRefPtr<CefBrowser> browser) {
            CEF_REQUIRE_UI_THREAD();

            // Closing the main window requires special handling. See the DoClose()
            // documentation in the CEF header for a detailed destription of this
            // process.
            if (browser_list_.size() == 1) {
                // Set a flag to indicate that the window close should be allowed.
                is_closing_ = true;
            }

            // Allow the close. For windowed browsers this will result in the OS close
            // event being sent.
            return false;
        }

        void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
            CEF_REQUIRE_UI_THREAD();

            // Remove from the list of existing browsers.
            BrowserList::iterator bit = browser_list_.begin();
            for (; bit != browser_list_.end(); ++bit) {
                if ((*bit)->IsSame(browser)) {
                    browser_list_.erase(bit);
                    break;
                }
            }

            if (browser_list_.empty()) {
                // All browser windows have closed. Quit the application message loop.
                CefQuitMessageLoop();
            }
        }

        void ClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
            ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) {

            CEF_REQUIRE_UI_THREAD();

            // Don't display an error for downloaded files.
            if (errorCode == ERR_ABORTED)
                return;

            // Display a load error message.
            std::stringstream ss;
            ss << "<html><body bgcolor=\"white\">"
                        "<h2>Failed to load URL "
                 << std::string(failedUrl) << " with error " << std::string(errorText)
                 << " (" << errorCode << ").</h2></body></html>";
            frame->LoadString(ss.str(), failedUrl);
        }

        void ClientHandler::CloseAllBrowsers(bool force_close) {
            if (!CefCurrentlyOn(TID_UI)) {
                // Execute on the UI thread.
                CefPostTask(TID_UI, base::Bind(&ClientHandler::CloseAllBrowsers, this, force_close));
                return;
            }

            if (browser_list_.empty())
                return;

            BrowserList::const_iterator it = browser_list_.begin();
            for (; it != browser_list_.end(); ++it)
                (*it)->GetHost()->CloseBrowser(force_close);
        }

        bool ClientHandler::OnQuotaRequest(CefRefPtr<CefBrowser> browser, const CefString& origin_url,
            int64 new_size, CefRefPtr<CefRequestCallback> callback) {

            CEF_REQUIRE_IO_THREAD();

            INFO("Requested quota: %ld", new_size);

            callback->Continue(true);
            return true;
        }

        void ClientHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
            const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback) {

            if (!download_item->IsValid()) {
                ERROR("download_item->IsValid() returned false!");
                return;
            }

            if (download_item->IsCanceled()) {
                WARN("The request is already cancelled!?");
                return;
            }

            std::string suggested_name_str = suggested_name.ToString();
            std::string path_str = Filesystem::mDefaultDownloadDir + suggested_name_str;

            std::string res;

            if (UI::UIManager::SaveFileDialog(path_str, suggested_name_str, res)) {
                DEBUG("Saving file to %s", res.c_str());

                callback->Continue(res, false);
            }
        }

        void ClientHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefDownloadItem> download_item,
            CefRefPtr<CefDownloadItemCallback> callback) {

            int progress = download_item->GetPercentComplete();
            int is_completed = download_item->IsComplete();

            DEBUG("%s progress: %3d%% [completed=%-5s]", download_item->GetSuggestedFileName().ToString().c_str(),
                progress, is_completed ? "true" : "false");

            if (is_completed) {
                UI::UIManager::ShowDownloadFinishedDialog();
            }
        }

        bool ClientHandler::RunContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
            CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model,
            CefRefPtr<CefRunContextMenuCallback> callback) {

            callback->Cancel();
            return true;
        }
    }
}
