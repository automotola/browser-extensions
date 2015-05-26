#include "stdafx.h"
#include "HTTP.h"


/**
 * HTTP::OnData
 */
void HTTP::OnData(BindStatusCallback *caller, BYTE *pData, DWORD sz)  
{
  // buffer data
  if (pData) {
    size += sz;
    bytes->insert(bytes->end(), pData, pData + sz);
    return;
  }

  // transfer complete
  wstring response(bytes->begin(), bytes->end());

  // invoke appropriate callback
  if (callback) {
    HRESULT hr = CComDispatchDriver(callback).Invoke1((DISPID)0, &CComVariant(response.c_str()));
    if (FAILED(hr))
      logger->error(L"HTTP::OnData Failed to invoke event OnComplete -> " + logger->parse(hr));
  } else if (callback_std) {
    logger->debug(L"HTTP::OnData -> callback");
    callback_std(response);
  } else
    logger->debug(L"HTTP::OnData -> no callback registered");

  // TODO error handling
}
