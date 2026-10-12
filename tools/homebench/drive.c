#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static double now_ms(void)
{
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return c.QuadPart * 1000.0 / f.QuadPart;
}

int main(int argc, char **argv)
{
    int n = atoi(argv[2]), interval = atoi(argv[3]);
    int x0 = atoi(argv[4]), y0 = atoi(argv[5]), x1 = atoi(argv[6]), y1 = atoi(argv[7]);
    int wheel = !strcmp(argv[1], "wheel") || !strcmp(argv[1], "pwheel"), post = argv[1][0] == 'p';
    double total = 0, worst = 0, start = now_ms();
    int i;
    timeBeginPeriod(1);
    for (i = 0; i < n; i++)
    {
        double f = (i % 40) / 40.0, t0;
        POINT pt, cp;
        HWND h;
        DWORD_PTR res;
        if (f > 0.5) f = 1 - f;
        f *= 2;
        pt.x = x0 + (int)((x1 - x0) * f);
        pt.y = y0 + (int)((y1 - y0) * f);
        h = WindowFromPoint(pt);
        t0 = now_ms();
        if (wheel)
        {
            int delta = ((i / 20) % 2) ? 120 : -120;
            if (post) PostMessageA(h, WM_MOUSEWHEEL, MAKEWPARAM(0, delta), MAKELPARAM(pt.x, pt.y));
            else SendMessageTimeoutA(h, WM_MOUSEWHEEL, MAKEWPARAM(0, delta), MAKELPARAM(pt.x, pt.y), SMTO_NORMAL, 5000, &res);
        }
        else
        {
            cp = pt;
            ScreenToClient(h, &cp);
            if (post) PostMessageA(h, WM_MOUSEMOVE, 0, MAKELPARAM(cp.x, cp.y));
            else SendMessageTimeoutA(h, WM_MOUSEMOVE, 0, MAKELPARAM(cp.x, cp.y), SMTO_NORMAL, 5000, &res);
        }
        t0 = now_ms() - t0;
        total += t0;
        if (t0 > worst) worst = t0;
        if (i == 0) printf("target %p\n", h);
        Sleep(interval);
    }
    printf("%s n=%d wall=%.0fms handler_avg=%.2fms handler_max=%.1fms\n", argv[1], n, now_ms() - start, total / n, worst);
    return 0;
}
