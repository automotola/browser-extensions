#include "stdafx.h"
#include "HTMLDocument.h"


/**
 * Static member initialization
 */
const BSTR HTMLDocument::beforeBegin = ::SysAllocString(L"beforeBegin");
const BSTR HTMLDocument::afterBegin  = ::SysAllocString(L"afterBegin");
const BSTR HTMLDocument::beforeEnd   = ::SysAllocString(L"beforeEnd");
const BSTR HTMLDocument::afterEnd    = ::SysAllocString(L"afterEnd");
const BSTR HTMLDocument::tagHead     = ::SysAllocString(L"HEAD");
const BSTR HTMLDocument::tagScript   = ::SysAllocString(L"SCRIPT");
const BSTR HTMLDocument::tagStyle    = ::SysAllocString(L"STYLE");
const BSTR HTMLDocument::attrScriptType = ::SysAllocString(L"text/javascript");
const BSTR HTMLDocument::attrStyleType  = ::SysAllocString(L"text/css");


/**
 * Construction
 */
HTMLDocument::HTMLDocument(const CComQIPtr<IWebBrowser2, &IID_IWebBrowser2>& webBrowser2)
  : m_webBrowser2(webBrowser2)
{
  HRESULT hr = OnConnect();
  if (FAILED(hr))
    logger->error(L"HTMLDocument::HTMLDocument failed to connect");
}

/**
 * Destruction
 */
HTMLDocument::~HTMLDocument()
{
  HRESULT hr = OnDisconnect();
  if (FAILED(hr))
    logger->error(L"HTMLDocument::~HTMLDocument failed to disconnect");
}


/**
 * Event: OnConnect
 */
HRESULT HTMLDocument::OnConnect()
{
  HRESULT hr = S_OK;
  CComPtr<IDispatch> disp = nullptr;

  for (;;) {
    BreakOnNull(m_webBrowser2, hr);

    hr = m_webBrowser2->get_Document(&disp);
    BreakOnFailed(hr);
    BreakOnNull(disp, hr);

    m_htmlDocument2 = CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2>(disp);
    BreakOnNull(m_htmlDocument2, hr);

    m_htmlDocument3 = CComQIPtr<IHTMLDocument3, &IID_IHTMLDocument3>(disp);
    BreakOnNull(m_htmlDocument3, hr);

    // hr = DispEventAdvise(CComQIPtr<IUnknown, &IID_IUnknown>(m_htmlDocument3), &DIID_HTMLDocumentEvents);

    break;
  }

  if (FAILED(hr))
    logger->error(L"HTMLDocument::OnConnect() failed -> " + logger->parse(hr));

  return hr;
}


/**
 * Event: OnDisconnect
 */
HRESULT HTMLDocument::OnDisconnect()
{
    HRESULT hr = S_OK;
    /*hr = DispEventUnadvise(CComQIPtr<IUnknown, 
                                           &IID_IUnknown>(m_htmlDocument3), 
                                           &DIID_HTMLDocumentEvents);  */

    return hr;
}


/**
 * Inject content into document <body />
 */
HRESULT HTMLDocument::InjectDocument(const wstringpointer& content)
{
  HRESULT hr = S_OK;
  CComSafeArray<VARIANT> safeArray;

  for (;;) {
    BreakOnNull(content, hr);
    logger->debug(L"HTMLDocument::InjectBody -> " + (*content));

    BreakOnNull(m_htmlDocument2, hr);
    BreakOnNull(m_htmlDocument3, hr);
    
    // clear any existing content
    hr = m_htmlDocument2->clear();
    if (FAILED(hr)) {
      logger->error(L"HTMLDocument::InjectBody failed to clear document -> " + logger->parse(hr));
      break;
    }

    safeArray.Create(1, 0);
    safeArray[0] = CComBSTR((*content).c_str());
    hr = m_htmlDocument2->write(safeArray);
    if (FAILED(hr)) {
      logger->error(L"HTMLDocument::InjectBody failed to inject content -> " + logger->parse(hr));
      break;
    }

    // close stream
    hr = m_htmlDocument2->close();
    if (FAILED(hr))
      logger->error(L"HTMLDocument::InjectBody failed to close stream -> " + logger->parse(hr));
 
    break;
  }

  return hr;
}

/**
 * Inject a script into <head />
 */
HRESULT HTMLDocument::InjectScript(const wstringpointer& content)
{
  HRESULT hr = S_OK;
  CComQIPtr<IHTMLElement> element = nullptr;
  CComPtr<IHTMLElementCollection> heads = nullptr;
  CComPtr<IDispatch> disp = nullptr;
  CComPtr<IHTMLDOMNode> firstChild = nullptr;
  CComPtr<IHTMLDOMNode> retnode = nullptr;

  for (;;) {
    BreakOnNull(m_htmlDocument2, hr);
    BreakOnNull(m_htmlDocument3, hr);
    
    hr = m_htmlDocument2->createElement(HTMLDocument::tagScript, &element);
    BreakOnNull(element, hr);
    BreakOnFailed(hr);

    CComQIPtr<IHTMLScriptElement> script(element);
    BreakOnNull(script, hr);
    hr = script->put_defer(VARIANT_TRUE);
    BreakOnFailed(hr);

    hr = script->put_type(HTMLDocument::attrScriptType);
    BreakOnFailed(hr);

    hr = script->put_text(CComBSTR((*content).c_str()));
    BreakOnFailed(hr);

    hr = m_htmlDocument3->getElementsByTagName(HTMLDocument::tagHead, &heads);
    BreakOnFailed(hr);
    BreakOnNull(heads, hr);

    hr = heads->item(CComVariant(0, VT_I4), CComVariant(0, VT_I4), &disp);
    BreakOnFailed(hr);
    BreakOnNull(disp, hr);

    CComQIPtr<IHTMLDOMNode> head(disp);
    BreakOnNull(head, hr);
      
    hr = head->get_firstChild(&firstChild);
    BreakOnFailed(hr);
    BreakOnNull(firstChild, hr);

    hr = head->insertBefore(CComQIPtr<IHTMLDOMNode>(script), CComVariant(firstChild), &retnode);
    BreakOnNull(retnode, hr);
    break;
  }

  if (FAILED(hr))
    logger->error(L"HTMLDocument::InjectScript failed -> " + logger->parse(hr));

  return hr;
}

/**
 * Inject a script link into <head />
 */
HRESULT HTMLDocument::InjectScriptTag(const wstring& type, const wstring& src)
{
  HRESULT hr = S_OK;
  CComQIPtr<IHTMLElement> element = nullptr;
  CComPtr<IHTMLElementCollection> heads = nullptr;
  CComPtr<IDispatch> disp = nullptr;

  for (;;) {
    BreakOnNull(m_htmlDocument2, hr);
    BreakOnNull(m_htmlDocument3, hr);

    hr = m_htmlDocument2->createElement(HTMLDocument::tagScript, &element);
    BreakOnFailed(hr);
    BreakOnNull(element, hr);

    CComQIPtr<IHTMLScriptElement> script(element);
    hr = script->put_defer(VARIANT_TRUE);
    BreakOnFailed(hr);

    hr = script->put_type(CComBSTR(type.c_str()));
    BreakOnFailed(hr);
    hr = script->put_src(CComBSTR(src.c_str()));
    BreakOnFailed(hr);

    hr = m_htmlDocument3->getElementsByTagName(HTMLDocument::tagHead, &heads);
    BreakOnFailed(hr);
    BreakOnNull(heads, hr);

    hr = heads->item(CComVariant(0, VT_I4), CComVariant(0, VT_I4), &disp);
    BreakOnFailed(hr);
    BreakOnNull(disp, hr);

    hr = CComQIPtr<IHTMLDOMNode>(disp)->appendChild(CComQIPtr<IHTMLDOMNode>(script), &CComPtr<IHTMLDOMNode>());
    if (FAILED(hr))
      logger->debug(L"HTMLDocument::InjectScriptTag failed -> " + src + L" -> " + logger->parse(hr));

    logger->debug(L"HTMLDocument::InjectScriptTag -> " + src);
    break;
  }

  if (FAILED(hr))
    logger->error(L"HTMLDocument::InjectScriptTag failed -> " + logger->parse(hr));

  return hr;
}


/**
 * Inject a stylesheet into <head />
 */
HRESULT HTMLDocument::InjectStyle(const wstringpointer& content)
{
  HRESULT hr = S_OK;
  CComQIPtr<IHTMLStyleSheet> style = nullptr;

  for (;;) {
    BreakOnNull(content, hr);
    BreakOnNull(m_htmlDocument2, hr);
    BreakOnNull(m_htmlDocument3, hr);

    hr = m_htmlDocument2->createStyleSheet(L"", 1, &style);
    BreakOnFailed(hr);
    BreakOnNull(style, hr);

    hr = style->put_cssText(CComBSTR(content->c_str()));
    break;
  }

  if (FAILED(hr))
    logger->error(L"HTMLDocument::InjectStyle failed -> " + logger->parse(hr));

  return hr;
}


/**
 * Inject content into document <body />
 */
HRESULT HTMLDocument::InjectBody(const wstringpointer& content, BSTR where)
{
  HRESULT hr = S_OK;
  CComPtr<IHTMLElement> body = nullptr;

  for (;;) {
    BreakOnNull(content, hr);
    BreakOnNull(where, hr);
    BreakOnNull(m_htmlDocument2, hr);
    BreakOnNull(m_htmlDocument3, hr);
    hr = m_htmlDocument2->get_body(&body);
    BreakOnFailed(hr);
    BreakOnNull(body, hr);
    hr = body->insertAdjacentHTML(where, CComBSTR(content->c_str()));
    break;
  }

  if (FAILED(hr))
    logger->error(L"HTMLDocument::InjectBody failed -> " + logger->parse(hr));

  return hr;
}


/**
 * Inject content into document element
 */
HRESULT HTMLDocument::InjectElementById(const wstring& id, const wstringpointer& content, BSTR where)
{
  HRESULT hr = S_OK;
  CComPtr<IHTMLElement> element = nullptr;

  for (;;) {
    BreakOnNull(content, hr);
    BreakOnNull(where, hr);
    BreakOnNull(m_htmlDocument2, hr);
    BreakOnNull(m_htmlDocument3, hr);

    hr = m_htmlDocument3->getElementById(CComBSTR(id.c_str()), &element);
    BreakOnFailed(hr);
    BreakOnNull(element, hr);

    hr = element->insertAdjacentHTML(where, CComBSTR(content->c_str()));
    break;
  }

  if (FAILED(hr))
    logger->error(L"HTMLDocument::InjectElementById failed -> " + logger->parse(hr));
  
  return hr;
}


/**
 * Send a click event to an element
 */
HRESULT HTMLDocument::ClickElementById(const wstring& id)
{
  HRESULT hr = S_OK;
  CComPtr<IHTMLElement> element = nullptr;
  for (;;) {
    BreakOnNull(m_htmlDocument2, hr);
    BreakOnNull(m_htmlDocument3, hr);
    hr = m_htmlDocument3->getElementById(CComBSTR(id.c_str()), &element);
    BreakOnFailed(hr);
    BreakOnNull(element, hr);

    hr = element->click();
    break;
  }

  if (FAILED(hr))
    logger->error(L"HTMLDocument::ClickElementById failed -> " + logger->parse(hr));
  
  return hr;
}
