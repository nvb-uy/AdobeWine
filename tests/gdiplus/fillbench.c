#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int GpStatus;
typedef void GpGraphics, GpBrush, GpPath, GpImage, GpBitmap;
typedef struct { UINT32 GdiplusVersion; void *cb; BOOL a, b; } GdiplusStartupInput;
GpStatus WINAPI GdiplusStartup(ULONG_PTR *, const GdiplusStartupInput *, void *);
GpStatus WINAPI GdipCreateFromHDC(HDC, GpGraphics **);
GpStatus WINAPI GdipDeleteGraphics(GpGraphics *);
GpStatus WINAPI GdipSetSmoothingMode(GpGraphics *, int);
GpStatus WINAPI GdipTranslateWorldTransform(GpGraphics *, float, float, int);
GpStatus WINAPI GdipResetWorldTransform(GpGraphics *);
GpStatus WINAPI GdipCreateSolidFill(DWORD, GpBrush **);
GpStatus WINAPI GdipDeleteBrush(GpBrush *);
GpStatus WINAPI GdipFillRectangle(GpGraphics *, GpBrush *, float, float, float, float);
GpStatus WINAPI GdipFillPath(GpGraphics *, GpBrush *, GpPath *);
GpStatus WINAPI GdipCreatePath(int, GpPath **);
GpStatus WINAPI GdipDeletePath(GpPath *);
GpStatus WINAPI GdipAddPathRectangle(GpPath *, float, float, float, float);
GpStatus WINAPI GdipAddPathArc(GpPath *, float, float, float, float, float, float);
GpStatus WINAPI GdipClosePathFigure(GpPath *);
GpStatus WINAPI GdipSetClipPath(GpGraphics *, GpPath *, int);
GpStatus WINAPI GdipSaveGraphics(GpGraphics *, UINT *);
GpStatus WINAPI GdipRestoreGraphics(GpGraphics *, UINT);
GpStatus WINAPI GdipIsVisibleRect(GpGraphics *, float, float, float, float, BOOL *);
GpStatus WINAPI GdipCreateBitmapFromScan0(INT, INT, INT, int, BYTE *, GpBitmap **);
GpStatus WINAPI GdipDrawImageRectRect(GpGraphics *, GpImage *, float, float, float, float, float, float, float, float, int, void *, void *, void *);
GpStatus WINAPI GdipDisposeImage(GpImage *);
GpStatus WINAPI GdipSetInterpolationMode(GpGraphics *, int);
GpStatus WINAPI GdipSetPixelOffsetMode(GpGraphics *, int);
GpStatus WINAPI GdipCreateLineBrush(const float *, const float *, DWORD, DWORD, int, GpBrush **);

static double now(void) { LARGE_INTEGER f, c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c); return c.QuadPart * 1000.0 / f.QuadPart; }

static GpPath *round_rect(float x, float y, float w, float h, float r)
{
    GpPath *p;
    GdipCreatePath(0, &p);
    GdipAddPathArc(p, x, y, 2 * r, 2 * r, 180, 90);
    GdipAddPathArc(p, x + w - 2 * r, y, 2 * r, 2 * r, 270, 90);
    GdipAddPathArc(p, x + w - 2 * r, y + h - 2 * r, 2 * r, 2 * r, 0, 90);
    GdipAddPathArc(p, x, y + h - 2 * r, 2 * r, 2 * r, 90, 90);
    GdipClosePathFigure(p);
    return p;
}

static GpBitmap *pattern_image(int w, int h, int format, int bpp, BYTE **data)
{
    int stride = (w * bpp / 8 + 3) & ~3, x, y;
    GpBitmap *img;
    *data = malloc(stride * h);
    for (y = 0; y < h; y++)
        for (x = 0; x < w * bpp / 8; x++)
            (*data)[y * stride + x] = (x * 7 + y * 13 + ((x * y) >> 5)) & 0xff;
    if (bpp == 32)
        for (y = 0; y < h; y++)
            for (x = 0; x < w; x++)
                (*data)[y * stride + x * 4 + 3] = (x + y) & 64 ? 0xff : (x * 3 + y) & 0xff;
    GdipCreateBitmapFromScan0(w, h, stride, format, *data, &img);
    return img;
}

static int home(int frames, int smoothing)
{
    int W = 1481, H = 925, f, i;
    ULONG_PTR tok; GdiplusStartupInput in = {1};
    HDC screen = GetDC(0), dc = CreateCompatibleDC(screen);
    HBITMAP bmp = NULL, old = NULL;
    BITMAPINFO bi = {{sizeof(BITMAPINFOHEADER), W, -H, 1, 32, BI_RGB}};
    double t_bg = 0, t_rr = 0, t_grad = 0, t_vis = 0, t_img = 0, t0, start;
    GpBitmap *big, *mid, *small, *wide, *icon;
    BYTE *d1, *d2, *d3, *d4, *d5;
    DWORD *px, crc = 0x12345678;
    GdiplusStartup(&tok, &in, NULL);
    big = pattern_image(1200, 1200, 0x21808, 24, &d1);
    mid = pattern_image(600, 600, 0x26200A, 32, &d2);
    small = pattern_image(240, 240, 0xE200B, 32, &d3);
    wide = pattern_image(512, 320, 0x21808, 24, &d4);
    icon = pattern_image(24, 24, 0x26200A, 32, &d5);
    start = now();
    for (f = 0; f < frames; f++)
    {
        GpGraphics *g; GpBrush *b, *b2, *bg, *gr; GpPath *p; UINT st; BOOL vis;
        if (bmp) { SelectObject(dc, old); DeleteObject(bmp); }
        bmp = CreateCompatibleBitmap(screen, W, H);
        old = SelectObject(dc, bmp);
        GdipCreateFromHDC(dc, &g);
        GdipSetSmoothingMode(g, smoothing);
        GdipSetPixelOffsetMode(g, 4);
        GdipTranslateWorldTransform(g, -260.5f, -64.5f, 0);
        GdipCreateSolidFill(0xff535353, &bg);
        GdipCreateSolidFill(f & 1 ? 0xff3a3a3a : 0xe0454545, &b);
        GdipCreateSolidFill(f & 1 ? 0x40ffffff : 0x80e8a039, &b2);
        t0 = now();
        for (i = 0; i < 3; i++) GdipFillRectangle(g, bg, 260, 64, W, H);
        t_bg += now() - t0;
        t0 = now();
        {
            static const float r[][5] = {{312,100,1377,198,8},{312,320,1377,198,8},{312,540,1377,198,8},{312,300,1377,600,6},
                {340,120,989,346,4},{340,500,958,189,6},{345,505,953,184,6}};
            for (i = 0; i < 7; i++) { p = round_rect(r[i][0], r[i][1], r[i][2], r[i][3], r[i][4]); GdipFillPath(g, i & 1 ? b2 : b, p); GdipDeletePath(p); }
        }
        t_rr += now() - t0;
        t0 = now();
        {
            static const float r[][4] = {{312,100,1377,198},{312,860,1377,40},{400,200,300,32},{720,200,300,32},{1040,200,300,32},{340,700,640,120}};
            for (i = 0; i < 6; i++)
            {
                float p1[2] = {r[i][0], r[i][1]}, p2[2] = {r[i][0], r[i][1] + r[i][3]};
                if (i >= 2 && i < 5) { p2[0] = r[i][0] + r[i][2]; p2[1] = r[i][1]; }
                if (i == 5) { p2[0] = r[i][0] + r[i][2]; }
                GdipCreateLineBrush(p1, p2, 0xff2b2b2b, i & 1 ? 0x00000000 : 0xff6a6a6a, 0, &gr);
                GdipFillRectangle(g, gr, r[i][0], r[i][1], r[i][2], r[i][3]);
                GdipDeleteBrush(gr);
            }
        }
        t_grad += now() - t0;
        t0 = now();
        GdipSaveGraphics(g, &st);
        GdipCreatePath(0, &p); GdipAddPathRectangle(p, 260, 64, W, H); GdipSetClipPath(g, p, 1); GdipDeletePath(p);
        for (i = 0; i < 1050; i++)
        {
            UINT st2;
            if (!(i % 12)) { GdipSaveGraphics(g, &st2); GdipCreatePath(0, &p); GdipAddPathRectangle(p, 260 + (i % 7) * 100, 64 + (i % 5) * 150, 120, 120); GdipSetClipPath(g, p, 1); GdipDeletePath(p); }
            GdipIsVisibleRect(g, 260 + (i * 37) % 1400, 64 + (i * 53) % 900, 120, 40, &vis);
            if (!(i % 12)) GdipRestoreGraphics(g, st2);
        }
        GdipRestoreGraphics(g, st);
        t_vis += now() - t0;
        t0 = now();
        GdipSetInterpolationMode(g, 7);
        for (i = 0; i < 3; i++) GdipDrawImageRectRect(g, big, 340 + i * 60, 140, 41, 41, 0, 0, 1200, 1200, 2, NULL, NULL, NULL);
        GdipDrawImageRectRect(g, mid, 600, 140, 120, 120, 0, 0, 600, 600, 2, NULL, NULL, NULL);
        GdipDrawImageRectRect(g, small, 760, 140, 48, 48, 0, 0, 240, 240, 2, NULL, NULL, NULL);
        GdipDrawImageRectRect(g, wide, 840, 140, 256, 160, 0, 0, 512, 320, 2, NULL, NULL, NULL);
        for (i = 0; i < 214; i++) GdipDrawImageRectRect(g, icon, 260 + (i * 41) % 1400, 64 + (i * 29) % 880, 24, 24, 0, 0, 24, 24, 2, NULL, NULL, NULL);
        t_img += now() - t0;
        GdipDeleteBrush(bg);
        GdipDeleteBrush(b);
        GdipDeleteBrush(b2);
        GdipDeleteGraphics(g);
    }
    t0 = now() - start;
    px = malloc(W * H * 4);
    SelectObject(dc, old);
    GetDIBits(dc, bmp, 0, H, px, &bi, DIB_RGB_COLORS);
    for (i = 0; i < W * H; i++) crc = (crc ^ px[i]) * 16777619u;
    printf("home pixels %08lx\n", crc);
    printf("per frame: total %.2f ms | 3 bg fills %.2f | 7 rounded fills %.2f | 6 gradients %.2f | 1050 IsVisibleRect %.2f | 220 images %.2f\n",
           t0 / frames, t_bg / frames, t_rr / frames, t_grad / frames, t_vis / frames, t_img / frames);
    return 0;
}

int main(int argc, char **argv)
{
    int W = 1741, H = 989, frames = argc > 1 ? atoi(argv[1]) : 10, f, i;
    ULONG_PTR tok; GdiplusStartupInput in = {1};
    BITMAPINFO bi = {{sizeof(BITMAPINFOHEADER), W, -H, 1, 32, BI_RGB}};
    void *bits; HDC dc = CreateCompatibleDC(0);
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    double t_bg = 0, t_rr = 0, t_vis = 0, t_img = 0, t0, start;
    static BYTE icon[24 * 24 * 4];
    GpBitmap *img;
    if (argc > 3 && !strcmp(argv[3], "home")) return home(frames, argc > 2 ? atoi(argv[2]) : 4);
    GdiplusStartup(&tok, &in, NULL);
    SelectObject(dc, bmp);
    for (i = 0; i < 24 * 24; i++) ((DWORD *)icon)[i] = 0x80ffffff;
    GdipCreateBitmapFromScan0(24, 24, 96, 0x26200A, icon, &img);
    start = now();
    for (f = 0; f < frames; f++)
    {
        GpGraphics *g; GpBrush *b, *b2; GpPath *p; UINT st; BOOL vis;
        GdipCreateFromHDC(dc, &g);
        GdipSetSmoothingMode(g, argc > 2 ? atoi(argv[2]) : 4);
        GdipTranslateWorldTransform(g, -0.5f, -0.5f, 0);
        GdipCreateSolidFill(f & 1 ? 0xff1e1e1e : 0xc0396ee8, &b);
        GdipCreateSolidFill(f & 1 ? 0xff3a3a3a : 0x80e8a039, &b2);
        t0 = now();
        {
            static const float r[][4] = {{0,0,1741,989},{0,0,1741,989},{0,0,1741,989},{260,64,1377,644},{300,600,448.33f,120},
                {760,600,448.33f,120},{1220,600,448.33f,120},{300,500,957,188},{300,300,700,116},{310,310,674,114},
                {260,700,1377,197.8f},{260,700,1377,197.8f},{260,700,1377,197.8f},{1700,64,8,925}};
            for (i = 0; i < 14; i++) GdipFillRectangle(g, i & 1 ? b2 : b, r[i][0], r[i][1], r[i][2], r[i][3]);
        }
        t_bg += now() - t0;
        t0 = now();
        {
            static const float r[][5] = {{300,200,989,346,4},{300,560,278,57,4},{600,560,278,57,4},{900,560,278,57,4},{40,120,208,247,7},
                {20,100,113,32,16},{20,140,131,32,16},{20,180,122,32,16},{40,400,20,20,4},{40,430,39,23.6f,8}};
            for (i = 0; i < 10; i++) { p = round_rect(r[i][0], r[i][1], r[i][2], r[i][3], r[i][4]); GdipFillPath(g, b2, p); GdipDeletePath(p); }
            for (i = 0; i < 12; i++) { p = round_rect(40 + i * 30, 900, 12, 12, 3); GdipFillPath(g, b2, p); GdipDeletePath(p); }
        }
        t_rr += now() - t0;
        t0 = now();
        GdipSaveGraphics(g, &st);
        GdipCreatePath(0, &p); GdipAddPathRectangle(p, 0, 0, W, H); GdipSetClipPath(g, p, 1); GdipDeletePath(p);
        for (i = 0; i < 917; i++)
        {
            UINT st2;
            if (!(i % 12)) { GdipSaveGraphics(g, &st2); GdipCreatePath(0, &p); GdipAddPathRectangle(p, 260 + (i % 7) * 100, 64 + (i % 5) * 150, 120, 120); GdipSetClipPath(g, p, 1); GdipDeletePath(p); }
            GdipIsVisibleRect(g, (i * 37) % 1700, (i * 53) % 950, 120, 40, &vis);
            if (!(i % 12)) GdipRestoreGraphics(g, st2);
        }
        GdipRestoreGraphics(g, st);
        t_vis += now() - t0;
        t0 = now();
        for (i = 0; i < 97; i++) GdipDrawImageRectRect(g, img, (i * 41) % 1700, (i * 29) % 950, 24, 24, 0, 0, 24, 24, 2, NULL, NULL, NULL);
        t_img += now() - t0;
        GdipDeleteBrush(b);
        GdipDeleteBrush(b2);
        GdipDeleteGraphics(g);
    }
    {
        DWORD crc = 0x12345678; const DWORD *px = bits;
        for (i = 0; i < W * H; i++) crc = (crc ^ px[i]) * 16777619u;
        printf("pixels %08lx\n", crc);
    }
    printf("per frame: total %.1f ms | 14 rect fills %.1f | 22 rounded fills %.1f | 917 IsVisibleRect %.1f | 97 icons %.1f\n",
           (now() - start) / frames, t_bg / frames, t_rr / frames, t_vis / frames, t_img / frames);
    return 0;
}
