#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const WCHAR *fonts[] = {L"Tahoma", L"Courier New", L"Times New Roman", L"DejaVu Sans", L"Noto Sans", L"Liberation Serif"};
static const WCHAR *words[] = {L"Home", L"Recent files", L"Lightroom photos", L"Learn", L"Discover Photoshop",
        L"New file", L"Open", L"Your files", L"Shared with you", L"Deleted", L"Generative Fill", L"0123456789 %&@#"};
static const WICPixelFormatGUID *formats[] = {&GUID_WICPixelFormat32bppPBGRA, &GUID_WICPixelFormat32bppBGR,
        &GUID_WICPixelFormat32bppPRGBA};

static IWICImagingFactory *wic;
static IDWriteFactory *dw;
static ID2D1Factory *factory;
static unsigned int seed;
static FILE *out;

static unsigned int rnd(unsigned int n)
{
    seed = seed * 1103515245 + 12345;
    return ((seed >> 8) & 0xffffff) % n;
}

static float rndf(void)
{
    return rnd(10001) / 10000.0f;
}

static double now_us(void)
{
    LARGE_INTEGER c, f;

    QueryPerformanceCounter(&c);
    QueryPerformanceFrequency(&f);
    return c.QuadPart * 1e6 / f.QuadPart;
}

static void draw_words(ID2D1RenderTarget *rt, UINT w, UINT h, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        D2D1_COLOR_F c = {rndf(), rndf(), rndf(), rnd(3) ? 1.0f : rndf()};
        D2D1_BRUSH_PROPERTIES bp = {rnd(2) ? 1.0f : rndf(), {{{1, 0, 0, 1, 0, 0}}}};
        const WCHAR *word = words[rnd(ARRAY_SIZE(words))];
        ID2D1SolidColorBrush *brush;
        IDWriteTextFormat *fmt;
        D2D1_RECT_F r;

        if (FAILED(IDWriteFactory_CreateTextFormat(dw, fonts[rnd(ARRAY_SIZE(fonts))], NULL,
                rnd(3) ? DWRITE_FONT_WEIGHT_NORMAL : DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, 7.0f + rnd(200) / 10.0f, L"en-us", &fmt)))
            continue;
        r.left = rnd(w) - 10.0f + (rnd(2) ? rndf() : 0.0f);
        r.top = rnd(h) - 12.0f + (rnd(2) ? rndf() : 0.0f);
        r.right = r.left + 400.0f;
        r.bottom = r.top + 80.0f;
        ID2D1RenderTarget_CreateSolidColorBrush(rt, &c, &bp, &brush);
        ID2D1RenderTarget_DrawText(rt, word, lstrlenW(word), fmt, &r, (ID2D1Brush *)brush,
                D2D1_DRAW_TEXT_OPTIONS_NONE, rnd(2) ? DWRITE_MEASURING_MODE_NATURAL : DWRITE_MEASURING_MODE_GDI_CLASSIC);
        ID2D1SolidColorBrush_Release(brush);
        IDWriteTextFormat_Release(fmt);
    }
}

static void fill_rect(ID2D1RenderTarget *rt, UINT w, UINT h)
{
    D2D1_COLOR_F c = {rndf(), rndf(), rndf(), rndf()};
    ID2D1SolidColorBrush *brush;
    D2D1_RECT_F r;

    r.left = rnd(w);
    r.top = rnd(h);
    r.right = r.left + rnd(w) + 1;
    r.bottom = r.top + rnd(h) + 1;
    ID2D1RenderTarget_CreateSolidColorBrush(rt, &c, NULL, &brush);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);
    ID2D1SolidColorBrush_Release(brush);
}

static void copy_and_draw(ID2D1RenderTarget *rt, UINT w, UINT h, DXGI_FORMAT format)
{
    D2D1_BITMAP_PROPERTIES bp;
    D2D1_SIZE_U size = {w, h};
    D2D1_RECT_F dst = {w / 2.0f, 0.0f, w, h / 2.0f};
    ID2D1Bitmap *bitmap;

    ID2D1RenderTarget_GetDpi(rt, &bp.dpiX, &bp.dpiY);
    bp.pixelFormat.format = format;
    bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    if (FAILED(ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bp, &bitmap)))
    {
        printf("CreateBitmap failed\n");
        return;
    }
    ID2D1Bitmap_CopyFromRenderTarget(bitmap, NULL, rt, NULL);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst, 0.5f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
    ID2D1Bitmap_Release(bitmap);
}

static void dump(IWICBitmap *bmp, UINT w, UINT h)
{
    WICRect wr = {0, 0, w, h};
    IWICBitmapLock *lock;
    UINT size, stride, y;
    BYTE *data;

    IWICBitmap_Lock(bmp, &wr, WICBitmapLockRead, &lock);
    IWICBitmapLock_GetStride(lock, &stride);
    IWICBitmapLock_GetDataPointer(lock, &size, &data);
    for (y = 0; y < h; ++y)
        fwrite(data + y * stride, 4, w, out);
    IWICBitmapLock_Release(lock);
}

static void run_case(unsigned int n)
{
    static const D2D1_TEXT_ANTIALIAS_MODE aa[] = {D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE,
            D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, D2D1_TEXT_ANTIALIAS_MODE_ALIASED, D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE,
            D2D1_TEXT_ANTIALIAS_MODE_DEFAULT};
    static const D2D1_MATRIX_3X2_F identity = {{{1, 0, 0, 1, 0, 0}}};
    D2D1_RENDER_TARGET_PROPERTIES p = {0};
    UINT w = 20 + rnd(330), h = 6 + rnd(60);
    unsigned int frame, frames = 1 + rnd(3);
    ID2D1RenderTarget *rt;
    IWICBitmap *bmp;

    if (rnd(8) == 0)
        p.dpiX = p.dpiY = 144.0f;
    IWICImagingFactory_CreateBitmap(wic, w, h, formats[n % ARRAY_SIZE(formats)], WICBitmapCacheOnLoad, &bmp);
    if (FAILED(ID2D1Factory_CreateWicBitmapRenderTarget(factory, bmp, &p, &rt)))
    {
        printf("case %u: no render target\n", n);
        IWICBitmap_Release(bmp);
        return;
    }
    ID2D1RenderTarget_SetTextAntialiasMode(rt, aa[rnd(ARRAY_SIZE(aa))]);
    for (frame = 0; frame < frames; ++frame)
    {
        unsigned int kind = rnd(10);

        ID2D1RenderTarget_BeginDraw(rt);
        if (frame == 0 || rnd(2))
        {
            D2D1_COLOR_F c = {rndf(), rndf(), rndf(), rnd(2) ? 0.0f : rndf()};

            if (rnd(3))
                c.r = c.g = c.b = c.a = 0.0f;
            ID2D1RenderTarget_Clear(rt, &c);
        }
        if (kind == 0)
        {
            D2D1_MATRIX_3X2_F m = {{{1.0f + rndf(), rndf() * 0.3f, -rndf() * 0.3f, 1.0f, rndf() * 5.0f, rndf() * 5.0f}}};

            ID2D1RenderTarget_SetTransform(rt, &m);
        }
        if (kind == 1)
        {
            D2D1_RECT_F clip = {rnd(w) + rndf(), rnd(h) + rndf(), 0, 0};

            clip.right = clip.left + rnd(w) + rndf();
            clip.bottom = clip.top + rnd(h) + rndf();
            ID2D1RenderTarget_PushAxisAlignedClip(rt, &clip, D2D1_ANTIALIAS_MODE_ALIASED);
            if (rnd(2))
            {
                D2D1_COLOR_F c = {rndf(), 0.0f, rndf(), 1.0f};

                ID2D1RenderTarget_Clear(rt, &c);
            }
        }
        draw_words(rt, w, h, 1 + rnd(6));
        if (kind == 2 || kind == 3)
        {
            fill_rect(rt, w, h);
            draw_words(rt, w, h, 1 + rnd(4));
        }
        if (kind == 4)
        {
            copy_and_draw(rt, w, h, n % ARRAY_SIZE(formats) == 2 ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM);
            draw_words(rt, w, h, 1 + rnd(3));
        }
        if (kind == 1)
            ID2D1RenderTarget_PopAxisAlignedClip(rt);
        if (kind == 5)
        {
            ID2D1RenderTarget_Flush(rt, NULL, NULL);
            draw_words(rt, w, h, 1 + rnd(3));
        }
        ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
        ID2D1RenderTarget_SetTransform(rt, &identity);
        fwrite(&w, 4, 1, out);
        fwrite(&h, 4, 1, out);
        dump(bmp, w, h);
    }
    ID2D1RenderTarget_Release(rt);
    IWICBitmap_Release(bmp);
}

static void timing(void)
{
    D2D1_RENDER_TARGET_PROPERTIES p = {0};
    D2D1_COLOR_F clear = {0}, col = {0.85f, 0.85f, 0.85f, 1.0f};
    ID2D1SolidColorBrush *brush;
    IDWriteTextFormat *fmt;
    D2D1_RECT_F r = {2, 2, 300, 30};
    ID2D1RenderTarget *rt;
    IWICBitmap *bmp;
    double t = 0.0;
    int i;

    IWICImagingFactory_CreateBitmap(wic, 300, 30, &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &bmp);
    ID2D1Factory_CreateWicBitmapRenderTarget(factory, bmp, &p, &rt);
    ID2D1RenderTarget_SetTextAntialiasMode(rt, D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    ID2D1RenderTarget_CreateSolidColorBrush(rt, &col, NULL, &brush);
    IDWriteFactory_CreateTextFormat(dw, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"en-us", &fmt);
    for (i = -50; i < 1000; ++i)
    {
        if (!i)
            t = now_us();
        ID2D1RenderTarget_BeginDraw(rt);
        ID2D1RenderTarget_Clear(rt, &clear);
        ID2D1RenderTarget_DrawText(rt, L"Recent files", 12, fmt, &r, (ID2D1Brush *)brush,
                D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
        ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    }
    printf("clear + text + EndDraw: %.1f us per frame\n", (now_us() - t) / 1000);
    IDWriteTextFormat_Release(fmt);
    ID2D1SolidColorBrush_Release(brush);
    ID2D1RenderTarget_Release(rt);
    IWICBitmap_Release(bmp);
}

static int compare(const char *a, const char *b)
{
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    unsigned long long total = 0, one = 0, more = 0;
    int ca, cb, d, max = 0;

    if (!fa || !fb)
    {
        printf("cannot open dumps\n");
        return 2;
    }
    for (;;)
    {
        ca = fgetc(fa);
        cb = fgetc(fb);
        if (ca == EOF || cb == EOF)
            break;
        ++total;
        d = abs(ca - cb);
        if (d > max)
            max = d;
        if (d == 1)
            ++one;
        else if (d > 1)
            ++more;
    }
    if (ca != cb)
    {
        printf("dumps differ in size\n");
        return 1;
    }
    printf("%llu bytes, %llu off by one (%.4f%%), %llu off by more, max diff %d\n",
            total, one, total ? one * 100.0 / total : 0.0, more, max);
    return more || one * 1000 >= total;
}

int main(int argc, char **argv)
{
    unsigned int i, cases = 400;

    if (argc == 4 && !strcmp(argv[1], "cmp"))
        return compare(argv[2], argv[3]);
    if (argc < 2)
    {
        printf("usage: wictext <dump> [cases] | wictext cmp <dump> <dump>\n");
        return 2;
    }
    if (argc > 2)
        cases = atoi(argv[2]);
    out = fopen(argv[1], "wb");
    CoInitialize(NULL);
    CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void **)&wic);
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown **)&dw);
    seed = 1234;
    for (i = 0; i < cases; ++i)
        run_case(i);
    fclose(out);
    timing();
    return 0;
}
