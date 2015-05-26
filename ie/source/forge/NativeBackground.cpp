#include "stdafx.h"
#include "NativeBackground.h"


/**
 * Construction
 */
CNativeBackground::CNativeBackground() : isVisible(FALSE), instanceCounter(1) // 0 -> background
{
}

/** 
 * Destruction
 */
CNativeBackground::~CNativeBackground()
{
  logger->debug(L"CNativeBackground::~CNativeBackground");
}

HRESULT CNativeBackground::FinalConstruct()
{
  return S_OK;
}

void CNativeBackground::FinalRelease()
{
  logger->debug(L"CNativeBackground::FinalRelease");
}


/**
 * InterfaceSupportsErrorInfo
 */
STDMETHODIMP CNativeBackground::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* const arr[] = { &IID_INativeBackground };
  
  for (auto arri : arr)
    if (InlineIsEqualGUID(*arri, riid))
      return S_OK;

  return S_FALSE;
}

/**
 * Method: NativeBackground::load
 *
 * @param uuid
 * @param instance
 * @param url
 */
STDMETHODIMP CNativeBackground::load(BSTR uuid, BSTR url, BOOL is_visible, unsigned int *instanceId)
{
  HRESULT hr = E_FAIL;
  HWND hwnd;

  for (;;) {
    if (!instanceId)
      break;

    *instanceId = this->instanceCounter++;
    m_clients[uuid].insert(*instanceId);

    logger->debug(L"CNativeBackground::load"
      L" -> " + wstring(uuid) +
      L" -> " + wstring(url) +
      L" -> " + boost::lexical_cast<wstring>(is_visible)+
      L" -> " + boost::lexical_cast<wstring>(*instanceId));

    // check if we're attached
    BrowserWindow::pointer crouchingWindow = m_crouchingWindows[uuid];
    if (crouchingWindow) {
      logger->debug(L"CNativeBackground::load already attached");
      hr = S_OK;
      break;
    }

    // create, attach & load
    crouchingWindow = BrowserWindow::pointer(new BrowserWindow(uuid, url));

    hwnd = crouchingWindow->Create(NULL, CRect(0, 0, 640, 780), uuid);
    if (!hwnd) {
      logger->debug(L"CNativeBackground::load failed to create window");
      break;
    }

    /*HRESULT hr;
    hr = crouchingWindow->Navigate(url);
    if (FAILED(hr)) {
    logger->debug(L"NativeBackground::load background page failed to load"
    L" -> " + wstring(uuid) +
    L" -> " + wstring(url) +
    L" -> " + logger->parse(hr));
    return hr;
    }*/

    // initial visibility
    crouchingWindow->ShowWindow(is_visible ? SW_SHOW : SW_HIDE);
    isVisible = is_visible;

    // save it
    m_crouchingWindows[uuid] = crouchingWindow;
    hr = S_OK;
    break;
  }

  return hr;
}

/**
 * Method: NativeBackground::unload
 *
 * @param uuid
 * @param instance
 */
STDMETHODIMP CNativeBackground::unload(BSTR uuid, unsigned int instanceId)
{
  logger->debug(L"CNativeBackground::unload -> " + wstring(uuid) + L" -> " + boost::lexical_cast<wstring>(instanceId));

  m_clients[uuid].erase(instanceId);
  if (m_clients[uuid].empty()) {
    logger->debug(L"CNativeBackground::unload shutting down console -> " + wstring(uuid));
    m_crouchingWindows.erase(uuid);
    m_clients.erase(uuid);
  }

  logger->debug(L"CNativeBackground::unload clients remaining: " + boost::lexical_cast<wstring>(m_clients.size()));

  if (m_clients.empty())
    return shutdown();
  
  return S_OK;
}


/**
 * Helper: shutdown
 */
HRESULT CNativeBackground::shutdown()
{
  logger->debug(L"CNativeBackground::shutdown -> " + boost::lexical_cast<wstring>(m_dwRef));
  return S_OK;
}


/**
 * Method: NativeBackground::visible
 *
 * @param uuid
 * @param isVisible
 */
STDMETHODIMP CNativeBackground::visible(BSTR uuid, BOOL is_visible)
{
  logger->debug(L"CNativeBackground::visible -> " + wstring(uuid) + L" -> " + boost::lexical_cast<wstring>(is_visible));
  return S_OK;
}


