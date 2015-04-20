#include "stdafx.h"
#include "Attach.h"

/**
 * Helper: Attach::NativeBackground
 */
HRESULT Attach::NativeBackground(const wstring& uuid, 
                                 const wstring& url, 
                                 bool isVisible,
                                 INativeBackground **nativeBackground,
                                 unsigned int *instanceId)
{
    HRESULT hr;
    
    if (*nativeBackground == NULL) {
        //logger->debug(L"Attach::NativeBackground creating instance");
        hr = ::CoCreateInstance(CLSID_NativeBackground, 
                                NULL,
                                CLSCTX_LOCAL_SERVER,
                                IID_INativeBackground,
                                (LPVOID*)nativeBackground);
        if (FAILED(hr)) {
            logger->error(L"Attach::NativeBackground failed to create instance"
                          L" -> " + logger->parse(hr));
            return hr;
        }
    }
    
    // load background page
    logger->debug(L"Attach::NativeBackground loading"
                  L" -> " + url);
    hr = (*nativeBackground)->load(CComBSTR(uuid.c_str()), 
                                   CComBSTR(url.c_str()),
                                   isVisible,
                                   instanceId);
    if (FAILED(hr)) {
        logger->error(L"Attach::NativeBackground "
                      L"failed to load background page"
                      L" -> " + logger->parse(hr));
        return hr;
    }

    return S_OK;
}


/**
 * Helper: Attach::NativeExtensions
 */
HRESULT Attach::NativeExtensions(const wstring& uuid, IDispatchEx *htmlWindow2Ex, const wstring& name, unsigned int instanceId, const wstring& location, INativeExtensions **out)
{
  HRESULT hr = E_FAIL;
  DISPID dispid;
  DISPID namedArgs[] = { DISPID_PROPERTYPUT };
  DISPPARAMS params;

  for (;;) {
    if (!out)
      break;
    if (!htmlWindow2Ex)
      break;

    if (*out == nullptr) {
      logger->debug(L"Attach::NativeExtension creating instance");
      hr = ::CoCreateInstance(CLSID_NativeExtensions, NULL, CLSCTX_LOCAL_SERVER, IID_INativeExtensions, (LPVOID*)out);
      if (FAILED(hr)) {
        logger->error(L"Attach::NativeExtensions failed to create instance -> " + logger->parse(hr));
        break;
      }
    }

    // register tabId 
    hr = (*out)->set_tabId(instanceId);
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeExtensions failed to set tabId -> " + logger->parse(hr));
      break;
    }

    // register location
    hr = (*out)->set_location(CComBSTR(location.c_str()));
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeExtensions failed to set location -> " + logger->parse(hr));
      break;
    }

    // attach to DOM
    hr = htmlWindow2Ex->GetDispID(CComBSTR(name.c_str()), fdexNameEnsure, &dispid);
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeExtensions GetDispID failed -> " + logger->parse(hr));
      return hr;
    }

    params.rgvarg = new VARIANT[1];
    params.rgvarg[0].pdispVal = *out;
    params.rgvarg[0].vt = VT_DISPATCH;
    params.rgdispidNamedArgs = namedArgs;
    params.cArgs = 1;
    params.cNamedArgs = 1;
    hr = htmlWindow2Ex->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, NULL, NULL, NULL);
    if (FAILED(hr))
      logger->error(L"Attach::NativeExtensions failed to attach -> " + logger->parse(hr));

    break;
  }

  return hr;
}

/**
 * Helper: Attach::NativeMessaging
 */
HRESULT Attach::NativeMessaging(const wstring& uuid, IDispatchEx *htmlWindow2Ex, const wstring& name, unsigned int instanceId, INativeMessaging **out)
{
  HRESULT hr = E_FAIL;
  DISPID  dispid;
  DISPID namedArgs[] = { DISPID_PROPERTYPUT };
  DISPPARAMS params;

  for (;;) {
    if (!out)
      break;
    if (!htmlWindow2Ex)
      break;

    if (*out == nullptr) {
      logger->debug(L"Attach::NativeMessaging creating instance");
      hr = ::CoCreateInstance(CLSID_NativeMessaging, NULL, CLSCTX_LOCAL_SERVER, IID_INativeMessaging, (LPVOID*)out);
      if (FAILED(hr)) {
        logger->error(L"Attach::NativeMessaging failed to create instance -> " + logger->parse(hr));
        break;
      }
    }

    // register caller's instanceId w/ NativeMessaging
    hr = (*out)->load(CComBSTR(uuid.c_str()), instanceId);
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeMessaging failed to load native messaging -> " + logger->parse(hr));
      break;
    }

    // attach to DOM  
    hr = htmlWindow2Ex->GetDispID(CComBSTR(name.c_str()), fdexNameEnsure, &dispid);
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeMessaging GetDispID failed -> " + logger->parse(hr));
      break;
    }
    
    params.rgvarg = new VARIANT[1];
    params.rgvarg[0].pdispVal = *out;
    params.rgvarg[0].vt = VT_DISPATCH;
    params.rgdispidNamedArgs = namedArgs;
    params.cArgs = 1;
    params.cNamedArgs = 1;

    hr = htmlWindow2Ex->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, NULL, NULL, NULL);
    if (FAILED(hr))
      logger->error(L"Attach::NativeMessaging failed to attach -> " + logger->parse(hr));

    break;
  }

  return hr;
}

/**
 * Helper: Attach::NativeTabs
 */
HRESULT Attach::NativeTabs(IDispatchEx *htmlWindow2Ex, const wstring& name, NativeAccessible *nativeTabs)
{
  HRESULT hr = E_FAIL;
  CComPtr<ITypeInfo> nativeTabsT;
  CComPtr<IUnknown>  nativeTabsI;

  DISPID namedArgs[] = { DISPID_PROPERTYPUT };
  DISPPARAMS params;
  DISPID dispid;

  for (;;) {
    if (!htmlWindow2Ex)
      break;
    if (!nativeTabs)
      break;

    // initialize nativeTabs COM object
    hr = ::CreateDispTypeInfo(&NativeAccessible::Interface, LOCALE_SYSTEM_DEFAULT, &nativeTabsT);
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeTabs failed to create nativeTabsT -> " + logger->parse(hr));
      break;
    }

    hr = ::CreateStdDispatch(NULL, nativeTabs, nativeTabsT, &nativeTabsI);
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeTabs failed to create nativeTabsI  -> " + logger->parse(hr));
      break;
    }

    IDispatch *dispatch = CComQIPtr<IDispatch>(nativeTabsI);
    if (!dispatch) {
      logger->error(L"Attach::NativeTabs failed to create nativeTabsI dispatch -> " + logger->parse(hr));
      break;
    }

    // attach to DOM
    hr = htmlWindow2Ex->GetDispID(CComBSTR(name.c_str()), fdexNameEnsure, &dispid);
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeTabs GetDispID failed -> " + logger->parse(hr));
      break;
    }

    params.rgvarg = new VARIANT[1];
    params.rgvarg[0].pdispVal = dispatch;
    params.rgvarg[0].vt = VT_DISPATCH;
    params.rgdispidNamedArgs = namedArgs;
    params.cArgs = 1;
    params.cNamedArgs = 1;

    hr = htmlWindow2Ex->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, NULL, NULL, NULL);
    if (FAILED(hr))
      logger->error(L"Attach::NativeTabs failed to attach -> " + logger->parse(hr));

    break;
  }

  return hr;
}

/**
 * Helper: Attach::NativeControls
 */
HRESULT Attach::NativeControls(const wstring& uuid, IDispatchEx *htmlWindow2Ex, const wstring& name,  unsigned int instanceId, INativeControls **out)
{
  HRESULT hr = E_FAIL;
  DISPID dispid;
  DISPID namedArgs[] = { DISPID_PROPERTYPUT };
  DISPPARAMS params;

  for (;;) {
    if (!out)
      break;
    if (!htmlWindow2Ex)
      break;

    if (*out == nullptr) {
      logger->debug(L"Attach::NativeControls creating instance");
      hr = ::CoCreateInstance(CLSID_NativeControls, NULL, CLSCTX_LOCAL_SERVER, IID_INativeControls, (LPVOID*)out);
      if (FAILED(hr)) {
        logger->error(L"Attach::NativeControls failed to create instance -> " + logger->parse(hr));
        break;
      }
    }

    // attach to DOM
    hr = htmlWindow2Ex->GetDispID(CComBSTR(name.c_str()), fdexNameEnsure, &dispid);
    if (FAILED(hr)) {
      logger->error(L"Attach::NativeControls GetDispID failed -> " + logger->parse(hr));
      break;
    }

    params.rgvarg = new VARIANT[1];
    params.rgvarg[0].pdispVal = *out;
    params.rgvarg[0].vt = VT_DISPATCH;
    params.rgdispidNamedArgs = namedArgs;
    params.cArgs = 1;
    params.cNamedArgs = 1;

    hr = htmlWindow2Ex->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, NULL, NULL, NULL);
    if (FAILED(hr))
      logger->error(L"Attach::NativeControls failed to attach -> " + logger->parse(hr));

    break;
  }

  return hr;
}
