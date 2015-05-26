#include "stdafx.h"
#include "BrowserControl.h"
#include <HTMLDocument.h>

#include "PopupWindow.h"

/**
 * LifeCycle: Constructor
 */
BrowserControl::BrowserControl(CWindow *parent, const wstring& uuid, bool resizeParent, bool subclass)
    : parent(parent),
      uuid(uuid),
      resizeParent(resizeParent),
      subclass(subclass), m_subclasser(NULL),
      bgcolor(255,255,255)
{
}


/**
 * LifeCycle: Destructor
 */
BrowserControl::~BrowserControl()
{
  HRESULT hr = S_OK;
  logger->debug(L"BrowserControl::~BrowserControl");

  if (subclass && m_subclasser)
    delete m_subclasser;

  hr = EasyUnadvise();
  if (FAILED(hr))
    logger->warn(L"BrowserControl::~BrowserControl failed to unadvise -> " + logger->parse(hr));

  // NativeMessaging
  logger->debug(L"BrowserControl::~BrowserControl release NativeMessaging");
  if (m_nativeMessaging) {
    hr = m_nativeMessaging->unload(CComBSTR(this->uuid.c_str()), 0);
    if (FAILED(hr))
      logger->debug(L"BrowserControl::~BrowserControl failed to release NativeMessaging -> " + logger->parse(hr));
  }

  // NativeControls
  logger->debug(L"BrowserControl::~BrowserControl release NativeControls");
  if (m_nativeControls) {
    hr = m_nativeControls->unload(CComBSTR(this->uuid.c_str()), 0);
    if (FAILED(hr))
      logger->debug(L"BrowserControl::~BrowserControl failed to release NativeControls  -> " + logger->parse(hr));
  }

  // BrowserControl
  if (!DestroyWindow())
    logger->warn(L"BrowserControl::~BrowserControl failed to destroy window");
  
  m_hWnd = NULL;
  logger->debug(L"BrowserControl::~BrowserControl done");
}


/** 
 * Message: On
 */
LRESULT BrowserControl::OnCreate(UINT msg, WPARAM wparam, LPARAM lparam, BOOL &handled) 
{  
  LRESULT lr = DefWindowProc(msg, wparam, lparam);
  AttachControl(true);
  return lr;
}


/** 
 * Event: BrowserControl::OnBeforeNavigate2
 */
void __stdcall BrowserControl::OnBeforeNavigate2(IDispatch *idispatch, 
                                                 VARIANT *url, 
                                                 VARIANT *flags, 
                                                 VARIANT *target, 
                                                 VARIANT *postData, 
                                                 VARIANT *headers, 
                                                 VARIANT_BOOL *cancel)
{
}


/** 
 * Event: BrowserControl::OnNavigateComplete2
 */
void __stdcall BrowserControl::OnNavigateComplete2(IDispatch *idispatch, VARIANT *url)
{
  HRESULT hr = S_OK;
  wstring location;

  CComPtr<IDispatch> disp = nullptr;
  CComQIPtr<IHTMLWindow2, &IID_IHTMLWindow2> htmlWindow2 = nullptr;

  for (;;) {
    BreakOnNull(url, hr);
    logger->debug(L"BrowserControl::OnNavigateComplete2 -> " + wstring(url->bstrVal));

    // Do nothing for blank pages 
    location = wstring(url->bstrVal);
    if (location.find(L"about:") == 0) {
      logger->debug(L"BrowserControl::OnNavigateComplete2 boring url");
      break;
    }

    // Get me some fiddlies
    CComQIPtr<IWebBrowser2, &IID_IWebBrowser2> webBrowser2(idispatch);
    if (!webBrowser2) {
      logger->error(L"BrowserControl::OnNavigateComplete2 no valid IWebBrowser2");
      break;
    }

    webBrowser2->get_Document(&disp);
    if (!disp) {
      logger->error(L"BrowserControl::OnNavigateComplete2 get_Document failed");
      break;
    }

    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
    if (!htmlDocument2) {
      logger->error(L"BrowserControl::OnNavigateComplete2 IHTMLDocument2 failed");
      break;
    }

    htmlDocument2->get_parentWindow(&htmlWindow2);
    if (!htmlWindow2) {
      logger->error(L"BrowserControl::OnNavigateComplete2 IHTMLWindow2 failed");
      break;
    }

    CComQIPtr<IDispatchEx> htmlWindow2Ex(htmlWindow2);
    if (!htmlWindow2Ex) {
      logger->error(L"BrowserControl::OnNavigateComplete2 IHTMLWindow2Ex failed");
      break;
    }

    // Attach NativeAccessible (forge.tabs.*)
    if (m_nativeAccessible) {
      logger->error(L"BrowserControl::OnNavigateComplete2 resetting nativeAccessible");
      m_nativeAccessible.reset();
    }

    m_nativeAccessible = NativeAccessible::pointer(new NativeAccessible(webBrowser2));
    hr = Attach::NativeTabs(htmlWindow2Ex, L"accessible", m_nativeAccessible.get());
    if (FAILED(hr)) {
      logger->error(L"BrowserControl::OnNavigateComplete2 failed to attach NativeAccessible -> " + logger->parse(hr));
      break;
    }

    // Attach NativeExtensions, tabId 0 is BG Page
    hr = Attach::NativeExtensions(uuid, htmlWindow2Ex, L"extensions", 0, location, &m_nativeExtensions.p);
    if (FAILED(hr)) {
      logger->error(L"BrowserControl::OnNavigateComplete2 failed to attach NativeExtensions -> " + logger->parse(hr));
      break;
    }

    // Attach NativeMessaging, instanceId == 0 is a console window
    hr = Attach::NativeMessaging(uuid, htmlWindow2Ex, L"messaging", 0, &m_nativeMessaging.p);
    if (FAILED(hr)) {
      logger->error(L"BrowserControl::OnNavigateComplete2 failed to attach NativeMessaging -> " + logger->parse(hr));
      break;
    }

    // Attach NativeControls, instanceId == 0 is a console window
    hr = Attach::NativeControls(uuid, htmlWindow2Ex, L"controls", 0, &m_nativeControls.p);
    if (FAILED(hr))
      logger->error(L"BrowserControl::OnNavigateComplete2 failed to attach NativeControls -> " + logger->parse(hr));

    break;
  }
}

/**
 * Event: OnDocumentComplete
 */
void __stdcall BrowserControl::OnDocumentComplete(IDispatch *idispatch, VARIANT *url)
{
  HRESULT hr = S_OK;
  CComPtr<IDispatch> disp = nullptr;
  CComPtr<IHTMLElement> htmlElement = nullptr;

  for (;;) {
    logger->debug(L"BrowserControl::OnDocumentComplete -> " + wstring(url->bstrVal ? url->bstrVal : L"NIL"));
    // Get me some fiddlies
    CComQIPtr<IWebBrowser2, &IID_IWebBrowser2> webBrowser2(idispatch);
    if (!webBrowser2) {
      logger->error(L"BrowserControl::OnDocumentComplete no valid IWebBrowser2");
      break;
    }

    webBrowser2->get_Document(&disp);
    if (!disp) {
      logger->error(L"BrowserControl::OnDocumentComplete get_Document failed");
      break;
    }

    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
    if (!htmlDocument2) {
      logger->error(L"BrowserControl::OnDocumentComplete IHTMLDocument2 failed");
      break;
    }

    // subclass if necessary
    if (subclass && !m_subclasser) {
      HWND hwnd = m_hWnd;
      while ((hwnd = ::GetWindow(hwnd, GW_CHILD)) != NULL) { // TODO use ::EnumChildWindows
        wchar_t buf[MAX_PATH] = { 0 };
        ::GetClassName(hwnd, buf, MAX_PATH);
        if (wstring(buf) == L"Internet Explorer_Server") {
          m_subclasser = new Subclasser(this->parent); // parent is PopupWindow
          if (m_subclasser->SubclassWindow(hwnd)) {
            m_subclasser->hwnd_ie = hwnd;
          }
          else {
            logger->error(L"BrowserControl::OnDocumentComplete failed to subclass IE");
          }
          break;
        }
      }
    }

    // resize parent if needed
    if (!resizeParent)
      break;
    
    // Get page size
    hr = htmlDocument2->get_body(&htmlElement);
    BreakOnFailed(hr);
    BreakOnNull(htmlElement, hr);
    
    CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> htmlElement2(htmlElement);
    BreakOnNull(htmlElement2, hr);

    long width, height;
    htmlElement2->get_scrollWidth(&width);
    htmlElement2->get_scrollHeight(&height);
    width = min(800, width);       // approx chrome's maximum size
    height = min(500, height);      // 

    // Get page color
    CComVariant v;
    hr = htmlDocument2->get_bgColor(&v);
    if (FAILED(hr)) {
      logger->debug(L"BrowserControl::OnDocumentComplete failed to get background color");
      bgcolor = Gdiplus::Color(255, 255, 255);
    } else {
      wstring colorstring = wstring(v.bstrVal).substr(1);
      DWORD rgb = std::wcstoul(colorstring.c_str(), NULL, 16);
      bgcolor = Gdiplus::Color(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
    }

    // resize popup
    PopupWindow* popup = static_cast<PopupWindow*>(parent);
    width  += 2; // We're 2 pixels smaller than the parent
    height += ((PopupWindow::DEFAULT_INSET * 2) + 1);
    int origin = buttonPosition.x;
    
    if (popup->alignment == PopupWindow::left) {
      origin = origin - (width - (PopupWindow::TAB_SIZE * 3)) - 3;
    }
    else {
      origin = origin - (PopupWindow::TAB_SIZE * 1) - 3;
    }

    parent->MoveWindow(origin, buttonPosition.y, width, height, TRUE);
    break;
  }
}


/**
 * Event: OnQuit
 */
void __stdcall BrowserControl::OnQuit()
{
  logger->debug(L"BrowserControl::OnQuit");
}


/**
 * Helper: BrowserControl::SetContent
 */
HRESULT BrowserControl::SetContent(const wstringpointer& content)
{
  HRESULT hr = S_OK;
  // gumph
  VARIANT*   param = nullptr;
  SAFEARRAY* sfArray = nullptr;
  BSTR       _content = nullptr;
  CComPtr<IDispatch> disp = nullptr; 
  
  for (;;) {
    BreakOnNull(content, hr);
    logger->debug(L"BrowserControl::SetContent -> " + wstring_limit(*content));
    hr = (*this)->get_Document(&disp);
    BreakOnFailed(hr);
    BreakOnNull(disp, hr);

    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> document(disp);
    BreakOnNull(document, hr);

    sfArray = ::SafeArrayCreateVector(VT_VARIANT, 0, 1);
    BreakOnNull(sfArray, hr);

    _content = ::SysAllocString((*content).c_str());
    BreakOnNull(_content, hr);

    hr = ::SafeArrayAccessData(sfArray, (LPVOID*)&param);
    BreakOnFailed(hr);
    BreakOnNull(param, hr);

    param->vt = VT_BSTR;
    param->bstrVal = _content;

    hr = ::SafeArrayUnaccessData(sfArray);
    BreakOnFailed(hr);

    // clear current data
    hr = document->clear();
    BreakOnFailed(hr);

    // write it
    hr = document->write(sfArray);
    BreakOnFailed(hr);

    // close stream
    hr = document->close();
    break;
  }

  // clean up behind ourselves
  if (_content)
    ::SysFreeString(_content);

  if (sfArray)
    ::SafeArrayDestroy(sfArray);

  return hr;
}


/**
 * Helper: AttachControl
 */
HRESULT BrowserControl::AttachControl(BOOL events) 
{
  CComPtr<IUnknown> unknown = nullptr;
  HRESULT hr = E_FAIL;
  for (;;) {
    if (!m_hWnd) {
      logger->error(L"BrowserControl::AttachControl no parent");
      break;
    }

    hr = ::AtlAxGetControl(m_hWnd, &unknown);
    if (FAILED(hr)) {
      logger->error(L"BrowserControl::AttachControl AtlAxGetControl failed -> " + logger->parse(hr));
      break;
    }

    hr = unknown->QueryInterface(__uuidof(IWebBrowser2), (void**)(CComPtr<IWebBrowser2>*)this);
    if (FAILED(hr)) {
      logger->error(L"BrowserControl::AttachControl QueryInterface failed -> " + logger->parse(hr));
      break;
    }

    if (!events) {
      hr = S_OK;
      break;
    }

    hr = EasyAdvise(unknown);
    if (FAILED(hr))
      logger->error(L"BrowserControl::AttachControl Advise failed -> " + logger->parse(hr));

    break;
  }

  return hr;
}


/**
 * Helper: EasyAdvise
 */
HRESULT BrowserControl::EasyAdvise(IUnknown* unknown) 
{
  m_unknown = unknown;
  ::AtlGetObjectSourceInterface(unknown, &m_libid, &m_iid, &m_wMajorVerNum, &m_wMinorVerNum);
  return this->DispEventAdvise(unknown, &m_iid);
}


/**
 * Helper: EasyUnadvice
 */
HRESULT BrowserControl::EasyUnadvise() 
{
  logger->debug(L"BrowserControl::EasyUnadvise");
  ::AtlGetObjectSourceInterface(m_unknown, &m_libid, &m_iid, &m_wMajorVerNum, &m_wMinorVerNum);
  return this->DispEventUnadvise(m_unknown, &m_iid);
}
