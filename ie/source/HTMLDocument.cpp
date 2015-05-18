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
HTMLDocument::HTMLDocument(const CComQIPtr<IWebBrowser2,
                                           &IID_IWebBrowser2>& webBrowser2)
    : m_webBrowser2(webBrowser2)
{
    HRESULT hr;
    hr = OnConnect();
    if (FAILED(hr)) {
        logger->error(L"HTMLDocument::HTMLDocument failed to connect");
    }
}

/**
 * Destruction
 */
HTMLDocument::~HTMLDocument()
{
    HRESULT hr;
    hr = OnDisconnect();
    if (FAILED(hr)) {
        logger->error(L"HTMLDocument::~HTMLDocument failed to disconnect");
    }
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
  if (!content)
    return E_POINTER;

  logger->debug(L"HTMLDocument::InjectBody -> " + (*content));

  if (!m_htmlDocument3 || !m_htmlDocument2) {
    return E_POINTER;
  }

  HRESULT hr = S_OK;

  // clear any existing content
  hr = m_htmlDocument2->clear();
  if (FAILED(hr)) {
    logger->error(L"HTMLDocument::InjectBody failed to clear document"
      L" -> " + logger->parse(hr));
    return hr;
  }

  // inject content
  CComSafeArray<VARIANT> safeArray;
  safeArray.Create(1, 0);
  safeArray[0] = CComBSTR((*content).c_str());
  hr = m_htmlDocument2->write(safeArray);
  if (FAILED(hr)) {
    logger->error(L"HTMLDocument::InjectBody failed to inject content"
      L" -> " + logger->parse(hr));
    return hr;
  }

  // close stream
  hr = m_htmlDocument2->close();
  if (FAILED(hr)) {
    logger->error(L"HTMLDocument::InjectBody failed to close stream"
      L" -> " + logger->parse(hr));
    return hr;
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

  return hr;
}

/**
 * Inject a script link into <head />
 */
HRESULT HTMLDocument::InjectScriptTag(const wstring& type, const wstring& src)
{
  HRESULT hr = S_OK;
  for (;;) {
    if (!m_htmlDocument3 || !m_htmlDocument2) {
      hr = E_POINTER;
      break;
    }

    CComQIPtr<IHTMLElement> element;
    hr = m_htmlDocument2->createElement(HTMLDocument::tagScript, &element);
    if (FAILED(hr))
      break;
    if (!element) {
      hr = E_POINTER;
      break;
    };

    CComQIPtr<IHTMLScriptElement> script(element);
    hr = script->put_defer(VARIANT_TRUE);
    if (FAILED(hr))
      break;
    hr = script->put_type(CComBSTR(type.c_str()));
    if (FAILED(hr))
      break;
    hr = script->put_src(CComBSTR(src.c_str()));
    if (FAILED(hr))
      break;

    CComPtr<IHTMLElementCollection> heads;
    hr = m_htmlDocument3->getElementsByTagName(HTMLDocument::tagHead, &heads);
    if (FAILED(hr))
      break;
    if (!heads) {
      hr = E_POINTER;
      break;
    };

    CComPtr<IDispatch> disp;
    hr = heads->item(CComVariant(0, VT_I4), CComVariant(0, VT_I4), &disp);
    if (FAILED(hr))
      break;
    if (!disp) {
      hr = E_POINTER;
      break;
    };

    hr = CComQIPtr<IHTMLDOMNode>(disp)->appendChild(CComQIPtr<IHTMLDOMNode>(script), &CComPtr<IHTMLDOMNode>());
    if (FAILED(hr)) {
      logger->debug(L"HTMLDocument::InjectScriptTag failed "
        L" -> " + src +
        L" -> " + logger->parse(hr));
    }

    logger->debug(L"HTMLDocument::InjectScriptTag "
      L" -> " + src);
    break;
  }

  return hr;
}


/**
 * Inject a stylesheet into <head />
 */
HRESULT HTMLDocument::InjectStyle(const wstringpointer& content)
{
    HRESULT hr;

    if (!m_htmlDocument3 || !m_htmlDocument2) {
        return E_POINTER;
    }

    CComQIPtr<IHTMLStyleSheet> style;
    hr = m_htmlDocument2->createStyleSheet(L"", 1, &style);
    if (FAILED(hr) || !style) {
        logger->debug(L"HTMLDocument::InjectStyle"
                      L" -> failed to create stylesheet");
        return FAILED(hr) ? hr : E_POINTER; 
    }

    hr = style->put_cssText(CComBSTR((*content).c_str()));
    if (FAILED(hr)) {
        logger->debug(L"HTMLDocument::InjectStyle"
                      L" -> " + (*content) +
                      L" -> failed to set content");
        return hr;
    }

    return S_OK;
}


/**
 * Inject content into document <body />
 */
HRESULT HTMLDocument::InjectBody(const wstringpointer& content, BSTR where)
{
    if (!m_htmlDocument3 || !m_htmlDocument2) {
        return E_POINTER;
    }

    CComPtr<IHTMLElement> body;
    HRESULT hr = m_htmlDocument2->get_body(&body);
    if (FAILED(hr) || !body) return FAILED(hr) ? hr : E_POINTER; 

    return body->insertAdjacentHTML(where, CComBSTR((*content).c_str()));
}


/**
 * Inject content into document element
 */
HRESULT HTMLDocument::InjectElementById(const wstring& id, 
                                        const wstringpointer& content,
                                        BSTR where)
{
    if (!m_htmlDocument3 || !m_htmlDocument2) {
        return E_POINTER;
    }

    CComPtr<IHTMLElement> element;
    HRESULT hr = m_htmlDocument3->getElementById(CComBSTR(id.c_str()), 
                                                 &element);
    if (FAILED(hr) || !element) return FAILED(hr) ? hr : E_POINTER; 

    return element->insertAdjacentHTML(where, CComBSTR((*content).c_str()));
}


/**
 * Send a click event to an element
 */
HRESULT HTMLDocument::ClickElementById(const wstring& id)
{
    if (!m_htmlDocument3 || !m_htmlDocument2) {
        return E_POINTER;
    }

    CComPtr<IHTMLElement> element;
    HRESULT hr = m_htmlDocument3->getElementById(CComBSTR(id.c_str()), 
                                                 &element);
    if (FAILED(hr) || !element) return FAILED(hr) ? hr : E_POINTER; 

    return element->click();
}
