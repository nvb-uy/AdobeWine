#include <windows.h>
#include <stdio.h>

#define W 800
#define H 40

static DWORD *bits;

static void clear(void)
{
    memset(bits, 0xff, W * H * 4);
}

static int ink_right(void)
{
    int x, y;
    for (x = W - 1; x >= 0; x--)
        for (y = 0; y < H; y++)
            if ((bits[y * W + x] & 0xffffff) != 0xffffff) return x;
    return -1;
}

static int ink_count(void)
{
    int i, n = 0;
    for (i = 0; i < W * H; i++)
        if ((bits[i] & 0xffffff) != 0xffffff) n++;
    return n;
}

static int advance_diffs(HDC dc, UINT format)
{
    static const MAT2 identity = {{0,1},{0,0},{0,0},{0,1}};
    GLYPHMETRICS a, b;
    int c, n = 0;
    for (c = 32; c < 127; c++)
    {
        if (GetGlyphOutlineW(dc, c, GGO_METRICS, &a, 0, NULL, &identity) == GDI_ERROR) continue;
        if (GetGlyphOutlineW(dc, c, format, &b, 0, NULL, &identity) == GDI_ERROR) continue;
        if (a.gmCellIncX != b.gmCellIncX) n++;
    }
    return n;
}

int main(void)
{
    static const struct { const WCHAR *face; int height; } fonts[] =
    {
        {L"Tahoma", -11}, {L"Tahoma", -12}, {L"Tahoma", -13}, {L"Tahoma", -15}, {L"Tahoma", -20},
        {L"Arial", -12}, {L"Verdana", -12}, {L"Segoe UI", -12}, {L"MS Shell Dlg", -11},
    };
    static const struct { BYTE quality; const char *name; } qualities[] =
    {
        {NONANTIALIASED_QUALITY, "mono"}, {ANTIALIASED_QUALITY, "gray"}, {CLEARTYPE_QUALITY, "cleartype"},
    };
    static const WCHAR *strings[] =
    {
        L"The object \x201c<unknown> menu item\x201d is not",
        L"Open from Adobe cloud storage",
        L"Could not complete your request because it is not without",
    };
    BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER), W, -H, 1, 32, BI_RGB}};
    HDC dc = CreateCompatibleDC(0);
    HBITMAP bmp = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    unsigned int f, q, s;
    SIZE sz;

    SelectObject(dc, bmp);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));

    for (f = 0; f < ARRAYSIZE(fonts); f++)
    {
        for (q = 0; q < ARRAYSIZE(qualities); q++)
        {
            LOGFONTW lf = {0};
            HFONT font, old;
            WCHAR face[LF_FACESIZE];

            lf.lfHeight = fonts[f].height;
            lf.lfQuality = qualities[q].quality;
            wcscpy(lf.lfFaceName, fonts[f].face);
            font = CreateFontIndirectW(&lf);
            old = SelectObject(dc, font);
            GetTextFaceW(dc, LF_FACESIZE, face);
            printf("%ls %d %s (%ls): metrics vs bitmap %d, vs gray8 %d", fonts[f].face, fonts[f].height,
                   qualities[q].name, face, advance_diffs(dc, GGO_BITMAP), advance_diffs(dc, GGO_GRAY8_BITMAP));
            GetTextExtentPoint32W(dc, L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &sz);
            printf(", alphabet %ld (avg %ld)\n", sz.cx, (sz.cx / 26 + 1) / 2);

            for (s = 0; s < ARRAYSIZE(strings); s++)
            {
                int len = lstrlenW(strings[s]), ink, clipped, unclipped;
                RECT rc = {10, 0, 810, H};

                GetTextExtentPoint32W(dc, strings[s], len, &sz);
                clear();
                ExtTextOutW(dc, 10, 0, 0, NULL, strings[s], len, NULL);
                ink = ink_right() - 10 + 1;

                DrawTextW(dc, strings[s], len, &rc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
                clear();
                DrawTextW(dc, strings[s], len, &rc, DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);
                unclipped = ink_count();
                clear();
                DrawTextW(dc, strings[s], len, &rc, DT_WORDBREAK | DT_NOPREFIX);
                clipped = ink_count();

                printf("    string %u width %3ld ink %3d over %+d calcrect %ld lost %d\n", s, sz.cx, ink, ink - (int)sz.cx,
                       rc.right - rc.left, unclipped - clipped);
            }
            SelectObject(dc, old);
            DeleteObject(font);
        }
    }
    printf("dialog base units %#lx\n", GetDialogBaseUnits());
    return 0;
}
