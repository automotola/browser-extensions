#include "stdafx.h"
#include "NativeExtensions.h"
#include <generated/Forge_i.h> /* for: */
#include "dllmain.h"           /*   _AtlModule */
#include "Preferences.h"
#include "HTTP.h"
#include "NativeMessagingTypes.h"
#include "AccessibleBrowser.h"
#include "NotificationWindow.h"
#include "wininet.h"


/**
 * Construction
 */
CNativeExtensions::CNativeExtensions()
{
  logger->initialize(L"C:\\forge_com.log");
}

CNativeExtensions::~CNativeExtensions()
{
  logger->debug(L"CNativeExtensions::~CNativeExtensions");
}

/** 
 * Destruction
 */
HRESULT CNativeExtensions::FinalConstruct()
{
  return S_OK;
}

void CNativeExtensions::FinalRelease()
{
  logger->debug(L"CNativeExtensions::FinalRelease");
}

/**
 * InterfaceSupportsErrorInfo
 */
STDMETHODIMP CNativeExtensions::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* const arr[] = { &IID_INativeExtensions };
  for (auto arri : arr)
    if (InlineIsEqualGUID(*arri, riid))
      return S_OK;

  return S_FALSE;
}

/**
 * Method: NativeExtensions::log
 * 
 * @param uuid 
 * @param message
 */
STDMETHODIMP CNativeExtensions::log(BSTR uuid, BSTR message)
{
    if (_AtlModule.moduleManifest->logging.console) {
        logger->debug(L"[" + wstring(uuid) + L"] " + wstring(message));
    } 
    return S_OK;
}

/**
 * Method: NativeExtensions::prefs_get
 *
 * @param name
 * @param success
 * @param error  TODO
 */
STDMETHODIMP CNativeExtensions::prefs_get(BSTR uuid, BSTR name, IDispatch *success, IDispatch *error)
{
  HRESULT hr = S_OK;
  for (;;) {
    logger->debug(L"NativeExtensions::prefs_get"
      L" -> " + wstring(uuid) +
      L" -> " + wstring(name) +
      L" -> " + boost::lexical_cast<wstring>(success)+
      L" -> " + boost::lexical_cast<wstring>(error));

    wstring value(Preferences(uuid).get(name));
    logger->debug(L"NativeExtensions::prefs_get -> " + wstring_limit(value));

    CComQIPtr<IDispatchEx> dispatch(success);
    hr = CComDispatchDriver(dispatch).Invoke1((DISPID)0, &CComVariant(value.c_str()));    
    break;
  }
  
  if (FAILED(hr))
    logger->debug(L"NativeExtensions::prefs_get failed -> " + wstring(name) + L" -> " + logger->parse(hr));

  return hr;
}


/**
 * Method: NativeExtensions::prefs_set
 *
 * @param name
 * @param value
 * @param success
 * @param error TODO
 */
STDMETHODIMP CNativeExtensions::prefs_set(BSTR uuid, BSTR name, BSTR value, IDispatch *success, IDispatch *error)
{
  HRESULT hr = S_OK;
  wstring ret;
  for (;;) {
    logger->debug(L"NativeExtensions::prefs_set"
      L" -> " + wstring(uuid) +
      L" -> " + wstring(name) +
      L" -> " + wstring_limit(value) +
      L" -> " + boost::lexical_cast<wstring>(success)+
      L" -> " + boost::lexical_cast<wstring>(error));

    ret = Preferences(uuid).set(name, value);
    logger->debug(L"NativeExtensions::prefs_set -> " + wstring_limit(ret));

    CComQIPtr<IDispatchEx> dispatch(success);
    hr = CComDispatchDriver(dispatch).Invoke1((DISPID)0, &CComVariant(ret.c_str()));
    break;
  }

  if (FAILED(hr))
    logger->debug(L"NativeExtensions::prefs_set failed -> " + wstring(name) + L" -> " + wstring_limit(ret) + L" -> " + logger->parse(hr));

  return hr;
}


/**
 * Method: NativeExtensions::prefs_keys
 *
 * @param uuid
 * @param success
 * @param error 
 */
STDMETHODIMP CNativeExtensions::prefs_keys(BSTR uuid, IDispatch *success, IDispatch *error)
{
  HRESULT hr = S_OK;
  for (;;) {
    logger->debug(L"NativeExtensions::prefs_keys"
      L" -> " + wstring(uuid) +
      L" -> " + boost::lexical_cast<wstring>(success)+
      L" -> " + boost::lexical_cast<wstring>(error));
    auto& keys = Preferences(uuid).keys();
    wstring json = keys.size() ? L"\"['" + boost::algorithm::join(keys, L"', '") + L"']\"" : L"\"[]\"";
    hr = CComDispatchDriver(success).Invoke1((DISPID)0, &CComVariant(json.c_str()));

    if (FAILED(hr)) {
      logger->debug(L"NativeExtensions::prefs_all failed -> " + logger->parse(hr));
      hr = CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(L"'failed to enumerate keys'"));
    }

    break;
  }

  return hr;
}


/**
 * Method: NativeExtensions::prefs_all
 *
 * @param uuid
 * @param success
 * @param error 
 */
STDMETHODIMP CNativeExtensions::prefs_all(BSTR uuid, IDispatch *success, IDispatch *error)
{
  HRESULT hr = S_OK;
  for (;;) {
    logger->debug(L"NativeExtensions::prefs_all"
      L" -> " + wstring(uuid) +
      L" -> " + boost::lexical_cast<wstring>(success)+
      L" -> " + boost::lexical_cast<wstring>(error));

    // :TODO: for what that all needed?
    // auto& all = Preferences(uuid).all();

    hr = CComDispatchDriver(success).Invoke1((DISPID)0, &CComVariant(L"{}"));
    if (FAILED(hr)) {
      logger->error(L"NativeExtensions::prefs_all failed -> " + logger->parse(hr));
      hr = CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(L"'failed to enumerate keys'"));
    }

    break;
  }

  return hr;
}


/**
 * Method: NativeExtensions::prefs_clear
 *
 * @param uuid
 * @param name
 * @param success
 * @param error 
 */
STDMETHODIMP CNativeExtensions::prefs_clear(BSTR uuid, BSTR name, IDispatch *success, IDispatch *error)
{
  HRESULT hr = S_OK;
  for (;;) {
    logger->debug(L"NativeExtensions::prefs_clear"
      L" -> " + wstring(uuid) +
      L" -> " + wstring(name) +
      L" -> " + boost::lexical_cast<wstring>(success)+
      L" -> " + boost::lexical_cast<wstring>(error));

    bool ret = (wstring(name) == L"*") ? Preferences(uuid).clear() : Preferences(uuid).clear(name);
    if (ret) {
      hr = CComDispatchDriver(success).Invoke0((DISPID)0);
    } else {
      logger->error(L"NativeExtensions::prefs_clear failed -> " + wstring(name));
      wstring message(L"failed to clear key: " + wstring(name));
      hr = CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(message.c_str()));
    }

    break;
  }

  return hr;
}


/**
 * Method: NativeExtensions::getURL
 *
 * @param url
 * @param out_url
 */
STDMETHODIMP CNativeExtensions::getURL(BSTR url, BSTR *out_url)
{
  bfs::wpath path = _AtlModule.modulePath / bfs::wpath(L"src") / bfs::wpath(url);
  wstring ret = L"file:///" + wstring_replace(path.wstring(), L'\\', L'/');
  HRESULT hr = S_OK;

  for (;;) {
    BreakOnNull(out_url, hr);
    *out_url = ::SysAllocString(ret.c_str()); // TODO leak
    break;
  }

  return hr;
}


/**
 * Method: NativeExtensions::xhr
 *
 * @param method
 * @param url
 * @param data
 * @param success
 * @param error
 */
STDMETHODIMP CNativeExtensions::xhr(BSTR method, BSTR url, BSTR data, BSTR contentType, BSTR headers, IDispatch *success, IDispatch *error)
{
  // TODO headers
  HRESULT hr = S_OK;
  CComObject<BindStatusCallback> *bindStatusCallback = nullptr;
  for (;;) {
    logger->debug(L"NativeExtensions::xhr "
      L" -> " + wstring(method) +
      L" -> " + wstring(url) +
      L" -> " + wstring_limit(data ? data : L"null") +
      L" -> " + wstring(contentType ? contentType : L"null") +
      L" -> " + wstring(headers ? headers : L"null") +
      L" -> " + boost::lexical_cast<wstring>(success)+
      L" -> " + boost::lexical_cast<wstring>(error));

    hr = CComObject<BindStatusCallback>::CreateInstance(&bindStatusCallback);
    BreakOnFailed(hr);
    BreakOnNull(bindStatusCallback, hr);

    hr = bindStatusCallback->Async(new HTTP(method, url, data, contentType, headers, success, error),
      (BindStatusCallback::ATL_PDATAAVAILABLE1)&HTTP::OnData,
      method,
      url,
      data,
      contentType,
      NULL, false);
    break;
  }

  return hr;
}


/**
 * Method: NativeExtensions::notification
 *
 * @param icon
 * @param title
 * @param text
 */
STDMETHODIMP CNativeExtensions::notification(BSTR icon, BSTR title, BSTR text, BOOL *out_success)
{
  HRESULT hr = S_OK;
  bfs::wpath path = _AtlModule.modulePath / L"forge\\graphics\\icon16.png";

  for (;;) {
    logger->debug(L"NativeExtensions::notification "
      L" -> " + wstring(icon) +
      L" -> " + wstring(title) +
      L" -> " + wstring(text));
    // TODO if permission check fails -> *out_success = FALSE
    NotificationWindow::Notification(path.wstring(), title, text);
    BreakOnNull(out_success, hr);
    *out_success = TRUE;
    break;
  }

  return hr;
}

STDMETHODIMP CNativeExtensions::cookies_get(BSTR url, BSTR name, IDispatch *success, IDispatch *error)
{
  wstring w_url = wstring(url);
  wstring w_name = wstring(name);
  logger->debug(L"NativeExtensions::cookies_get -> " + w_url + L" -> " + w_name);

  HRESULT hr = S_OK;

  if (tabId == 0) {
    std::vector<TCHAR> cookieData;
    DWORD dwSize = 0;

    if (!InternetGetCookieEx(w_url.c_str(), w_name.empty() ? NULL : w_name.c_str(), NULL, &dwSize, 0, NULL)) {
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        cookieData.resize(dwSize, 0);
        // Try the call again.
        if (InternetGetCookieEx(w_url.c_str(), w_name.empty() ? NULL : w_name.c_str(), &cookieData[0], &dwSize, 0, NULL)) {
          CComVariant param2(&cookieData[0]);
          hr = CComDispatchDriver(success).Invoke1((DISPID)0, &param2);
          param2.Clear();
        }
        else if (GetLastError() == ERROR_NO_MORE_ITEMS) {
          hr = CComDispatchDriver(success).Invoke1((DISPID)0, &CComVariant(L""));
        }
        else {
          logger->error(L"NativeExtensions::cookies_get failed -> " + w_url + L" -> " + w_name);

          wstring message = L"Error code: " + std::to_wstring(GetLastError());
          hr = CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(message.c_str()));
        }
      }
      else if (GetLastError() == ERROR_NO_MORE_ITEMS) {
        hr = CComDispatchDriver(success).Invoke1((DISPID)0, &CComVariant(L""));
      }
      else {
        logger->error(L"NativeExtensions::cookies_get failed -> " + w_url + L" -> " + w_name);

        wstring message = L"Error code: " + std::to_wstring(GetLastError());
        hr = CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(message.c_str()));
      }
    }
    else {
      hr = CComDispatchDriver(success).Invoke1((DISPID)0, &CComVariant(L""));
    }
  }
  else {
    logger->error(L"NativeExtensions::cookies_get failed -> " + w_url + L" -> " + w_name);

    wstring message = L"get cookies only from background page";
    hr = CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(message.c_str()));
  }

  return hr;
}

STDMETHODIMP CNativeExtensions::cookies_remove(BSTR url, BSTR name, BOOL *out_success)
{
  wstring w_url = wstring(url);
  wstring w_name = wstring(name);

  HRESULT hr = S_OK;

  for (;;) {
    logger->debug(L"NativeExtensions::cookies_remove -> " + w_url + L" -> " + w_name);
    BreakOnNull(out_success, hr);

    if (tabId == 0) {
      wstring newCookieData = w_name + L"=; expires = Sat,01-Jan-2000 00:00:00 GMT";
      *out_success = InternetSetCookie(w_url.c_str(), NULL, newCookieData.c_str()) ? TRUE : FALSE;
    }
    else {
      logger->error(L"NativeExtensions::cookies_remove failed: tabId != 0 -> " + w_url + L" -> " + w_name);
      *out_success = FALSE;
    }

    break;
  }

  return hr;
}
