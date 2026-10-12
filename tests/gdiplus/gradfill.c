#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int GpStatus;
typedef void GpGraphics, GpBrush, GpImage, GpBitmap;
typedef struct { UINT32 GdiplusVersion; void *cb; BOOL a, b; } GdiplusStartupInput;
typedef struct { float X, Y; } PointF;
typedef struct { float X, Y, Width, Height; } RectF;
GpStatus WINAPI GdiplusStartup(ULONG_PTR *, const GdiplusStartupInput *, void *);
GpStatus WINAPI GdipCreateFromHDC(HDC, GpGraphics **);
GpStatus WINAPI GdipGetImageGraphicsContext(GpImage *, GpGraphics **);
GpStatus WINAPI GdipDeleteGraphics(GpGraphics *);
GpStatus WINAPI GdipSetSmoothingMode(GpGraphics *, int);
GpStatus WINAPI GdipSetPixelOffsetMode(GpGraphics *, int);
GpStatus WINAPI GdipTranslateWorldTransform(GpGraphics *, float, float, int);
GpStatus WINAPI GdipRotateWorldTransform(GpGraphics *, float, int);
GpStatus WINAPI GdipScaleWorldTransform(GpGraphics *, float, float, int);
GpStatus WINAPI GdipGraphicsClear(GpGraphics *, DWORD);
GpStatus WINAPI GdipCreateLineBrush(const PointF *, const PointF *, DWORD, DWORD, int, GpBrush **);
GpStatus WINAPI GdipCreateLineBrushFromRectWithAngle(const RectF *, DWORD, DWORD, float, BOOL, int, GpBrush **);
GpStatus WINAPI GdipSetLineGammaCorrection(GpBrush *, BOOL);
GpStatus WINAPI GdipSetLineBlend(GpBrush *, const float *, const float *, INT);
GpStatus WINAPI GdipSetLinePresetBlend(GpBrush *, const DWORD *, const float *, INT);
GpStatus WINAPI GdipScaleLineTransform(GpBrush *, float, float, int);
GpStatus WINAPI GdipRotateLineTransform(GpBrush *, float, int);
GpStatus WINAPI GdipDeleteBrush(GpBrush *);
GpStatus WINAPI GdipFillRectangle(GpGraphics *, GpBrush *, float, float, float, float);
GpStatus WINAPI GdipFillEllipse(GpGraphics *, GpBrush *, float, float, float, float);
GpStatus WINAPI GdipCreateBitmapFromScan0(INT, INT, INT, int, BYTE *, GpBitmap **);
GpStatus WINAPI GdipBitmapGetPixel(GpBitmap *, INT, INT, DWORD *);
GpStatus WINAPI GdipDisposeImage(GpImage *);

static double now(void) { LARGE_INTEGER f, c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c); return c.QuadPart * 1000.0 / f.QuadPart; }

static GpBrush *make_brush(int kind, int wrap)
{
    static const PointF p[][2] = {{{10, 0}, {10, 90}}, {{0, 20}, {130, 20}}, {{5, 5}, {120, 80}}, {{40, 70}, {40, 7}},
        {{200, 3}, {17, 3}}, {{0, 0}, {0, 0.75f}}, {{3, 0}, {3.4f, 300}}};
    static const RectF r = {12, 9, 150, 70};
    static const float factors[] = {0, 0.3f, 0.9f, 1}, positions[] = {0, 0.2f, 0.7f, 1};
    static const DWORD colors[] = {0xff000000, 0x80ff0000, 0xff00ff00, 0x200000ff};
    GpBrush *b;
    int variant = kind % 4;
    kind /= 4;
    if (kind < 7)
        GdipCreateLineBrush(&p[kind][0], &p[kind][1], 0xff2b2b2b, kind & 1 ? 0x00000000 : 0xc06a9aff, wrap, &b);
    else
        GdipCreateLineBrushFromRectWithAngle(&r, 0xff102030, 0xfff0e0d0, kind == 7 ? 90.0f : kind == 8 ? 0.0f : 33.0f, kind == 9, wrap, &b);
    if (variant == 1) GdipSetLineGammaCorrection(b, TRUE);
    if (variant == 2) GdipSetLineBlend(b, factors, positions, 4);
    if (variant == 3) GdipSetLinePresetBlend(b, colors, positions, 4);
    if (kind == 2 && variant) GdipScaleLineTransform(b, 1.0f, 2.0f, 0);
    if (kind == 4 && variant == 3) GdipRotateLineTransform(b, 90.0f, 0);
    return b;
}

static void fill_set(GpGraphics *g, int kind, int wrap)
{
    static const float r[][4] = {{0, 0, 200, 100}, {210.5f, 3.25f, 77, 140}, {300, 0, 1, 160}, {310, 10, 120, 1}, {5, 110, 280, 45}};
    GpBrush *b = make_brush(kind, wrap);
    int i;
    for (i = 0; i < 5; i++) GdipFillRectangle(g, b, r[i][0], r[i][1], r[i][2], r[i][3]);
    GdipFillEllipse(g, b, 330, 30, 90, 120);
    GdipDeleteBrush(b);
}

int main(int argc, char **argv)
{
    ULONG_PTR tok; GdiplusStartupInput in = {1};
    int W = 440, H = 170, kind, wrap, t, x, y, reps = argc > 1 ? atoi(argv[1]) : 1, rep;
    BITMAPINFO bi = {{sizeof(BITMAPINFOHEADER), W, -H, 1, 32, BI_RGB}};
    HDC dc = CreateCompatibleDC(0);
    void *bits;
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    GpBitmap *target;
    DWORD crc = 0x12345678, c;
    double time = 0, t0;
    GdiplusStartup(&tok, &in, NULL);
    SelectObject(dc, bmp);
    GdipCreateBitmapFromScan0(W, H, 0, 0x26200A, NULL, &target);
    for (rep = 0; rep < reps; rep++)
    for (kind = 0; kind < 40; kind++)
        for (wrap = 0; wrap < 4; wrap++)
            for (t = 0; t < 4; t++)
            {
                GpGraphics *g;
                memset(bits, 0x30, W * H * 4);
                GdipCreateFromHDC(dc, &g);
                GdipSetSmoothingMode(g, t & 1 ? 4 : 3);
                if (t & 2)
                {
                    GdipTranslateWorldTransform(g, 400, 2, 0);
                    GdipRotateWorldTransform(g, 90, 0);
                    GdipScaleWorldTransform(g, 0.8f, 1.5f, 0);
                }
                t0 = now();
                fill_set(g, kind, wrap);
                time += now() - t0;
                GdipDeleteGraphics(g);
                if (!rep)
                {
                    DWORD *px = bits;
                    for (x = 0; x < W * H; x++) crc = (crc ^ px[x]) * 16777619u;
                }
                if (t != 1 || rep) continue;
                GdipGetImageGraphicsContext(target, &g);
                GdipGraphicsClear(g, 0xff204060);
                GdipSetSmoothingMode(g, 4);
                GdipSetPixelOffsetMode(g, 4);
                fill_set(g, kind, wrap);
                GdipDeleteGraphics(g);
                for (y = 0; y < H; y++)
                    for (x = 0; x < W; x++) { GdipBitmapGetPixel(target, x, y, &c); crc = (crc ^ c) * 16777619u; }
            }
    printf("gradfill %08lx %.0f ms\n", crc, time);
    return 0;
}
