
#ifndef LD_CLIENT_HANDLER_H
#define LD_CLIENT_HANDLER_H

#include "include/cef_client.h"
#include "uibase.h"

#include <list>

namespace LeagueDisplays {
    namespace CEF {
        class ClientHandler : public CefClient,
                              public CefLifeSpanHandler,
                              public CefLoadHandler,
                              public CefRequestHandler,
                              public CefDownloadHandler,
                              public CefContextMenuHandler {
        public:
            explicit ClientHandler();
            ~ClientHandler();

            // Provide access to the single global instance of this object.
            static ClientHandler *GetInstance();

            // CefClient methods:
            virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE {
                return this;
            }
            
            virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE {
                return this;
            }
            
            virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE {
                return this;
            }

            virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() OVERRIDE {
                return this;
            }

            virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() OVERRIDE {
                return this;
            }

            virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE {
                return UI::UIManager::mIsScreensaver ? UI::UIManager::mScreensaverHandler : NULL;
            }

            // CefLifeSpanHandler methods:
            virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
            virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
            virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

            // CefLoadHandler methods:
            virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                ErrorCode errorCode,
                const CefString& errorText,
                const CefString& failedUrl) OVERRIDE;

            // Request that all existing browser windows close.
            void CloseAllBrowsers(bool force_close);

            bool IsClosing() const { return is_closing_; }

            virtual bool OnQuotaRequest(CefRefPtr<CefBrowser> browser,
                const CefString& origin_url,
                int64 new_size,
                CefRefPtr<CefRequestCallback> callback) OVERRIDE;

            virtual void OnBeforeDownload(CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefDownloadItem> download_item,
                const CefString& suggested_name,
                CefRefPtr<CefBeforeDownloadCallback> callback) OVERRIDE;

            virtual void OnDownloadUpdated(CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefDownloadItem> download_item,
                CefRefPtr<CefDownloadItemCallback> callback) OVERRIDE;

            virtual bool RunContextMenu(CefRefPtr<CefBrowser> browser,
                CefRefPtr<CefFrame> frame,
                CefRefPtr<CefContextMenuParams> params,
                CefRefPtr<CefMenuModel> model,
                CefRefPtr<CefRunContextMenuCallback> callback) OVERRIDE;

         private:
            // List of existing browser windows. Only accessed on the CEF UI thread.
            typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
            BrowserList browser_list_;

            bool is_closing_;

            // Include the default reference counting implementation.
            IMPLEMENT_REFCOUNTING(ClientHandler);
        };
    }
}

#endif // LD_CLIENT_HANDLER_H
