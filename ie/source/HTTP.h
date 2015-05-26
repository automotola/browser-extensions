#pragma once

#include <util.h>
#include <json_spirit/json_spirit.h>

using namespace ATL;

// forward declarations
class HTTP;

/**
 * Implementation: IBindStatusCallback
 */
template <class T, int flags = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_GETNEWESTVERSION>
class ATL_NO_VTABLE HTTPBindStatusCallback : public CBindStatusCallback <T, flags>, public IHttpNegotiate
{
public:
  BEGIN_COM_MAP(HTTPBindStatusCallback<T>)
    COM_INTERFACE_ENTRY(IBindStatusCallback)
    COM_INTERFACE_ENTRY(IHttpNegotiate)
  END_COM_MAP()

  // aliases
  typedef void (T::*ATL_PDATAAVAILABLE1)(CBindStatusCallback<T, flags>* pbsc,
    BYTE* pBytes, DWORD dwSize);
  typedef void (T::*ATL_PDATAAVAILABLE2)(HTTPBindStatusCallback<T, flags>* pbsc,
    BYTE* pBytes, DWORD dwSize);

  HTTPBindStatusCallback()
    : verb(BINDVERB_GET),
    method(NULL),
    data(NULL),
    datasize(0) {
    //logger->debug(L"HTTPBindStatusCallback::HTTPBindStatusCallback");
  };

  ~HTTPBindStatusCallback() {
    //logger->debug(L"HTTPBindStatusCallback::~HTTPBindStatusCallback");
    if (this->data) {
      ::GlobalFree(this->data);
      this->data = NULL;
    }
  }

  HTTP* http;

  /**
   * Method: Async
   *
   * Perform an async HTTP request
   */
  HRESULT Async(_In_ T* pT,
    _In_ ATL_PDATAAVAILABLE callback,
    _In_z_ BSTR imethod,
    _In_z_ BSTR url,
    _In_z_ BSTR idata,
    _In_z_ BSTR icontentType,
    _Inout_opt_ IUnknown* container = NULL,
    _In_ BOOL relative = FALSE) 
  {
    if (pT)
      http = (HTTP*)pT;
    
    method = imethod;
    contentType = icontentType;

    // convert POST data to ASCII
    if (data) {
      ::GlobalFree(data);
      data = nullptr;
    }

    wstring s(idata);
    std::string asciidata(s.begin(), s.end());
    datasize = static_cast<DWORD>(asciidata.length());
    data = static_cast<byte*>(::GlobalAlloc(GPTR, datasize));
    if (!data) {
      DWORD error_code = ::GetLastError();
      HRESULT hr = error_code != NO_ERROR ? HRESULT_FROM_WIN32(error_code) : E_FAIL;
      logger->debug(L"HTTPBindStatusCallback::StartAsyncDownload ::GlobalAlloc failed -> " + logger->parse(hr));
      return hr;
    }

    memcpy(data, asciidata.c_str(), datasize);

    wstring lverb(imethod);
    if (lverb == L"GET") {
      verb = BINDVERB_GET;
    }
    else if (lverb == L"PUT") {
      verb = BINDVERB_PUT;
    }
    else if (lverb == L"POST") {
      verb = BINDVERB_POST;
    }
    else if (lverb == L"DELETE") {
      verb = BINDVERB_CUSTOM; // TODO 
    }
    else {
      verb = BINDVERB_CUSTOM;
    }

    return CBindStatusCallback<T, flags>::StartAsyncDownload(pT, callback, url, container, relative);
  }


  /**
   * IBindStatusCallback
   */
  STDMETHOD(OnStartBinding)(DWORD reserved, IBinding *binding)
  {
    //logger->debug(L"HTTPBindStatusCallback::OnStartBinding");
    // broken microsoft api's are broken, see:
    // http://groups.google.com/groups?as_umsgid=08ee01c177c7$e8ca3dd0$3aef2ecf@TKMSFTNGXA09
    HRESULT hr = S_OK;
    for (;;) {
      hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
      if (FAILED(hr)) {
        logger->error(L"HTTP::OnBindStatusCallback CoinitializeEx failed -> " + logger->parse(hr));
        break;
      }
      hr = CBindStatusCallback<T, flags>::OnStartBinding(reserved, binding);
      if (FAILED(hr))
        logger->error(L"HTTP::OnBindStatusCallback CBindStatusCallback failed -> " + logger->parse(hr));
      break;
    }
    
    ::CoUninitialize();
    return hr;
  }

  STDMETHOD(BeginningTransaction)(LPCWSTR url, LPCWSTR headers, DWORD reserved, LPWSTR *additionalHeaders)  
  {
    logger->debug(L"HTTPBindStatusCallback::BeginningTransaction -> " + boost::lexical_cast<wstring>(url) + L" -> " + boost::lexical_cast<wstring>(headers));

    if (!additionalHeaders) {
      logger->error(L"HTTPBindStatusCallback::BeginningTransaction could not set additional headers");
      return E_POINTER;
    }

    // set Content-Type header
    wstring headerString;
    if (contentType)
      headerString = L"Content-Type: " + wstring(contentType) + L"\n";
    
    // set additional headers
    if (http) {
      for (auto& i : http->headers) {
        logger->debug(L"Setting header: " + i.first + L" -> " + i.second);
        headerString += i.first + L": " + i.second + L"\n";
      }
    }

    *additionalHeaders = NULL;
    LPWSTR p = (LPWSTR)::CoTaskMemAlloc((headerString.length() + 1) * sizeof(WCHAR));
    if (!p) {
      logger->error(L"HTTPBindStatusCallback::BeginningTransaction out of memory");
      return E_OUTOFMEMORY;
    }
    wcscpy_s(p, (headerString.length() + 1 * sizeof(WCHAR)), headerString.c_str());
    *additionalHeaders = p;

    return S_OK;
  }

  STDMETHOD(OnResponse)(DWORD responseCode, LPCWSTR responseHeaders,LPCWSTR requestHeaders, LPWSTR *extRequestHeaders) 
  {
    if (responseCode < 400)
      return S_OK;
    
    if (http && http->error) {
      wstring json = L"{\"statusCode\": \"" + boost::lexical_cast<wstring>(responseCode) + L"\"}";
      logger->debug(L"HTTPBindStatusCallback::OnResponse -> " + boost::lexical_cast<wstring>(this->http->error));
      HRESULT hr = CComDispatchDriver(this->http->error).Invoke1((DISPID)0, &CComVariant(json.c_str()));
      if (FAILED(hr))
        logger->error(L"HTTPBindStatusCallback::OnResponse Failed to invoke error callback -> " + logger->parse(hr));
    }

    return E_FAIL;
  }

  STDMETHOD(GetBindInfo)(DWORD *bindf, BINDINFO *bindinfo)
  {
    if (bindinfo == NULL || bindinfo->cbSize == 0 || bindf == NULL)
      return E_INVALIDARG;
    
    ULONG cbSize = bindinfo->cbSize;        // remember incoming cbSize       
    memset(bindinfo, 0, sizeof(BINDINFO));  // zero out structure
    bindinfo->cbSize = cbSize;              // restore cbSize
    bindinfo->dwBindVerb = verb;            // set verb
    *bindf = flags;                         // set flags

    switch (verb) {
    case BINDVERB_GET:
      break;
    case BINDVERB_POST:
    case BINDVERB_PUT:
      bindinfo->stgmedData.tymed = TYMED_HGLOBAL;
      bindinfo->stgmedData.hGlobal = this->data;
      bindinfo->stgmedData.pUnkForRelease =
        static_cast<IBindStatusCallback*>(this);
      AddRef();
      bindinfo->cbstgmedData = this->datasize;
      break;
    default:
      logger->debug(L"HTTPBindStatusCallback::GetBindInfo does not support HTTP method: " + boost::lexical_cast<wstring>(this->method));
      return E_FAIL;
    };

    return S_OK;
  }

private:
  BSTR method;
  byte *data;
  DWORD datasize;
  BSTR contentType;
  BINDVERB verb;
};

class HTTP;
typedef HTTPBindStatusCallback<HTTP, BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_GETNEWESTVERSION> BindStatusCallback;

/**
 * HTTP
 */
class HTTP : public CComObjectRootEx < CComSingleThreadModel >
{
public:

  HTTP(const wstring& url, const AsyncCallback& callback_std)
    : method(L"GET"),
    url(url),
    callback_std(callback_std),
    callback(NULL)
  {
    bytes = shared_ptr<bytevector>(new bytevector);
    size = 0;
  }

  HTTP(const wstring& url, IDispatch *callback)
    : method(L"GET"),
    url(url),
    callback(callback)
  {
    if (callback)
      callback->AddRef();
    bytes = shared_ptr<bytevector>(new bytevector);
    size = 0;
  }

  HTTP(const wstring& method, const wstring& url, const wstring& data, const wstring& contentType, const wstring& json_headers, IDispatch *callback, IDispatch *error)
    : method(method),
    url(url),
    data(data),
    contentType(contentType),
    callback_std(NULL),
    callback(callback),
    error(error)
  {
    if (callback)
      callback->AddRef();
    if (error)
      error->AddRef();
    bytes = shared_ptr<bytevector>(new bytevector);
    size = 0;

    // parse headers
    if (json_headers != L"") {
      logger->debug(L"HTTP::HTTP Parsing headers -> " + json_headers);
      json_spirit::wValue v;
      json_spirit::read(json_headers, v);
      for (auto& i : v.get_obj())
        headers[i.name_] = i.value_.get_str();
    }
  }

  ~HTTP() 
  {
    logger->debug(L"HTTP::~HTTP");
    if (callback) 
      callback->Release();
    if (error)
      error->Release();
  }

  void OnData(BindStatusCallback *caller, BYTE *bytes, DWORD size);

public:// private:
  wstring method;
  wstring url;
  wstring data;
  wstring contentType;
  wstringmap headers;

  IDispatch *callback;
  IDispatch *error; // TODO implement all failure cases
  AsyncCallback callback_std;

  shared_ptr<bytevector> bytes;
  DWORD size;
};
