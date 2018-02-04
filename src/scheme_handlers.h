
#ifndef LD_SCHEME_HANDLERS_H
#define LD_SCHEME_HANDLERS_H

#pragma once

#include "include/cef_scheme.h"
#include "log.h"

#include <sstream>

namespace LeagueDisplays {
    namespace CEF {
        enum FeatureFlags : int {
            LD_FEAUTURE_FILESYSTEM      = (1 << 0),
            LD_FEAUTURE_NOTIFICATIONS   = (1 << 1),
            LD_FEAUTURE_THEMES          = (1 << 2),
            LD_FEAUTURE_OFFLINE_SS      = (1 << 3),
            LD_FEAUTURE_MP4             = (1 << 4),
            LD_FEAUTURE_ASSISTANT       = (1 << 5),
            LD_FEAUTURE_SCREENSAVER     = (1 << 6),
            LD_FEAUTURE_POWER_SETTINGS  = (1 << 7),
            LD_FEAUTURE_LOCKSCREEN      = (1 << 8)
        };

        template<typename T>
        class BasicSchemeHandlerFactory : public CefSchemeHandlerFactory {
            public:
                virtual CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                    const CefString& scheme_name, CefRefPtr<CefRequest> request) OVERRIDE
                {
                    return new T();
                }
            private:
                IMPLEMENT_REFCOUNTING(BasicSchemeHandlerFactory);
        };

        class FilesystemResourceHandler : public CefResourceHandler {
            public:
                virtual bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) OVERRIDE;
                virtual void GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl) OVERRIDE;
                virtual bool ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) OVERRIDE;
                virtual void Cancel() OVERRIDE;
            private:
                int                 mErrorCode;
                int64               mResponseLength;
                FILE*               mFileStream;

                IMPLEMENT_REFCOUNTING(FilesystemResourceHandler);
        };

        class NotificationsResourceHandler : public CefResourceHandler {
            public:
                virtual bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) OVERRIDE;
                virtual void GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl) OVERRIDE;
                virtual bool ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) OVERRIDE;
                virtual void Cancel() OVERRIDE;
            private:
                int                 mErrorCode;

                IMPLEMENT_REFCOUNTING(NotificationsResourceHandler);
        };

        class ThemesResourceHandler : public CefResourceHandler {
            public:
                virtual bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) OVERRIDE;
                virtual void GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl) OVERRIDE;
                virtual bool ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) OVERRIDE;
                virtual void Cancel() OVERRIDE;
            private:
                int                 mErrorCode;
                std::stringstream   mStream;

                IMPLEMENT_REFCOUNTING(ThemesResourceHandler);
        };
    }
}

#endif // LD_SCHEME_HANDLERS_H
