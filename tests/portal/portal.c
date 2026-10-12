/* Tests for the XDG desktop portal file dialogs, see run.sh.
 *
 *   portal.exe legacy|save        GetOpenFileNameW / GetSaveFileNameW
 *   portal.exe hook|reject        GetOpenFileNameW with a hook and a Save for Web like template
 *   portal.exe hooka              GetSaveFileNameA with an ANSI hook and the same template
 *   portal.exe savedef            GetSaveFileNameW with a default extension
 *   portal.exe multi              GetOpenFileNameW with OFN_ALLOWMULTISELECT
 *   portal.exe modal              IFileOpenDialog and GetOpenFileNameW owned by a window
 *   portal.exe item|itemsave      IFileOpenDialog / IFileSaveDialog with custom controls
 *   portal.exe itemreopen         IFileOpenDialog whose OnFileOk refuses the first file
 *   portal.exe folder             IFileOpenDialog with FOS_PICKFOLDERS
 *   portal.exe browse             SHBrowseForFolderW
 *   portal.exe fallback|off|vanish GetOpenFileNameW expecting Wine's dialog
 *   portal.exe itemfallback       IFileOpenDialog expecting Wine's dialog
 *   portal.exe browsefallback     SHBrowseForFolderW expecting the classic dialog
 *   portal.exe hookauto           the hook test expecting Wine's dialog
 *   portal.exe itemlegacy         IFileOpenDialog without anything special
 *
 * <dir> (second argument) is the Windows path of the test files. Wine's own dialogs are
 * cancelled as soon as they are visible. The last line is "RESULT ok|cancel|fail <detail>". */
#define COBJMACROS
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <stdio.h>

#define ID_GROUP     100
#define ID_IMPORTAS  101
#define ID_SEQUENCE  102
#define ID_NOLABEL   103
#define ID_FMTTEXT   104
#define ID_FORMAT    105
#define ID_COPY      201
#define ID_DISABLED  202
#define ID_PROFILE   204
#define ID_CLOUD     205

#define IDD_SAVEFORWEB 3843
#define ID_SETTINGS    0xbba
#define ID_SLICES      0xbb9
#define ID_METADATA    0x101
#define ID_GREYED      0x102
#define ID_RADIO_A     0x104
#define ID_RADIO_B     0x105

static const WCHAR *dir = L".";
static WCHAR mode[32];
static int file_ok_calls, type_changes;
static HWND owner;
static int ticks, ticks_disabled;

static void CALLBACK timer_proc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time)
{
    ticks++;
    if (owner && !IsWindowEnabled(owner)) ticks_disabled++;
}

static DWORD WINAPI watch_thread(void *arg)
{
    const WCHAR *title = arg;
    WCHAR text[256];
    HWND hwnd;
    int i;

    for (i = 0; i < 400; i++)
    {
        if ((hwnd = FindWindowW(L"#32770", title)) && IsWindowVisible(hwnd))
        {
            GetWindowTextW(hwnd, text, ARRAYSIZE(text));
            printf("wine dialog visible: %ls\n", text);
            Sleep(300);
            PostMessageW(hwnd, WM_COMMAND, IDCANCEL, 0);
            return 0;
        }
        Sleep(50);
    }
    return 0;
}

static void watch(const WCHAR *title)
{
    CloseHandle(CreateThread(NULL, 0, watch_thread, (void *)title, 0, NULL));
}

static void path_of(const WCHAR *name, WCHAR *buf)
{
    swprintf(buf, MAX_PATH, L"%ls\\%ls", dir, name);
}

struct events
{
    IFileDialogEvents IFileDialogEvents_iface;
    IFileDialogControlEvents IFileDialogControlEvents_iface;
};

static HRESULT WINAPI events_QueryInterface(IFileDialogEvents *iface, REFIID riid, void **out)
{
    struct events *This = CONTAINING_RECORD(iface, struct events, IFileDialogEvents_iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IFileDialogEvents))
        *out = &This->IFileDialogEvents_iface;
    else if (IsEqualIID(riid, &IID_IFileDialogControlEvents))
        *out = &This->IFileDialogControlEvents_iface;
    else
    {
        *out = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}
static ULONG WINAPI events_AddRef(IFileDialogEvents *iface) { return 2; }
static ULONG WINAPI events_Release(IFileDialogEvents *iface) { return 1; }

static void print_controls(IFileDialog *dlg, const char *when)
{
    IFileDialogCustomize *custom;
    DWORD item = 0;
    BOOL a = -1, b = -1, c = -1;
    HRESULT hr;

    if (FAILED(IFileDialog_QueryInterface(dlg, &IID_IFileDialogCustomize, (void **)&custom))) return;
    if (!wcscmp(mode, L"itemsave"))
    {
        IFileDialogCustomize_GetCheckButtonState(custom, ID_COPY, &a);
        IFileDialogCustomize_GetCheckButtonState(custom, ID_DISABLED, &b);
        IFileDialogCustomize_GetCheckButtonState(custom, ID_PROFILE, &c);
        printf("%s: copy %d, disabled %d, profile %d\n", when, a, b, c);
    }
    else
    {
        hr = IFileDialogCustomize_GetSelectedControlItem(custom, ID_IMPORTAS, &item);
        IFileDialogCustomize_GetCheckButtonState(custom, ID_SEQUENCE, &a);
        IFileDialogCustomize_GetCheckButtonState(custom, ID_NOLABEL, &b);
        printf("%s: import-as item %lu (hr %#lx), sequence %d, no-label %d\n", when, item, hr, a, b);
    }
    IFileDialogCustomize_Release(custom);
}

static HRESULT WINAPI events_OnFileOk(IFileDialogEvents *iface, IFileDialog *dlg)
{
    IShellItem *item;
    WCHAR *path, *name = NULL;
    UINT type = 0;

    file_ok_calls++;
    IFileDialog_GetFileName(dlg, &name);
    IFileDialog_GetFileTypeIndex(dlg, &type);
    if (SUCCEEDED(IFileDialog_GetResult(dlg, &item)))
    {
        if (SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &path)))
        {
            printf("OnFileOk: %ls, name %ls, type %u\n", path, name, type);
            CoTaskMemFree(path);
        }
        IShellItem_Release(item);
    }
    else printf("OnFileOk: GetResult failed, name %ls, type %u\n", name, type);
    CoTaskMemFree(name);
    print_controls(dlg, "OnFileOk");
    if (!wcscmp(mode, L"itemreopen") && file_ok_calls == 1) return S_FALSE;
    return S_OK;
}
static HRESULT WINAPI events_OnFolderChanging(IFileDialogEvents *iface, IFileDialog *dlg, IShellItem *item) { return S_OK; }
static HRESULT WINAPI events_OnFolderChange(IFileDialogEvents *iface, IFileDialog *dlg) { return S_OK; }
static HRESULT WINAPI events_OnSelectionChange(IFileDialogEvents *iface, IFileDialog *dlg) { return S_OK; }
static HRESULT WINAPI events_OnShareViolation(IFileDialogEvents *iface, IFileDialog *dlg, IShellItem *item,
                                              FDE_SHAREVIOLATION_RESPONSE *resp) { return S_OK; }
static HRESULT WINAPI events_OnTypeChange(IFileDialogEvents *iface, IFileDialog *dlg)
{
    UINT type = 0;

    IFileDialog_GetFileTypeIndex(dlg, &type);
    printf("OnTypeChange: %u\n", type);
    type_changes++;
    return S_OK;
}
static HRESULT WINAPI events_OnOverwrite(IFileDialogEvents *iface, IFileDialog *dlg, IShellItem *item,
                                         FDE_OVERWRITE_RESPONSE *resp)
{
    WCHAR *path;

    if (SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &path)))
    {
        printf("OnOverwrite: %ls\n", path);
        CoTaskMemFree(path);
    }
    *resp = FDEOR_ACCEPT;
    return S_OK;
}

static IFileDialogEventsVtbl events_vtbl =
{
    events_QueryInterface, events_AddRef, events_Release, events_OnFileOk, events_OnFolderChanging,
    events_OnFolderChange, events_OnSelectionChange, events_OnShareViolation, events_OnTypeChange,
    events_OnOverwrite,
};

static struct events *impl_from_ctrl(IFileDialogControlEvents *iface)
{
    return CONTAINING_RECORD(iface, struct events, IFileDialogControlEvents_iface);
}
static HRESULT WINAPI cevents_QueryInterface(IFileDialogControlEvents *iface, REFIID riid, void **out)
{
    return events_QueryInterface(&impl_from_ctrl(iface)->IFileDialogEvents_iface, riid, out);
}
static ULONG WINAPI cevents_AddRef(IFileDialogControlEvents *iface) { return 2; }
static ULONG WINAPI cevents_Release(IFileDialogControlEvents *iface) { return 1; }
static HRESULT WINAPI cevents_OnItemSelected(IFileDialogControlEvents *iface, IFileDialogCustomize *c, DWORD ctl, DWORD item)
{
    printf("OnItemSelected: control %lu item %lu\n", ctl, item);
    return S_OK;
}
static HRESULT WINAPI cevents_OnButtonClicked(IFileDialogControlEvents *iface, IFileDialogCustomize *c, DWORD ctl) { return S_OK; }
static HRESULT WINAPI cevents_OnCheckButtonToggled(IFileDialogControlEvents *iface, IFileDialogCustomize *c, DWORD ctl, BOOL checked)
{
    printf("OnCheckButtonToggled: control %lu %d\n", ctl, checked);
    return S_OK;
}
static HRESULT WINAPI cevents_OnControlActivating(IFileDialogControlEvents *iface, IFileDialogCustomize *c, DWORD ctl) { return S_OK; }

static IFileDialogControlEventsVtbl cevents_vtbl =
{
    cevents_QueryInterface, cevents_AddRef, cevents_Release, cevents_OnItemSelected,
    cevents_OnButtonClicked, cevents_OnCheckButtonToggled, cevents_OnControlActivating,
};

static IShellItem *item_from_path(const WCHAR *path)
{
    IShellItem *item = NULL;
    HRESULT hr;

    if (FAILED(hr = SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void **)&item)))
        printf("SHCreateItemFromParsingName %ls failed %#lx\n", path, hr);
    return item;
}

static int show_item(IFileDialog *dlg, HWND parent)
{
    IShellItem *item;
    WCHAR *path, *name = NULL;
    UINT type = 0;
    HRESULT hr;

    print_controls(dlg, "before");
    hr = IFileDialog_Show(dlg, parent);
    printf("Show: %#lx, OnFileOk calls %d, OnTypeChange calls %d\n", hr, file_ok_calls, type_changes);
    print_controls(dlg, "after");
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) { printf("RESULT cancel\n"); return 0; }
    if (FAILED(hr)) { printf("RESULT fail Show %#lx\n", hr); return 1; }
    IFileDialog_GetFileName(dlg, &name);
    IFileDialog_GetFileTypeIndex(dlg, &type);
    printf("GetFileName: %ls, GetFileTypeIndex: %u\n", name, type);
    CoTaskMemFree(name);
    if (FAILED(hr = IFileDialog_GetResult(dlg, &item))) { printf("RESULT fail GetResult %#lx\n", hr); return 1; }
    IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &path);
    printf("RESULT ok %ls\n", path);
    CoTaskMemFree(path);
    IShellItem_Release(item);
    return 0;
}

static int test_item(void)
{
    static const COMDLG_FILTERSPEC filters[] = {{L"Images", L"*.png;*.exr"}, {L"All files", L"*.*"}};
    struct events events = {{&events_vtbl}, {&cevents_vtbl}};
    IFileDialogCustomize *custom;
    IFileDialog *dlg;
    DWORD cookie;
    HRESULT hr;
    int ret;

    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void **)&dlg);
    if (FAILED(hr)) { printf("RESULT fail CoCreateInstance %#lx\n", hr); return 1; }

    IFileDialog_SetTitle(dlg, L"Import File (portal smoke test)");
    IFileDialog_SetFileTypes(dlg, ARRAYSIZE(filters), filters);
    IFileDialog_QueryInterface(dlg, &IID_IFileDialogCustomize, (void **)&custom);
    IFileDialogCustomize_StartVisualGroup(custom, ID_GROUP, L"Import As:");
    IFileDialogCustomize_AddComboBox(custom, ID_IMPORTAS);
    IFileDialogCustomize_AddControlItem(custom, ID_IMPORTAS, 1, L"Footage");
    IFileDialogCustomize_AddControlItem(custom, ID_IMPORTAS, 2, L"Composition");
    IFileDialogCustomize_AddControlItem(custom, ID_IMPORTAS, 3, L"Composition - Retain Layer Sizes");
    IFileDialogCustomize_SetSelectedControlItem(custom, ID_IMPORTAS, 2);
    IFileDialogCustomize_EndVisualGroup(custom);
    IFileDialogCustomize_AddCheckButton(custom, ID_SEQUENCE, L"ImporterJPEG Sequence", FALSE);
    IFileDialogCustomize_AddCheckButton(custom, ID_NOLABEL, L"", TRUE);
    IFileDialogCustomize_AddText(custom, ID_FMTTEXT, L"Format:");
    IFileDialogCustomize_AddComboBox(custom, ID_FORMAT);
    IFileDialogCustomize_AddControlItem(custom, ID_FORMAT, 1, L"PNG");
    IFileDialogCustomize_AddControlItem(custom, ID_FORMAT, 2, L"TIFF");
    IFileDialogCustomize_SetSelectedControlItem(custom, ID_FORMAT, 1);
    IFileDialogCustomize_AddPushButton(custom, ID_CLOUD, L"Open from Adobe cloud storage");
    IFileDialogCustomize_Release(custom);
    IFileDialog_Advise(dlg, &events.IFileDialogEvents_iface, &cookie);

    ret = show_item(dlg, NULL);
    IFileDialog_Unadvise(dlg, cookie);
    IFileDialog_Release(dlg);
    return ret;
}

static int test_itemsave(void)
{
    static const COMDLG_FILTERSPEC filters[] =
        {{L"Photoshop (*.PSD;*.PDD;*.PSDT)", L"*.PSD;*.PDD;*.PSDT"}, {L"JPEG (*.JPG;*.JPEG;*.JPE)", L"*.jpg;*.jpeg;*.jpe"}};
    struct events events = {{&events_vtbl}, {&cevents_vtbl}};
    IFileDialogCustomize *custom;
    IFileDialog *dlg;
    IShellItem *folder;
    WCHAR path[MAX_PATH];
    DWORD cookie;
    HRESULT hr;
    int ret;

    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void **)&dlg);
    if (FAILED(hr)) { printf("RESULT fail CoCreateInstance %#lx\n", hr); return 1; }

    IFileDialog_SetTitle(dlg, L"Save As (portal smoke test)");
    IFileDialog_SetFileTypes(dlg, ARRAYSIZE(filters), filters);
    IFileDialog_SetFileTypeIndex(dlg, 1);
    IFileDialog_SetDefaultExtension(dlg, L"psd");
    IFileDialog_SetFileName(dlg, L"Untitled-1");
    IFileDialog_SetOkButtonLabel(dlg, L"&Save");
    path_of(L"out", path);
    if ((folder = item_from_path(path)))
    {
        IFileDialog_SetFolder(dlg, folder);
        IShellItem_Release(folder);
    }
    IFileDialog_QueryInterface(dlg, &IID_IFileDialogCustomize, (void **)&custom);
    IFileDialogCustomize_AddCheckButton(custom, ID_COPY, L"&As a Copy", FALSE);
    IFileDialogCustomize_AddCheckButton(custom, ID_DISABLED, L"Notes", FALSE);
    IFileDialogCustomize_SetControlState(custom, ID_DISABLED, CDCS_VISIBLE);
    IFileDialogCustomize_AddCheckButton(custom, ID_PROFILE, L"I&CC Profile:  sRGB IEC61966-2.1", TRUE);
    IFileDialogCustomize_Release(custom);
    IFileDialog_Advise(dlg, &events.IFileDialogEvents_iface, &cookie);

    ret = show_item(dlg, NULL);
    IFileDialog_Unadvise(dlg, cookie);
    IFileDialog_Release(dlg);
    return ret;
}

static int test_itemplain(DWORD options, const WCHAR *title)
{
    static const COMDLG_FILTERSPEC filters[] = {{L"Images", L"*.png"}, {L"All files", L"*.*"}};
    struct events events = {{&events_vtbl}, {&cevents_vtbl}};
    IFileDialog *dlg;
    IShellItem *folder;
    WCHAR path[MAX_PATH];
    DWORD cookie, fos;
    HRESULT hr;
    int ret;

    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void **)&dlg);
    if (FAILED(hr)) { printf("RESULT fail CoCreateInstance %#lx\n", hr); return 1; }
    IFileDialog_SetTitle(dlg, title);
    IFileDialog_GetOptions(dlg, &fos);
    IFileDialog_SetOptions(dlg, fos | options);
    if (!(options & FOS_PICKFOLDERS)) IFileDialog_SetFileTypes(dlg, ARRAYSIZE(filters), filters);
    path_of(L"link", path);
    if ((folder = item_from_path(path)))
    {
        IFileDialog_SetFolder(dlg, folder);
        IShellItem_Release(folder);
    }
    if (wcscmp(mode, L"itemlegacy")) IFileDialog_Advise(dlg, &events.IFileDialogEvents_iface, &cookie);
    if (!wcscmp(mode, L"itemfallback")) watch(title);
    ret = show_item(dlg, owner);
    if (wcscmp(mode, L"itemlegacy")) IFileDialog_Unadvise(dlg, cookie);
    IFileDialog_Release(dlg);
    return ret;
}

static void fill_combo(HWND hdlg, int id, const WCHAR **items)
{
    for (; *items; items++) SendDlgItemMessageW(hdlg, id, CB_ADDSTRING, 0, (LPARAM)*items);
    SendDlgItemMessageW(hdlg, id, CB_SETCURSEL, 0, 0);
}

static char events_seen[512];
static int reject;

static void seen(const char *name)
{
    if (strlen(events_seen) + strlen(name) + 2 < sizeof(events_seen))
    {
        if (*events_seen) strcat(events_seen, ",");
        strcat(events_seen, name);
    }
}

static void print_template(HWND hdlg, const char *when)
{
    printf("%s: settings %ld, slices %ld, metadata %ld, radio b %ld, dialog visible %d\n", when,
           (long)SendDlgItemMessageW(hdlg, ID_SETTINGS, CB_GETCURSEL, 0, 0),
           (long)SendDlgItemMessageW(hdlg, ID_SLICES, CB_GETCURSEL, 0, 0),
           (long)IsDlgButtonChecked(hdlg, ID_METADATA), (long)IsDlgButtonChecked(hdlg, ID_RADIO_B),
           IsWindowVisible(GetParent(hdlg)));
}

static UINT_PTR CALLBACK hook_procW(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static const WCHAR *settings[] = {L"PNG-24", L"JPEG High", L"GIF 128 Dithered", NULL};
    static const WCHAR *slices[] = {L"All Slices", L"Selected Slices", NULL};
    WCHAR buf[MAX_PATH];

    if (msg == WM_INITDIALOG)
    {
        OPENFILENAMEW *ofn = (OPENFILENAMEW *)lp;
        printf("hook WM_INITDIALOG: size %lu, custdata %#Ix\n", ofn->lStructSize, ofn->lCustData);
        fill_combo(hdlg, ID_SETTINGS, settings);
        fill_combo(hdlg, ID_SLICES, slices);
        CheckDlgButton(hdlg, ID_RADIO_A, BST_CHECKED);
        return TRUE;
    }
    if (msg != WM_NOTIFY) return FALSE;
    switch (((NMHDR *)lp)->code)
    {
    case CDN_INITDONE: seen("INITDONE"); break;
    case CDN_FOLDERCHANGE: seen("FOLDERCHANGE"); break;
    case CDN_SELCHANGE: seen("SELCHANGE"); break;
    case CDN_TYPECHANGE: seen("TYPECHANGE"); break;
    case CDN_FILEOK:
    {
        OPENFILENAMEW *ofn = ((OFNOTIFYW *)lp)->lpOFN;
        seen("FILEOK");
        printf("CDN_FILEOK: events %s\n", events_seen);
        print_template(hdlg, "CDN_FILEOK");
        SendMessageW(GetParent(hdlg), CDM_GETFILEPATH, MAX_PATH, (LPARAM)buf);
        printf("CDN_FILEOK: file path %ls, filter index %lu\n", buf, ofn->nFilterIndex);
        if (reject-- > 0)
        {
            printf("CDN_FILEOK: refused\n");
            SetWindowLongPtrW(hdlg, DWLP_MSGRESULT, 1);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

static UINT_PTR CALLBACK hook_procA(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    static const char *settings[] = {"PNG-24", "JPEG High", "GIF 128 Dithered", NULL};
    const char **s;
    char buf[MAX_PATH];

    if (msg == WM_INITDIALOG)
    {
        OPENFILENAMEA *ofn = (OPENFILENAMEA *)lp;
        printf("hook WM_INITDIALOG: size %lu, custdata %#Ix, filter %s\n", ofn->lStructSize, ofn->lCustData,
               ofn->lpstrFilter);
        for (s = settings; *s; s++) SendDlgItemMessageA(hdlg, ID_SETTINGS, CB_ADDSTRING, 0, (LPARAM)*s);
        SendDlgItemMessageA(hdlg, ID_SETTINGS, CB_SETCURSEL, 0, 0);
        SendDlgItemMessageA(hdlg, ID_SLICES, CB_ADDSTRING, 0, (LPARAM)"All Slices");
        SendDlgItemMessageA(hdlg, ID_SLICES, CB_SETCURSEL, 0, 0);
        return TRUE;
    }
    if (msg == WM_NOTIFY && ((NMHDR *)lp)->code == CDN_FILEOK)
    {
        OPENFILENAMEA *ofn = ((OFNOTIFYA *)lp)->lpOFN;
        print_template(hdlg, "CDN_FILEOK");
        SendMessageA(GetParent(hdlg), CDM_GETFILEPATH, MAX_PATH, (LPARAM)buf);
        printf("CDN_FILEOK: size %lu, file path %s\n", ofn->lStructSize, buf);
    }
    return FALSE;
}

static void *load_template(void)
{
    HRSRC res = FindResourceW(NULL, MAKEINTRESOURCEW(IDD_SAVEFORWEB), (LPCWSTR)RT_DIALOG);
    return res ? LockResource(LoadResource(NULL, res)) : NULL;
}

static void print_legacy(BOOL ret, const WCHAR *file, const OPENFILENAMEW *ofn)
{
    printf("returned %d, CommDlgExtendedError %#lx, offset %u, ext %u, filter index %lu\n",
           ret, CommDlgExtendedError(), ofn->nFileOffset, ofn->nFileExtension, ofn->nFilterIndex);
    if (ret && (ofn->Flags & OFN_ALLOWMULTISELECT))
    {
        const WCHAR *p;
        printf("RESULT ok");
        for (p = file; *p; p += wcslen(p) + 1) printf(" %ls|", p);
        printf("\n");
    }
    else if (ret) printf("RESULT ok %ls\n", file);
    else if (!CommDlgExtendedError()) printf("RESULT cancel\n");
    else printf("RESULT fail %#lx\n", CommDlgExtendedError());
}

static int test_legacy(BOOL save)
{
    WCHAR file[2048] = L"", title[64] = L"";
    OPENFILENAMEW ofn = {sizeof(ofn)};
    BOOL ret;

    if (save) lstrcpyW(file, L"untitled.aep");
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Projects (*.aep)\0*.aep\0All files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = file;
    ofn.nMaxFile = ARRAYSIZE(file);
    ofn.lpstrFileTitle = title;
    ofn.nMaxFileTitle = ARRAYSIZE(title);
    ofn.lpstrTitle = save ? L"Save As (portal smoke test)" : L"Open (portal smoke test)";
    ofn.lpstrInitialDir = L"Z:\\nonexistent\\folder";
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
                (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!wcscmp(mode, L"multi"))
    {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
        ofn.lpstrFilter = L"Images\0*.png;*.jpg\0";
    }
    if (!wcscmp(mode, L"savedef"))
    {
        ofn.lpstrDefExt = L"txt";
        ofn.lpstrFilter = L"Text (*.txt)\0*.txt\0Images (*.png;*.jpg)\0*.PNG;*.JPG\0";
    }
    if (!wcscmp(mode, L"fallback") || !wcscmp(mode, L"vanish") || !wcscmp(mode, L"off")) watch(ofn.lpstrTitle);
    ret = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    printf("file title %ls\n", title);
    print_legacy(ret, file, &ofn);
    return ret || !CommDlgExtendedError() ? 0 : 1;
}

static int test_hook(void)
{
    WCHAR file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {sizeof(ofn)};
    BOOL ret;

    reject = !wcscmp(mode, L"reject");
    ofn.hInstance = load_template();
    ofn.lpstrFilter = L"PNG-24 (*.PNG)\0*.PNG\0Images (*.PNG;*.JPG)\0*.PNG;*.JPG\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = file;
    ofn.nMaxFile = ARRAYSIZE(file);
    ofn.lpstrTitle = wcscmp(mode, L"hook") ? L"Load Settings (portal smoke test)" : L"";
    ofn.lCustData = 0xc0ffee;
    ofn.lpfnHook = hook_procW;
    ofn.Flags = 0x808804 | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATEHANDLE;
    if (!wcscmp(mode, L"hookauto")) watch(ofn.lpstrTitle);
    ret = GetOpenFileNameW(&ofn);
    print_legacy(ret, file, &ofn);
    return ret || !CommDlgExtendedError() ? 0 : 1;
}

static int test_hooka(void)
{
    char file[MAX_PATH] = "untitled.png";
    OPENFILENAMEA ofn = {sizeof(ofn)};
    BOOL ret;

    ofn.hInstance = load_template();
    ofn.lpstrFilter = "PNG-24 (*.png)\0*.png\0All files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = file;
    ofn.nMaxFile = ARRAYSIZE(file);
    ofn.lpstrDefExt = "png";
    ofn.lpstrTitle = "Save Optimized As (portal smoke test)";
    ofn.lCustData = 0xbeef;
    ofn.lpfnHook = hook_procA;
    ofn.Flags = 0x808804 | 2 | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATEHANDLE;
    ret = GetSaveFileNameA(&ofn);
    printf("returned %d, CommDlgExtendedError %#lx, offset %u, ext %u, filter index %lu\n",
           ret, CommDlgExtendedError(), ofn.nFileOffset, ofn.nFileExtension, ofn.nFilterIndex);
    if (ret) printf("RESULT ok %s\n", file);
    else if (!CommDlgExtendedError()) printf("RESULT cancel\n");
    else printf("RESULT fail %#lx\n", CommDlgExtendedError());
    return ret || !CommDlgExtendedError() ? 0 : 1;
}

static int CALLBACK browse_proc(HWND hwnd, UINT msg, LPARAM lp, LPARAM data)
{
    WCHAR path[MAX_PATH];

    if (msg == BFFM_INITIALIZED)
    {
        printf("BFFM_INITIALIZED\n");
        path_of(L"host", path);
        SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)path);
        SendMessageW(hwnd, BFFM_SETOKTEXT, 0, (LPARAM)L"Choose");
    }
    return 0;
}

static int test_browse(void)
{
    BROWSEINFOW bi = {0};
    WCHAR name[MAX_PATH] = L"", path[MAX_PATH] = L"";
    ITEMIDLIST *pidl;

    bi.hwndOwner = owner;
    bi.pszDisplayName = name;
    bi.lpszTitle = L"Choose a folder (portal smoke test)";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = browse_proc;
    if (!wcscmp(mode, L"browsefallback")) watch(NULL);
    pidl = SHBrowseForFolderW(&bi);
    if (!pidl) { printf("RESULT cancel\n"); return 0; }
    SHGetPathFromIDListW(pidl, path);
    printf("display name %ls\n", name);
    printf("RESULT ok %ls\n", path);
    CoTaskMemFree(pidl);
    return 0;
}

int wmain(int argc, WCHAR **argv)
{
    int ret;

    setvbuf(stdout, NULL, _IONBF, 0);
    lstrcpynW(mode, argc > 1 ? argv[1] : L"legacy", ARRAYSIZE(mode));
    if (argc > 2) dir = argv[2];
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (!wcscmp(mode, L"modal"))
    {
        owner = CreateWindowW(L"STATIC", L"portal test owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              0, 0, 300, 200, NULL, NULL, NULL, NULL);
        SetPropW(owner, L"__wine_x11_whole_window", (HANDLE)0x1234);
        SetTimer(NULL, 0, 100, timer_proc);
        ret = test_itemplain(0, L"Open item (portal smoke test)");
        printf("modal item: ticks %d, owner disabled %d, enabled after %d\n", ticks, ticks_disabled, IsWindowEnabled(owner));
        ticks = ticks_disabled = 0;
        ret |= test_legacy(FALSE);
        printf("modal legacy: ticks %d, owner disabled %d, enabled after %d\n", ticks, ticks_disabled, IsWindowEnabled(owner));
        return ret;
    }
    if (!wcscmp(mode, L"item")) return test_item();
    if (!wcscmp(mode, L"itemsave")) return test_itemsave();
    if (!wcscmp(mode, L"itemreopen") || !wcscmp(mode, L"itemlegacy") || !wcscmp(mode, L"itemfallback"))
        return test_itemplain(0, L"Open item (portal smoke test)");
    if (!wcscmp(mode, L"folder")) return test_itemplain(FOS_PICKFOLDERS, L"Select Folder (portal smoke test)");
    if (!wcscmp(mode, L"browse") || !wcscmp(mode, L"browsefallback")) return test_browse();
    if (!wcscmp(mode, L"hook") || !wcscmp(mode, L"reject") || !wcscmp(mode, L"hookauto")) return test_hook();
    if (!wcscmp(mode, L"hooka")) return test_hooka();
    if (!wcscmp(mode, L"save") || !wcscmp(mode, L"savedef")) return test_legacy(TRUE);
    return test_legacy(FALSE);
}
