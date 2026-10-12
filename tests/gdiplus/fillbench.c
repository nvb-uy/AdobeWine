#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

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
