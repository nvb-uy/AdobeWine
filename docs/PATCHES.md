# Patches

AdobeWine is upstream Wine 11.18 plus these patches, applied in order. Each one fixes a
difference between Wine and Windows found by running the same Adobe workflow on both. How each
problem was found, with measurements and the test programs (in `tests/`), is in
[DEVLOG.md](DEVLOG.md); the method is in [HOW-IT-WORKS.md](HOW-IT-WORKS.md). Patches marked AdobeWine are meant to go upstream
to WineHQ; help with that is very welcome.

| # | Wine component | What it fixes or adds | Origin |
|---|---|---|---|
| 0001 | libs | Embedded xml decl | PhialsBasement |
| 0002 | msvcrt | Unlinkframe null | PhialsBasement |
| 0003 | d2d1 | Geometry realization bitmap rt | PhialsBasement |
| 0004 | user32 | Stub SetWindowFeedbackSetting | AdobeWine |
| 0005 | mshtml | Honour FEATURE_BROWSER_EMULATION | AdobeWine |
| 0006 | mshtml | Implement queryCommandSupported Enabled | AdobeWine |
| 0007 | mshtml | Location.port empty for default port | AdobeWine |
| 0008 | appxdeploymentclient | AddPackageAsync reports completion | AdobeWine |
| 0009 | kernelbase | Child inherits SetDllDirectory | AdobeWine |
| 0010 | dwrite | Resolve system font paths via data dir | AdobeWine |
| 0011 | winex11.drv | Client surface present uses toplevel visual | AdobeWine |
| 0012 | mshtml | Add window.atob and btoa | AdobeWine |
| 0013 | jscript | String.split empty separator embedded nulls | AdobeWine |
| 0014 | dxcore | Attributes and IsIntegrated IsDetachable | AdobeWine |
| 0015 | ntdll | Do not hold FLS lock across thread exit callbacks | AdobeWine |
| 0016 | ntdll | Heap fail instead of crashing on unmapped pointer | AdobeWine |
| 0017 | dwrite | Implement IDWriteTextLayout HitTestPoint | AdobeWine |
| 0018 | ntdll, winecfg | Windows 11 as build 26100 24H2 | AdobeWine |
| 0019 | win32u | Re present offscreen client surfaces after window surface flush | AdobeWine |
| 0020 | win32u | Re copy offscreen vulkan surfaces after sparse presents | AdobeWine |
| 0021 | d2d1 | Fix centre of rotated arcs bone shaped buttons | AdobeWine |
| 0022 | winmm | Init devices before answering preferred device queries | AdobeWine |
| 0023 |  | Heap skip corrupted free blocks instead of faulting under lock | AdobeWine |
| 0024 | d2d1 | Implement layers masks and opacity via offscreen bitmap | AdobeWine |
| 0025 | include, winex11.drv | Keep offscreen client windows at swapchain size and stretch | AdobeWine |
| 0026 | dwrite | Let trailing whitespace hang when wrapping lines | AdobeWine |
| 0027 | ntdll | Heap survive app heap corruption rebuild free lists keep subheaps | AdobeWine |
| 0028 | winhttp | Long header lines and no credentials on cross host redirect | AdobeWine |
| 0029 | windows.storage | Stream references clones and content type | AdobeWine |
| 0030 | winex11.drv | Manage popups owned by layered windows | AdobeWine |
| 0031 | windows.ui | Toast notification manager | AdobeWine |
| 0032 | ntdll | Heap quarantine for listed processes | AdobeWine |
| 0033 | rtworkq | Shut queues down without holding queues_section | AdobeWine |
| 0034 | d2d1 | Layer bitmaps are premultiplied | AdobeWine |
| 0035 | mshtml | MutationObserver and local script load events | AdobeWine |
| 0036 | ntdll, wow64 | Ntdll TokenLinkedToken size query | AdobeWine |
| 0037 | kernelbase | Token process from service on user desktop | AdobeWine |
| 0038 | include, ntdll, server | Ntdll token integrity levels | AdobeWine |
| 0039 | winhttp | Return available data and drop raw content length when decompressing | AdobeWine |
| 0040 | win32u | Font cache family name | AdobeWine |
| 0041 | rtworkq | Serial queue lifetime | AdobeWine |
| 0042 | d2d1 | No join for straight continuation | AdobeWine |
| 0043 | win32u | Pick up fonts added by other processes | AdobeWine |
| 0044 | ntdll | Wow64 remote breakin | AdobeWine |
| 0045 | win32u | Dialogwin trace channel | AdobeWine |
| 0046 | winhttp | Queue async receive response | AdobeWine |
| 0047 | gdi32, include, win32u, wow64win | D3dkmt open adapter from hdc and vblank wait | AdobeWine |
| 0048 | gdiplus | Honour padded source stride when blending PARGB to a DC | AdobeWine |
| 0049 | gdi32 | CreateBitmapIndirect reads rows bmWidthBytes apart | AdobeWine |
| 0050 | dxgi | Composition swapchains present to the DirectComposition target | AdobeWine |
| 0051 | dcomp | Minimal device target and visual for swapchain content | AdobeWine |
| 0052 | dwrite | Last resort font uses the system collection for ranges without one | AdobeWine |
| 0053 | win32u, winex11.drv | Recreated swapchains get the window size not the old offscreen size | AdobeWine |
| 0055 | mshtml | ChildNode remove, replaceWith, before, after (cause found by NickPittas, #2) | AdobeWine |
| 0056 | mshtml | HTMLInputElement indeterminate is stored instead of E_NOTIMPL (NickPittas, #2) | AdobeWine |
| 0057 | jscript | Object.entries, Object.values, Object.assign in ES5+ modes (NickPittas, #2) | AdobeWine |
| 0058 | win32u | Implement DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL | switch87 |
| 0060a | comdlg32 | Experimental: XDG desktop portal file dialogs (your desktop's own file picker), off unless `WINE_FORCE_PORTAL=1` or `HKCU\Software\Wine\X11 Driver\FileDialogPortal` = `always`/`auto` | NickPittas (AE4Linux); Wine MR 10060 by Alexander Wilms |
| 0060b | comdlg32 | Portal dialogs: custom controls as choices (After Effects Import As / Sequence), document portal host paths | NickPittas (AE4Linux) |
| 0061 | d2d1 | Insert triangulation segments iteratively and give up when they do not converge (After Effects' Render Queue panel overflowed the stack) | AdobeWine |
| 0062 | server | Read and write process memory through a live thread id (winedbg on processes whose main thread exited) | AdobeWine |
| 0063 | dwrite | No FreeType auto-hinting, unhinted glyph run outlines as on Windows | nvb-uy |
| 0064 | dwrite | Quarter-pixel glyph positions in natural modes instead of truncated ones (uneven letter spacing) | nvb-uy |
| 0065 | win32u | No FreeType auto-hinting for GDI text | nvb-uy |
| 0066 | d2d1 | Draw to a multisampled copy of the target bitmap (`WINE_D2D_MSAA`, default 8, 0 off) | nvb-uy |
| 0067 | d2d1 | Analytic coverage for curve fills and strokes | nvb-uy |
| 0068a | include | SmoothingModeAntiAlias8x4 and SmoothingModeAntiAlias8x8 | Wine MR 12221 by Thomas Ritter |
| 0068b | gdiplus | Antialiased filled paths (Photoshop toolbar pills and icons) | Wine MR 12221 by Thomas Ritter |
| 0068c | gdiplus | Antialiased thin lines one pixel wide | Wine MR 12221 by Thomas Ritter |
| 0068d | gdiplus | Antialiased lines widened with the default flatness | Wine MR 12221 by Thomas Ritter |
| 0069 | win32u | Offscreen swapchains present immediately and vsync is paced to the display refresh rate, 59 to 237 fps at 240 Hz under XWayland (`WINE_OFFSCREEN_PACING`, `WINE_OFFSCREEN_REFRESH`) | nvb-uy |
| 0070 | win32u | Copy waited-for offscreen presents only after they completed | nvb-uy |
| 0071 | msvcp140 | Concurrency::details::platform::GetNextAsyncId (AdobeNotificationHelper aborted on the stub) | nvb-uy |
| 0072 | comdlg32 | Portal helpers shared by the Explorer and IFileDialog dialogs, one request structure for open and save | nvb-uy |
| 0073 | comdlg32 | Portal dialogs wait on a worker thread while the app keeps dispatching messages, modal to the owner, which is passed as the parent window | nvb-uy |
| 0074 | comdlg32 | Portal requests listen for their response before sending, give up when the portal goes away, and the FileChooser version is probed first | nvb-uy |
| 0075 | comdlg32 | Portal file types match regardless of case (`*.PSD` shows .psd files) and the type picked in the desktop dialog is returned | nvb-uy |
| 0076 | comdlg32 | Only existing folders and files are sent to the portal, resolved (~/Documents rather than the prefix's link to it) | nvb-uy |
| 0077 | comdlg32 | GetOpenFileName and GetSaveFileName drive the portal from a hidden Explorer dialog, so hooks, templates, CDN_FILEOK and ANSI structures keep working | nvb-uy |
| 0078 | comdlg32 | Check boxes, combo boxes and radio buttons of dialog templates (Save for Web, filter plugins) become portal choices | nvb-uy |
| 0079 | comdlg32 | Only enabled IFileDialog custom controls become portal choices, labels without & mnemonics | nvb-uy |
| 0080 | comdlg32 | Portal save names get the extension of the chosen type, OnOverwrite is sent, GetFileName has no folder | nvb-uy |
| 0081 | comdlg32 | The portal comes back when OnFileOk refuses the chosen file | nvb-uy |
| 0082 | comdlg32 | The OK button label goes to the portal as its accept label | nvb-uy |
| 0083 | shell32 | SHBrowseForFolder uses the IFileDialog folder picker, and so the portal, when the file dialog portal is enabled | nvb-uy |
| 0085 | wineboot | No "configuration is being updated" window while the prefix is updated | nvb-uy |
| 0086 | user32 | Windows without an icon get the executable's icon instead of the Wine logo | nvb-uy |
| 0087 | win32u | `Decorated=owned` in the X11 Driver key lets the window manager decorate owned windows (dialogs) only | nvb-uy |
| 0088 | explorer | Systray icon windows are named after their tooltip, so XEmbed hosts like xembedsniproxy can show a name | nvb-uy |
| 0089 | gdiplus | Rectangle path regions are built from their corners instead of a sorted edge list (Photoshop Home visibility tests, 113 to 1 ms a frame) | nvb-uy |
| 0090 | gdiplus | Region visibility tests build the region only inside the tested rectangle (non-rectangular clips, 917 tests 1982 to 30 ms) | nvb-uy |
| 0091 | gdiplus | Antialiased fills add up coverage a pixel at a time and scale alpha with shifts instead of dividing every sample run | nvb-uy |
| 0092 | gdiplus | Antialiased fills of axis-aligned rectangles take their coverage from the covered sample rows and columns; pixel-aligned ones skip sampling and the alpha conversion | nvb-uy |
| 0093 | win32u | 32-bpp alpha blends copy opaque source pixels and skip fully transparent ones | nvb-uy |
| 0094 | gdiplus | ARGB to premultiplied conversion reuses the last result while pixels repeat and copies opaque ones | nvb-uy |
