#include "stdafx.h"
#include "NativeMessaging.h"


/**
 * Construction
 */
CNativeMessaging::CNativeMessaging()
{
}

/**
 * Destruction
 */
CNativeMessaging::~CNativeMessaging()
{
  logger->debug(L"CNativeMessaging::~CNativeMessaging");
}

HRESULT CNativeMessaging::FinalConstruct()
{
  return S_OK;
}

void CNativeMessaging::FinalRelease()
{
  logger->debug(L"CNativeMessaging::FinalRelease");
}

/**
 * InterfaceSupportsErrorInfo
 */
STDMETHODIMP CNativeMessaging::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* const arr[] = { &IID_INativeMessaging };

  for (auto iid : arr)
    if (InlineIsEqualGUID(*iid, riid))
      return S_OK;

  return S_FALSE;
}

/**
 * Method: NativeMessaging::tabs_set
 *
 * Called by the browser whenever the active tab changes
 *
 * @param uuid
 * @param tabInfo
 */
STDMETHODIMP CNativeMessaging::tabs_set(BSTR uuid, UINT instanceId, BSTR url, BSTR title, BOOL focused)
{
  if (focused) {
    m_activeTab.id = instanceId;
    m_activeTab.url = url;
    m_activeTab.title = title;
    m_activeTab.selected = focused ? true : false;
  }

  // TODO save tab info

  return S_OK;
}

/**
 * Method: NativeMessaging::tabs_active
 *
 * @param uuid
 *
 * @returns Tab
 */
STDMETHODIMP CNativeMessaging::tabs_active(BSTR uuid, IDispatch *callback, UINT *out_tabId)
{
  HRESULT hr = S_OK;
  CComPtr<ITypeInfo> tabT = nullptr;
  CComPtr<IUnknown>  tabI = nullptr;
  for (;;) {
    BreakOnNull(out_tabId, hr);
    *out_tabId = m_activeTab.id;

    BreakOnNull(callback, hr);

    hr = ::CreateDispTypeInfo(&Tab::Interface, LOCALE_SYSTEM_DEFAULT, &tabT);
    BreakOnFailed(hr);
    BreakOnNull(tabT, hr);

    hr = ::CreateStdDispatch(NULL, &m_activeTab, tabT, &tabI);
    BreakOnFailed(hr);
    BreakOnNull(tabI, hr);

    hr = CComDispatchDriver(callback).Invoke1((DISPID)0, &CComVariant(tabI));
    break;
  }

  return hr;
}

/**
 * Method: NativeMessaging::load
 *
 * @param uuid
 * @param instance
 */
STDMETHODIMP CNativeMessaging::load(BSTR uuid, unsigned int instanceId)
{
  m_clients[uuid].insert(instanceId);
  return S_OK;
}

/**
 * Method: NativeMessaging::unload
 *
 * @param uuid
 * @param instance
 */
struct delete_callback 
{
  delete_callback(UINT tabId) : tabId(tabId) {}
  UINT tabId;
  bool operator()(Callback::pointer callback) const {
    logger->debug(L"CNativeMessaging::unload checking callback "
      L" -> " + boost::lexical_cast<wstring>(tabId)+
      L" -> " + callback->toString());
    if (callback->tabId == tabId) {
      logger->debug(L"\terased");
      return true;
    }
    return false;
  }
};

STDMETHODIMP CNativeMessaging::unload(BSTR uuid, unsigned int instanceId)
{
  logger->debug(L"CNativeMessaging::unload -> " + wstring(uuid) + L" -> " + boost::lexical_cast<wstring>(instanceId));

  // clean up any callbacks registered for this instance
  auto& v = fg_callbacks[uuid];
  v.erase(std::remove_if(v.begin(), v.end(), delete_callback(instanceId)), v.end());

  m_clients[uuid].erase(instanceId);
  if (m_clients[uuid].empty()) {
    logger->debug(L"CNativeMessaging::unload shutting down extension -> " + wstring(uuid));
    bg_callbacks.erase(uuid);
    fg_callbacks.erase(uuid);
    m_clients.erase(uuid);
    logger->debug(L"CNativeMessaging::unload done");
  }

  logger->debug(L"CNativeMessaging::unload clients remaining: " + boost::lexical_cast<wstring>(m_clients.size()));

  if (m_clients.empty())
    return shutdown();

  return S_OK;
}

/**
 * Helper: shutdown
 */
HRESULT CNativeMessaging::shutdown()
{
  logger->debug(L"CNativeMessaging::shutdown -> " + boost::lexical_cast<wstring>(m_dwRef));
  bg_callbacks.clear();
  fg_callbacks.clear();
  return S_OK;
}

/**
 * Method: fg_listen
 *
 * FG <= BG
 */
STDMETHODIMP CNativeMessaging::fg_listen(BSTR _uuid, UINT tabId, BSTR _type, IDispatch *callback, IDispatch *error)
{
  wstring uuid(_uuid), type(_type);
  fg_callbacks[uuid].push_back(Callback::pointer(new Callback(tabId, type, callback, error)));
  return S_OK;
}

/**
 * Method: fg_broadcast
 *
 * FG => FG(*)
 */
STDMETHODIMP CNativeMessaging::fg_broadcast(BSTR _uuid, BSTR _type, BSTR _content, IDispatch *callback, IDispatch *error)
{
  wstring uuid(_uuid), type(_type), content(_content);

  HRESULT hr;
  auto& v = fg_callbacks[uuid];
  for (auto i : v)
    if (i->type == L"*" || i->type == type)
      hr = i->Dispatch(content, callback);

  return S_OK;
}

/**
 * Method: fg_toFocussed
 *
 * FG => FG(1)
 */
STDMETHODIMP CNativeMessaging::fg_toFocussed(BSTR _uuid, BSTR _type, BSTR _content, IDispatch *callback, IDispatch *error)
{
  wstring uuid(_uuid), type(_type), content(_content);

  HRESULT hr;
  auto& v = fg_callbacks[uuid];
  for (auto i : v) {
    if (i->tabId != m_activeTab.id)
      continue;
    if (i->type == L"*" || i->type == type)
      hr = i->Dispatch(content, callback);
  }

  return S_OK;
}

/**
 * Method: fg_broadcastBackround
 *
 * FG => BG
 */
STDMETHODIMP CNativeMessaging::fg_broadcastBackground(BSTR _uuid, BSTR _type, BSTR _content, IDispatch *callback, IDispatch *error)
{
  wstring uuid(_uuid), type(_type), content(_content);

  HRESULT hr;
  auto& v = bg_callbacks[uuid];
  for (auto i : v) {
    if (i->type == L"*" && type == L"bridge")
      continue;
    if (i->type == L"*" || i->type == type)
      hr = i->Dispatch(content, callback);
  }

  return S_OK;
}

/**
 * Method: bg_listen -> type -> (String -> (String))
 *
 * BG <= FG
 */
STDMETHODIMP CNativeMessaging::bg_listen(BSTR _uuid, BSTR _type, IDispatch *callback, IDispatch *error)
{
  wstring uuid(_uuid), type(_type);
  bg_callbacks[uuid].push_back(Callback::pointer(new Callback(type, callback, error)));

  return S_OK;
}

/**
 * Method: bg_broadcast
 *
 * BG => FG(*)
 */
STDMETHODIMP CNativeMessaging::bg_broadcast(BSTR _uuid, BSTR _type, BSTR _content, IDispatch *callback, IDispatch *error)
{
  wstring uuid(_uuid), type(_type), content(_content);
  HRESULT hr;
  auto& v = fg_callbacks[uuid];
  for (auto i : v)
    if (i->type == L"*" || i->type == type)
      hr = i->Dispatch(content, callback);

  return S_OK;
}

/**
 * Method: bg_toFocussed
 *
 * BG => FG(1)
 */
STDMETHODIMP CNativeMessaging::bg_toFocussed(BSTR _uuid, BSTR _type, BSTR _content, IDispatch *callback, IDispatch *error)
{
  wstring uuid(_uuid), type(_type), content(_content);
  HRESULT hr;
  auto& v = fg_callbacks[uuid];
  for (auto i : v)
    if (i->type == L"*" || i->type == type)
      hr = i->Dispatch(content, callback);

  return S_OK;
}
