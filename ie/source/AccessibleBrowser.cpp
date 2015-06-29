#include "stdafx.h"
#include "AccessibleBrowser.h"
#include <SHLGUID.h>  /* for SID_SWebBrowserApp */

/**
 * Construction: Accessible
 */
Accessible::Accessible(HWND hwnd, long id) : id (id)
{  
  HRESULT hr = ::AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, IID_IAccessible, (void**)&iaccessible);
  if (FAILED(hr) || !iaccessible)
    logger->warn(L"Accessible::Accessible invalid hwnd");
}

Accessible::Accessible(IDispatch *dispatch, long id) : iaccessible(CComQIPtr<IAccessible, &IID_IAccessible>(dispatch)), id(id)
{
  if (!iaccessible)
    logger->warn(L"Accessible::Accessible invalid dispatch");
}

Accessible::Accessible(IAccessible *iaccessible, long id) : iaccessible(iaccessible), id(id)
{
}

/** 
 * Destruction: Accessible
 */
Accessible::~Accessible()
{
}

/**
 * Accessible::children
 */
Accessible::vector Accessible::children() 
{
  HRESULT hr = S_OK;
  Accessible::vector ret;
  std::vector<CComVariant> accessors;

  long count = 0; // children count
  long countObtained = 0;

  for (;;) {
    BreakOnNullWithErrorLog(iaccessible, L"Accessible::children invalid IAccessible");

    hr = iaccessible->get_accChildCount(&count);
    BreakOnFailedWithErrorLog(hr, L"Accessible::children failed to get child count -> " + logger->parse(hr));

    // get accessors
    accessors.resize(count);
    hr = ::AccessibleChildren(iaccessible, 0, count, &accessors[0], &countObtained);
    BreakOnFailedWithErrorLog(hr, L"Accessible::children failed to get accessors -> " + logger->parse(hr));

    // iterate through accessors
    for (long n = 0; n < countObtained; ++n) {
      auto& v = accessors[n];
      if (v.vt != VT_DISPATCH)
        continue;
      
      auto accessor = Accessible::pointer(new Accessible(v.pdispVal, n + 1));

      CComBSTR name = nullptr;
      accessor->iaccessible->get_accName(CComVariant(CHILDID_SELF), &name);
      if (name) 
        logger->debug(L"Accessible::children -> " + wstring(name));

      ret.push_back(accessor);
    }

    break;
  }

  return ret;
}

/**
 * Construction: AccessibleBrowser
 */
AccessibleBrowser::AccessibleBrowser(HWND hwnd) : m_hwnd(hwnd)
{
}

/** 
 * Destruction: AccessibleBrowser
 */
AccessibleBrowser::~AccessibleBrowser()
{
}

/**
 * AccessibleBrowser::tabs
 */
wstringvector AccessibleBrowser::tabs()
{
  HRESULT hr = S_OK;
  HWND    hwnd = nullptr;
  CComPtr<IAccessible> iaccessible = nullptr;
  wstringvector ret;

  for (;;) {
    // get IEFrame HWND if necessary
    if (m_hwnd == NULL) {
      logger->debug(L"AccessibleBrowser::tabs calling EnumWindows for IE hwnd");
      ::EnumWindows(AccessibleBrowser::EnumWndProc, (LPARAM)&m_hwnd);
      BreakOnNullWithErrorLog(m_hwnd, L"AccessibleBrowser::tabs could not get active tab");
    }

    logger->debug(L"AccessibleBrowser::tabs active tab -> " + boost::lexical_cast<wstring>(m_hwnd));
    
    // Get DirectUIHWND for IEFrame
    // TODO test on IE9
    hwnd = ::FindWindowEx(m_hwnd, NULL, L"CommandBarClass", NULL);
    BreakOnNullWithErrorLog(hwnd, L"AccessibleBrowser::tabs CommandBarClass failed");

    hwnd = ::FindWindowEx(hwnd, NULL, L"ReBarWindow32", NULL);
    BreakOnNullWithErrorLog(hwnd, L"AccessibleBrowser::tabs ReBarWindow32 failed");

    hwnd = ::FindWindowEx(hwnd, NULL, L"TabBandClass", NULL);
    BreakOnNullWithErrorLog(hwnd, L"AccessibleBrowser::tabs TabBandClass failed");

    hwnd = ::FindWindowEx(hwnd, NULL, L"DirectUIHWND", NULL);
    BreakOnNullWithErrorLog(hwnd, L"AccessibleBrowser::tabs DirectUIHWND failed");
    
    // get IAccessible for IE
    hr = ::AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, IID_IAccessible, (void**)&iaccessible);
    BreakOnNull(iaccessible, hr);
    BreakOnFailed(hr);
    
    // iterate through accessors
    auto& accessors = Accessible(iaccessible).children();
    for (auto& accessor : accessors) {
      auto& children = accessor->children();
      for (auto& child : children) {
        auto& tabs = child->children();
        for (auto& tab : tabs) {
          CComBSTR name = nullptr;
          hr = tab->iaccessible->get_accName(CComVariant(CHILDID_SELF), &name);
          if (FAILED(hr) || !name) {
            logger->debug(L"AccessibleBrowser::tabs could not get tab name -> " + boost::lexical_cast<wstring>(tab->id) + L" -> " + logger->parse(hr));
            continue;
          }
          logger->debug(L"AccessibleBrowser::tabs -> " + boost::lexical_cast<wstring>(tab->id) + L" -> " + wstring(name));

          ret.push_back(wstring(name));
        }
      }
    }

    break;
  }

  if (FAILED(hr))
    logger->error(L" AccessibleBrowser::tabs() failed -> " + logger->parse(hr));

  return ret;
}

/**
 * Helper: EnumWndProc
 */
BOOL CALLBACK AccessibleBrowser::EnumWndProc(HWND hwnd, LPARAM param)
{
  wstring windowtext;
  {
    wchar_t buf[MAX_PATH] = { 0 };
    ::GetWindowText(hwnd, buf, MAX_PATH);
    windowtext = buf;
  }

  wstring windowclass;
  {
    wchar_t buf[MAX_PATH] = { 0 };
    ::RealGetWindowClass(hwnd, buf, MAX_PATH);
    windowclass = buf;
  }

  if (windowclass == L"IEFrame") {
    logger->debug(L"AccessibleBrowser::EnumWndProc "
      L" -> " + boost::lexical_cast<wstring>(hwnd)+
      L" -> " + windowtext +
      L" -> " + windowclass);
    *(HWND*)param = hwnd;
    return FALSE;
  }
  return TRUE;
}

/**
 * Helper::EnumChildWndProc
 */
BOOL CALLBACK AccessibleBrowser::EnumChildWndProc(HWND hwnd, LPARAM param)
{
  wstring windowtext, windowclass;
  wchar_t buf[MAX_PATH] = { 0 };
  ::GetClassName(hwnd, buf, MAX_PATH);
  windowclass = buf;
  logger->debug(L"AccessibleBrowser::EnumChildWndProc "
    L" -> " + boost::lexical_cast<wstring>(hwnd)+
    L" -> " + windowtext +
    L" -> " + windowclass);
  if (windowclass == L"Internet Explorer_Server") {
    *(HWND*)param = hwnd;
    return FALSE;
  }
  return TRUE;
}


/**
 * Utility: active
 */

HRESULT AccessibleBrowser::active(IWebBrowser2 **webBrowser2) 
{
  logger->debug(L"AccessibleBrowser::active");

  HINSTANCE instance = nullptr;
  HWND ieframe = nullptr;
  HWND hwnd = nullptr;

  CComPtr<IHTMLDocument2> htmlDocument2 = nullptr;
 
  HRESULT hr = E_FAIL;

  for (;;) {
    instance = ::LoadLibrary(L"OLEACC.DLL");
    BreakOnNullWithErrorLog(instance, L"AccessibleBrowser::active could not load OLEACC.DLL");
    
    // get "IEFrame"
    // gets into a race condition with the rest of the startup code
    //hr = ::EnumWindows(AccessibleBrowser::EnumWndProc, (LPARAM)&ieframe);
    ieframe = ::FindWindowEx(NULL, NULL, L"IEFrame", NULL);
    BreakOnNullWithErrorLog(ieframe, L"AccessibleBrowser::active could not get IEFrame");

    // get "Internet Explorer_Server"
    hwnd = ieframe;
    // gets into a race condition with the rest of the startup code
    //hr = ::EnumChildWindows(ieframe, AccessibleBrowser::EnumChildWndProc, (LPARAM)&hwnd);
    hwnd = ::FindWindowEx(hwnd, NULL, L"Frame Tab", NULL);
    if (!hwnd) {
      // In IE7 there is no intermediate "Frame Tab" window. If we didn't find
      // one try getting TabWindowClass directly from under ieframe.
      hwnd = ieframe;
    }

    hwnd = ::FindWindowEx(hwnd, NULL, L"TabWindowClass", NULL);
    BreakOnNullWithErrorLog(hwnd, L"AccessibleBrowser::active failed: TabWindowClass");

    hwnd = ::FindWindowEx(hwnd, NULL, L"Shell DocObject View", NULL);
    BreakOnNullWithErrorLog(hwnd, L"AccessibleBrowser::active failed: Shell DocObject View");

    hwnd = ::FindWindowEx(hwnd, NULL, L"Internet Explorer_Server", NULL);
    BreakOnNullWithErrorLog(hwnd, L"AccessibleBrowser::active could not get Internet Explorer_Server");

    UINT msg = ::RegisterWindowMessage(L"WM_HTML_GETOBJECT");

    LRESULT result;
    LRESULT ret = ::SendMessageTimeout(hwnd, msg, 0L, 0L, SMTO_ABORTIFHUNG, 1000, reinterpret_cast<PDWORD_PTR>(&result));
    if (!ret || !result) {
      wchar_t *buf = nullptr;
      DWORD error = GetLastError();
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);

      std::wstring sbuf(L"unknown error");
      if (buf) {
        sbuf = std::wstring(buf);
        ::LocalFree(buf);
      }

      logger->error(L"AccessibleBrowser::active WM_HTML_GETOBJECT failed -> " + sbuf +
        L" -> " + boost::lexical_cast<wstring>(msg)+
        L" -> " + boost::lexical_cast<wstring>(result));

      break;
    }

    LPFNOBJECTFROMLRESULT object = reinterpret_cast<LPFNOBJECTFROMLRESULT>(::GetProcAddress(instance, "ObjectFromLresult"));
    BreakOnNullWithErrorLog(object, L"AccessibleBrowser::active could not get HTML Object");

    hr = (*object)(result, IID_IHTMLDocument, 0, reinterpret_cast<void**>(&htmlDocument2));
    BreakOnFailedWithErrorLog(hr, L"AccessibleBrowser::active could not get IHTMLDocument -> " + logger->parse(hr));

    CComQIPtr<IServiceProvider> serviceProvider(htmlDocument2);
    BreakOnNullWithErrorLog(serviceProvider, L"AccessibleBrowser::active could not get IServiceProvider");

    hr = serviceProvider->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, reinterpret_cast<void**>(webBrowser2));
    if (FAILED(hr))
      logger->error(L"AccessibleBrowser::active could not get IWebBrowser2 -> " + logger->parse(hr));

    break;
  }

  if (instance)
    ::FreeLibrary(instance);

  return hr;
}

/**
 * Type: NativeAccessible
 */
PARAMDATA NativeAccessible::_open [] = { 
    { L"bstr",     VT_BSTR }, 
    { L"bool",     VT_BOOL },
    { L"dispatch", VT_DISPATCH },
    { L"dispatch", VT_DISPATCH }
};
PARAMDATA NativeAccessible::_dispatch = { L"dispatch", VT_DISPATCH };
METHODDATA NativeAccessible::methods[] = {
    { L"open",         NativeAccessible::_open,      1, 0, CC_STDCALL, 4, DISPATCH_METHOD, VT_EMPTY },
    { L"closeCurrent", &NativeAccessible::_dispatch, 2, 1, CC_STDCALL, 1, DISPATCH_METHOD, VT_EMPTY },
};

INTERFACEDATA NativeAccessible::Interface = { 
  NativeAccessible::methods, _countof(methods) 
};


/**
 * Method: NativeAccessible::open
 *
 * @param url
 * @param selected
 * @param success
 * @param error
 */
void __stdcall NativeAccessible::open(BSTR url, VARIANT_BOOL selected, IDispatch *success, IDispatch *error)
{
  logger->debug(L"NativeAccessible::open -> " + wstring(url) +
    L" -> " + boost::lexical_cast<wstring>(selected == -1) +
    L" -> " + boost::lexical_cast<wstring>(success)+
    L" -> " + boost::lexical_cast<wstring>(error));

  HRESULT hr = S_OK;
  CComPtr<IWebBrowser2> webBrowser2 = nullptr;

  for (;;) {
    hr = AccessibleBrowser().active(&webBrowser2);
    if (FAILED(hr)) {
      logger->debug(L"NativeAccessible::open failed  -> " + boost::lexical_cast<wstring>(hr) + L" -> " + wstring(url) + L" -> " + logger->parse(hr));
      CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(L"failed to get active window"));
      break;
    }

    BrowserNavConstants flags = (selected == -1 ? navOpenInBackgroundTab : navOpenInNewTab);
    hr = webBrowser2->Navigate2(&CComVariant(url), &CComVariant(flags, VT_I4), &CComVariant(NULL), &CComVariant(NULL), &CComVariant(NULL));
    if (FAILED(hr)) {
      logger->debug(L"NativeAccessible::open failed -> " + boost::lexical_cast<wstring>(hr) + L" -> " + wstring(url) + L" -> " + logger->parse(hr));
      CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(L"failed to open tab"));
      break;
    }

    hr = CComDispatchDriver(success).Invoke0((DISPID)0);
    if (FAILED(hr))
      logger->debug(L"CNativeExtensions::tabs_open failed to invoke success callback -> " + wstring(url) + L" -> " + logger->parse(hr));

    break;
  }
}

/** 
 * NativeAccessible::closeCurrent
 * @param error
 */
void __stdcall NativeAccessible::closeCurrent(IDispatch *error) 
{
  logger->debug(L"NativeAccessible::closeCurrent -> " + boost::lexical_cast<wstring>(error));
  HRESULT hr = m_webBrowser2->Quit();
  if (FAILED(hr)) {
    logger->debug(L"NativeAccessible::closeCurrent failed to close current tab -> " + logger->parse(hr));
    CComDispatchDriver(error).Invoke1((DISPID)0, &CComVariant(L"failed to close current tab"));
  }
}
