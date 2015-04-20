#include "stdafx.h"
#include "HTTP.h"


/**
 * HTTP::OnData
 */
void HTTP::OnData(BindStatusCallback *caller, BYTE *pData, DWORD sz)  
{
    // buffer data
    if (pData) {
        /*logger->debug(L"HTTP::OnData"
                      L" -> " + this->url + 
                      L" -> " + boost::lexical_cast<wstring>(size) + L" bytes");*/
        size += sz;
        bytes->insert(bytes->end(), pData, pData + sz);
        return;
    }

    // transfer complete
    wstring response(bytes->begin(), bytes->end());
    /*logger->debug(L"HTTP::OnData"
                  L" -> " + this->url + 
                  L" -> " + boost::lexical_cast<wstring>(this->size) + L" bytes " +
                  L" -> |" + wstring_limit(response) + L"|");*/
    
    // invoke appropriate callback
    if (callback) { 
        //logger->debug(L"HTTP::OnData -> IDispatch*");
        HRESULT hr = CComDispatchDriver(callback)
            .Invoke1((DISPID)0, &CComVariant(response.c_str()));
        if (FAILED(hr)) {
            logger->error(L"HTTP::OnData Failed to invoke event OnComplete"
                          L" -> " + logger->parse(hr));
        }
    } else if (callback_std) { 
        logger->debug(L"HTTP::OnData -> callback");
        callback_std(response);
    } else {
        logger->debug(L"HTTP::OnData -> no callback registered");
    }
    //logger->debug(L"HTTP::OnData -> done");

    // TODO error handling
    
    // cleanup
    //delete this; TODO fix this leak sans abominations
}


