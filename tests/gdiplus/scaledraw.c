#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int GpStatus;
typedef void GpGraphics, GpImage, GpBitmap, GpImageAttributes;
typedef struct { UINT32 GdiplusVersion; void *cb; BOOL a, b; } GdiplusStartupInput;
typedef struct { float m[5][5]; } ColorMatrix;
typedef struct { float X, Y; } PointF;
GpStatus WINAPI GdiplusStartup(ULONG_PTR *, const GdiplusStartupInput *, void *);
GpStatus WINAPI GdipCreateFromHDC(HDC, GpGraphics **);
GpStatus WINAPI GdipGetImageGraphicsContext(GpImage *, GpGraphics **);
GpStatus WINAPI GdipDeleteGraphics(GpGraphics *);
GpStatus WINAPI GdipSetInterpolationMode(GpGraphics *, int);
GpStatus WINAPI GdipSetPixelOffsetMode(GpGraphics *, int);
GpStatus WINAPI GdipGraphicsClear(GpGraphics *, DWORD);
GpStatus WINAPI GdipCreateBitmapFromScan0(INT, INT, INT, int, BYTE *, GpBitmap **);
GpStatus WINAPI GdipBitmapGetPixel(GpBitmap *, INT, INT, DWORD *);
GpStatus WINAPI GdipDrawImageRectRect(GpGraphics *, GpImage *, float, float, float, float, float, float, float, float, int, GpImageAttributes *, void *, void *);
GpStatus WINAPI GdipDrawImagePointsRect(GpGraphics *, GpImage *, const PointF *, INT, float, float, float, float, int, GpImageAttributes *, void *, void *);
GpStatus WINAPI GdipDisposeImage(GpImage *);
GpStatus WINAPI GdipCreateImageAttributes(GpImageAttributes **);
GpStatus WINAPI GdipDisposeImageAttributes(GpImageAttributes *);
GpStatus WINAPI GdipSetImageAttributesWrapMode(GpImageAttributes *, int, DWORD, BOOL);
GpStatus WINAPI GdipSetImageAttributesGamma(GpImageAttributes *, int, BOOL, float);
GpStatus WINAPI GdipSetImageAttributesColorMatrix(GpImageAttributes *, int, BOOL, const ColorMatrix *, const ColorMatrix *, int);
GpStatus WINAPI GdipSetImageAttributesColorKeys(GpImageAttributes *, int, BOOL, DWORD, DWORD);

static double now(void) { LARGE_INTEGER f, c; QueryPerformanceFrequency(&f); QueryPerformanceCounter(&c); return c.QuadPart * 1000.0 / f.QuadPart; }

static GpBitmap *pattern_image(int w, int h, int format, int bpp)
{
    int stride = (w * bpp / 8 + 3) & ~3, x, y;
    BYTE *data = malloc(stride * h);
    GpBitmap *img;
    for (y = 0; y < h; y++)
        for (x = 0; x < stride; x++)
            data[y * stride + x] = (x * 7 + y * 13 + ((x * y) >> 5)) & 0xff;
    if (bpp == 32)
        for (y = 0; y < h; y++)
            for (x = 0; x < w; x++)
            {
                BYTE a = (x + y) & 64 ? 0xff : (x * 3 + y) & 0xff, *p = data + y * stride + x * 4;
                p[3] = a;
                if (format == 0xE200B) { p[0] = p[0] * a / 255; p[1] = p[1] * a / 255; p[2] = p[2] * a / 255; }
            }
    GdipCreateBitmapFromScan0(w, h, stride, format, data, &img);
    return img;
}

static DWORD crc_bits(DWORD crc, const DWORD *px, int n)
{
    int i;
    for (i = 0; i < n; i++) crc = (crc ^ px[i]) * 16777619u;
    return crc;
}

static DWORD crc_bitmap(DWORD crc, GpBitmap *bmp, int w, int h)
{
    int x, y;
    DWORD c;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) { GdipBitmapGetPixel(bmp, x, y, &c); crc = (crc ^ c) * 16777619u; }
    return crc;
}

static void draw_set(GpGraphics *g, GpImage *img, int w, int h, GpImageAttributes **attrs)
{
    static const float dst[][4] = {{3, 4, 41, 41}, {50.3f, 7.7f, 41, 41}, {100, 2, 120, 120}, {230, 10, 48, 48}, {290, 3, 13, 77},
        {10, 140, 256, 160}, {280, 140, 41, 26}, {340.5f, 150.5f, -41, 41}, {360, 250, 41, -41}};
    PointF pts[3] = {{120, 200}, {170, 215}, {105, 250}};
    int i, a;
    for (a = 0; a < 5; a++)
    {
        for (i = 0; i < 9; i++)
            GdipDrawImageRectRect(g, img, dst[i][0], dst[i][1] + a * 2, dst[i][2], dst[i][3], 0, 0, w, h, 2, attrs[a], NULL, NULL);
        GdipDrawImageRectRect(g, img, 380, 5 + a * 40, 30, 35, w / 10.0f, h / 7.0f, w * 0.7f, h * 0.8f, 2, attrs[a], NULL, NULL);
        GdipDrawImageRectRect(g, img, 330, 270, 60, 25, -w / 4.0f, -h / 3.0f, w * 1.5f, h * 1.6f, 2, attrs[a], NULL, NULL);
        GdipDrawImagePointsRect(g, img, pts, 3, 0, 0, w, h, 2, attrs[a], NULL, NULL);
    }
}

int main(void)
{
    static const struct { int w, h, format, bpp; } src[] = {{1200, 1200, 0x21808, 24}, {1200, 1200, 0x26200A, 32}, {600, 600, 0xE200B, 32},
        {600, 600, 0x21808, 24}, {512, 320, 0x21808, 24}, {97, 1000, 0x22009, 32}, {240, 240, 0x26200A, 32}};
    static const int modes[] = {0, 1, 2, 3, 4, 5, 6, 7};
    ULONG_PTR tok; GdiplusStartupInput in = {1};
    int W = 420, H = 310, s, m, o;
    BITMAPINFO bi = {{sizeof(BITMAPINFOHEADER), W, -H, 1, 32, BI_RGB}};
    HDC dc = CreateCompatibleDC(0);
    void *bits;
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    GpImageAttributes *attrs[5];
    ColorMatrix cm = {{{0.9f, 0.1f, 0, 0, 0}, {0, 0.8f, 0.1f, 0, 0}, {0.1f, 0, 0.7f, 0, 0}, {0, 0, 0, 0.9f, 0}, {0.05f, 0, 0.02f, 0, 1}}};
    DWORD total = 0x12345678;
    double t = 0, t0;
    GdiplusStartup(&tok, &in, NULL);
    SelectObject(dc, bmp);
    attrs[0] = NULL;
    GdipCreateImageAttributes(&attrs[1]);
    GdipSetImageAttributesWrapMode(attrs[1], 4, 0x80ff0000, FALSE);
    GdipCreateImageAttributes(&attrs[2]);
    GdipSetImageAttributesWrapMode(attrs[2], 3, 0, FALSE);
    GdipSetImageAttributesGamma(attrs[2], 0, TRUE, 1.7f);
    GdipCreateImageAttributes(&attrs[3]);
    GdipSetImageAttributesColorMatrix(attrs[3], 0, TRUE, &cm, NULL, 0);
    GdipCreateImageAttributes(&attrs[4]);
    GdipSetImageAttributesWrapMode(attrs[4], 1, 0, FALSE);
    GdipSetImageAttributesColorKeys(attrs[4], 0, TRUE, 0x00303030, 0x00a0a0a0);
    for (s = 0; s < ARRAYSIZE(src); s++)
    {
        GpBitmap *img = pattern_image(src[s].w, src[s].h, src[s].format, src[s].bpp);
        GpBitmap *target;
        DWORD crc = 0x12345678;
        for (m = 0; m < ARRAYSIZE(modes); m++)
            for (o = 0; o < 2; o++)
            {
                GpGraphics *g;
                memset(bits, 0x40, W * H * 4);
                GdipCreateFromHDC(dc, &g);
                GdipSetInterpolationMode(g, modes[m]);
                GdipSetPixelOffsetMode(g, o ? 4 : 0);
                t0 = now();
                draw_set(g, img, src[s].w, src[s].h, attrs);
                t += now() - t0;
                GdipDeleteGraphics(g);
                crc = crc_bits(crc, bits, W * H);
            }
        GdipCreateBitmapFromScan0(W, H, 0, 0x26200A, NULL, &target);
        for (m = 0; m < ARRAYSIZE(modes); m += 3)
        {
            GpGraphics *g;
            GdipGetImageGraphicsContext(target, &g);
            GdipGraphicsClear(g, 0xff204060);
            GdipSetInterpolationMode(g, modes[m]);
            t0 = now();
            draw_set(g, img, src[s].w, src[s].h, attrs);
            t += now() - t0;
            GdipDeleteGraphics(g);
            crc = crc_bitmap(crc, target, W, H);
        }
        GdipDisposeImage(target);
        GdipDisposeImage(img);
        printf("%dx%d %#x: %08lx\n", src[s].w, src[s].h, src[s].format, crc);
        total = (total ^ crc) * 16777619u;
    }
    printf("scaledraw %08lx %.0f ms\n", total, t);
    return 0;
}
