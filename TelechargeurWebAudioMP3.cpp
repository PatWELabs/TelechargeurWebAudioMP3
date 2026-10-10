#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <cstring>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shfolder.lib")

#define CSIDL_MYVIDEO 0x000e

HINSTANCE g_hInst;

HWND g_hEditLink;
HWND g_hEditCmd;
HWND g_hBtnAdd;
HWND g_hBtnRun;
HWND g_hBtnBrowse;
HWND g_hListBefore;
HWND g_hListAfter;
HWND g_hStatus;
HWND g_hProgress;
HWND g_hBtnAll;

char g_szTargetFolder[MAX_PATH] = "";

static void ProvisionDevice(const char* link, const char* targetFolder);
static void TexteStatut(const char* text);
static void Progresssion(int value);
BOOL RechercherDossier(HWND hWndOwner, char* outFolder, int outSize);
static void ActiverGlisserDeposser(HWND hWnd);
static void DepotFichiers(HWND hWnd, HDROP hDrop);
static void InitialiserColonnesListView(HWND hList);
static void ExecuterCommandeConsole(const char* cmd);
static void AjouterLien();
static void Cascade();

class CDropTarget : public IDropTarget {
    LONG m_ref;
    HWND m_hWnd;
public:
    CDropTarget(HWND hWnd) : m_ref(1), m_hWnd(hWnd) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) { if (riid == IID_IUnknown || riid == IID_IDropTarget) { *ppv = this; AddRef(); return S_OK;} *ppv = NULL; return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&m_ref); }
    ULONG STDMETHODCALLTYPE Release() { LONG r = InterlockedDecrement(&m_ref); if (r == 0) delete this; return r;}
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD, POINTL, DWORD* pdwEffect) { *pdwEffect = DROPEFFECT_COPY; return S_OK;}
    HRESULT STDMETHODCALLTYPE DragOver(DWORD, POINTL, DWORD* pdwEffect) { *pdwEffect = DROPEFFECT_COPY; return S_OK;}
    HRESULT STDMETHODCALLTYPE DragLeave() { return S_OK;}
    HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD, POINTL, DWORD* pdwEffect) { 
	*pdwEffect = DROPEFFECT_COPY;
	FORMATETC fmt; STGMEDIUM stg;
	fmt.cfFormat = CF_TEXT; 
	fmt.ptd = NULL; 
	fmt.dwAspect = DVASPECT_CONTENT; 
	fmt.lindex = -1; 
	fmt.tymed = TYMED_HGLOBAL;
	if (SUCCEEDED(pDataObj->GetData(&fmt, &stg))) { 
		char* pText = (char*)GlobalLock(stg.hGlobal); 
		if (pText) {
			LVITEMA item;
			ZeroMemory(&item, sizeof(item));
			item.mask = LVIF_TEXT;
			item.iItem = ListView_GetItemCount(g_hListBefore);
			item.pszText = pText;
			ListView_InsertItem(g_hListBefore, &item);
			SetWindowText(g_hEditLink, pText);
			TexteStatut("Lien ajouté à la liste.");
			GlobalUnlock(stg.hGlobal); 
		}
		ReleaseStgMedium(&stg); 
		}return S_OK;
    }
};
static void ProvisionDevice(const char* link, const char* targetFolder) {    Sleep(500);}
static void TexteStatut(const char* text) {   SendMessageA(g_hStatus, SB_SETTEXTA, 0, (LPARAM)text);    }
static void Progresssion(int value) {    SendMessage(g_hProgress, PBM_SETPOS, value, 0);}
BOOL RechercherDossier(HWND hWndOwner, char* outFolder, int outSize) {
    BROWSEINFOA bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hWndOwner;
    bi.lpszTitle = "Choisir le dossier de traitement";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) { lstrcpynA(outFolder, path, outSize); LPMALLOC pMalloc; if (SHGetMalloc(&pMalloc)) {     pMalloc->Free(pidl);     pMalloc->Release(); } return TRUE;
        }
    }
    return FALSE;
}
static void ActiverGlisserDeposser(HWND hWnd) {    DragAcceptFiles(hWnd, TRUE);}
static void DepotFichiers(HWND hWnd, HDROP hDrop) {
    UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
    char buffer[1024];
    for (UINT i = 0; i < count; ++i) {
        if (DragQueryFileA(hDrop, i, buffer, sizeof(buffer))) { 
		LVITEMA item; 
		ZeroMemory(&item, sizeof(item)); 
		item.mask = LVIF_TEXT; 
		item.iItem = ListView_GetItemCount(g_hListBefore); 
		item.pszText = buffer; 
		ListView_InsertItem(g_hListBefore, &item);
        }
    }
    DragFinish(hDrop);
    TexteStatut("Liens déposés.");
}
static void InitialiserColonnesListView(HWND hList) {
    LVCOLUMNA col;
    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText =(char*) "Lien / Chemin";
    col.cx = 400;
    col.iSubItem = 0;
    ListView_InsertColumn(hList, 0, &col);
    col.pszText = (char*)"Statut";
    col.cx = 150;
    col.iSubItem = 1;
    ListView_InsertColumn(hList, 1, &col);
}
///utilisation de ffmpeg pour le format mp3 44.1kHz
static void ExecuterCommandeConsole(const char* cmd) {
    if (!cmd || !*cmd)  return;
    char full[1024];
    lstrcpyA(full, "yt-dlp -x --audio-format mp3 --audio-quality 128K --embed-thumbnail --postprocessor-args \"ExtractAudio+ffmpeg:-ar 44100\"  ");
    lstrcatA(full, cmd);
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    if (CreateProcessA( NULL, full, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        TexteStatut("Commande console lancée.");
    }
    else {TexteStatut("Erreur lancement commande."); }
}
static void AjouterLien() {
    char buf[1024];
    GetWindowTextA(g_hEditLink, buf, sizeof(buf));
    if (lstrlenA(buf) == 0)
        return;
    if(MessageBox(0x00,buf,"Voulez vous telecharger maintenant?",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==0x06)
    {
        ExecuterCommandeConsole(buf);
        TexteStatut("Lien en telechargement.");
    }
    LVITEMA item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(g_hListBefore);
    item.pszText = buf;
    ListView_InsertItem(g_hListBefore, &item);
    SetWindowText(g_hEditLink, 0x00);
    TexteStatut("Lien ajouté à la liste.");
}
static void Cascade() {
    int count = ListView_GetItemCount(g_hListBefore);
    if (count == 0) {TexteStatut("Aucun lien à traiter.");return;}
    if (g_szTargetFolder[0] == '\0') {
        TexteStatut("Choisir un dossier de traitement avant.");
        MessageBoxA(NULL, "Veuillez choisir un dossier de traitement.", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    TexteStatut("Traitement en cours...");
    Progresssion(0);
    char text[1024];
    for (int i = 0; i < count; ++i) {
        LVITEMA item;
        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = text;
        item.cchTextMax = sizeof(text);

        if (ListView_GetItem(g_hListBefore, &item)) {
			ProvisionDevice(text, g_szTargetFolder); 
			ExecuterCommandeConsole(text);
			LVITEMA itemAfter; 
			ZeroMemory(&itemAfter, sizeof(itemAfter)); 
			itemAfter.mask = LVIF_TEXT; 
			itemAfter.iItem = ListView_GetItemCount(g_hListAfter); 
			itemAfter.pszText = text; 
			int idx = ListView_InsertItem(g_hListAfter, &itemAfter);
			ListView_SetItemText(g_hListAfter, idx, 1,(char*) "OK");
        }
        int progress = (int)((i + 1) * 100 / count);
        Progresssion(progress);
    }
    ListView_DeleteAllItems(g_hListBefore);
    ListView_DeleteAllItems(g_hListAfter);
    TexteStatut("Traitement terminé.");
}
static LRESULT CALLBACK ProcedurePrincipale(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        OleInitialize(NULL);
        CDropTarget* pDrop = new CDropTarget(hWnd);
        RegisterDragDrop(hWnd, pDrop);
        g_hBtnAll = CreateWindowA( "BUTTON", "Tout", WS_CHILD | WS_VISIBLE, 530, 40, 100, 22, hWnd, (HMENU)110, g_hInst, NULL);    // EDIT lien
        g_hEditLink = CreateWindowExA( WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 10, 10, 400, 22, hWnd, (HMENU)101, g_hInst, NULL);
        g_hBtnAdd = CreateWindowA( "BUTTON", "Ajouter lien", WS_CHILD | WS_VISIBLE, 420, 10, 100, 22, hWnd, (HMENU)102, g_hInst, NULL);
        g_hEditCmd = CreateWindowExA( WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 10, 40, 400, 22, hWnd, (HMENU)103, g_hInst, NULL);
        g_hBtnRun = CreateWindowA( "BUTTON", "Executer cmd", WS_CHILD | WS_VISIBLE, 420, 40, 100, 22, hWnd, (HMENU)104, g_hInst, NULL);
        g_hBtnBrowse = CreateWindowA( "BUTTON", "Dossier...", WS_CHILD | WS_VISIBLE, 530, 10, 100, 22, hWnd, (HMENU)105, g_hInst, NULL);
        g_hListBefore = CreateWindowExA( WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, 10, 80, 350, 260, hWnd, (HMENU)106, g_hInst, NULL);
        g_hListAfter = CreateWindowExA( WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, 370, 80, 350, 260, hWnd, (HMENU)107, g_hInst, NULL);
        InitialiserColonnesListView(g_hListBefore);
        InitialiserColonnesListView(g_hListAfter);
        g_hStatus = CreateWindowExA( 0, STATUSCLASSNAMEA, "", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, (HMENU)108, g_hInst, NULL);
        int parts[2] = { 400, -1 };
        SendMessage(g_hStatus, SB_SETPARTS, 2, (LPARAM)parts);
        TexteStatut("Prêt.");
        SendMessageA(g_hStatus, SB_SETTEXTA, 1, (LPARAM)"© Patrice Waechter - Ebling 2026");
        g_hProgress = CreateWindowExA( 0, PROGRESS_CLASSA, "", WS_CHILD | WS_VISIBLE, 10, 350, 710, 20, hWnd, (HMENU)109, g_hInst, NULL);
        SendMessage(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        Progresssion(0);
        ActiverGlisserDeposser(hWnd);
        return 0;
    }
    case WM_SIZE:SendMessage(g_hStatus, WM_SIZE, 0, 0);return 0;
    case WM_DROPFILES:DepotFichiers(hWnd, (HDROP)wParam);return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 102:AjouterLien(); break;
        case 104:{ char cmd[1024]; GetWindowTextA(g_hEditCmd, cmd, sizeof(cmd)); ExecuterCommandeConsole(cmd); break;}
        case 110:Cascade(); break;
        case 105:
        { 
			if (RechercherDossier(hWnd, g_szTargetFolder, sizeof(g_szTargetFolder))) {
				char msg[512];
				wsprintfA(msg, "Dossier : %s", g_szTargetFolder);
				TexteStatut(msg);
				SetCurrentDirectory(g_szTargetFolder); 
			} else {TexteStatut("Aucun dossier sélectionné.");
			} break;
        }
        default: break;
        }
        return 0;
    case WM_KEYDOWN:
		if (wParam == VK_F5) {Cascade();}return 0;
    case WM_DESTROY:
        RevokeDragDrop(hWnd);
        OleUninitialize();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInst = hInstance;
    InitCommonControls();
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS | ICC_WIN95_CLASSES | ICC_USEREX_CLASSES|ICC_COOL_CLASSES|ICC_INTERNET_CLASSES | ICC_STANDARD_CLASSES | ICC_LINK_CLASS;
    InitCommonControlsEx(&icc);
    GetCurrentDirectory(0xFF, g_szTargetFolder);
    HRESULT hr = SHGetFolderPath(NULL,CSIDL_MYVIDEO, NULL,0, g_szTargetFolder);
    if (SUCCEEDED(hr)){SendMessageA(g_hStatus, SB_SETTEXTA, 0, (LPARAM)g_szTargetFolder);}
    else{MessageBox(NULL, TEXT("Erreur SHGetFolderPath"), TEXT("Erreur"), MB_OK);}  
    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ProcedurePrincipale;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(wc.hInstance, (LPCSTR)101);
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE +1);
    wc.lpszClassName = "TelechargeurWeb";
    if (!RegisterClassExA(&wc)) return 0;
    HWND hWnd = CreateWindowExA( WS_EX_DLGMODALFRAME| WS_EX_ACCEPTFILES | WS_EX_TRANSPARENT| WS_EX_WINDOWEDGE |WS_EX_CLIENTEDGE, wc.lpszClassName, "Telechargeur flux Audio a partir de video web",WS_POPUPWINDOW| WS_CAPTION,CW_USEDEFAULT, CW_USEDEFAULT, 760, 450,NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
