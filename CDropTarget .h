#pragma once
#include <windows.h>
#include <commctrl.h>

class CDropTarget : public IDropTarget {
    LONG m_ref;
    HWND m_hWnd;

public:
    CDropTarget(HWND hWnd) : m_ref(1), m_hWnd(hWnd) {}

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_IDropTarget) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&m_ref); }
    ULONG STDMETHODCALLTYPE Release() {
        LONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return r;
    }

    // IDropTarget
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD, POINTL, DWORD* pdwEffect) {
        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DragOver(DWORD, POINTL, DWORD* pdwEffect) {
        *pdwEffect = DROPEFFECT_COPY;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DragLeave() {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD, POINTL, DWORD* pdwEffect) {
        *pdwEffect = DROPEFFECT_COPY;

        FORMATETC fmt;
        STGMEDIUM stg;

        // On demande du texte (URL sous forme texte)
        fmt.cfFormat = CF_TEXT;
        fmt.ptd = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex = -1;
        fmt.tymed = TYMED_HGLOBAL;

        if (SUCCEEDED(pDataObj->GetData(&fmt, &stg))) {
            char* pText = (char*)GlobalLock(stg.hGlobal);
            if (pText) {
                // pText contient l’URL
                SetWindowTextA(g_hEditLink, pText);
                GlobalUnlock(stg.hGlobal);
            }
            ReleaseStgMedium(&stg);
        }

        return S_OK;
    }
};
