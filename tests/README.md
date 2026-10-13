# Test programs

Small Windows programs used to find and check the PhotoshopWine patches. Each one prints what
an API returns; run it on Windows and under PhotoshopWine and compare. Build with MinGW, e.g.

```sh
x86_64-w64-mingw32-gcc -O1 -o indirect.exe ddbstride/indirect.c -lgdi32 -luser32
photoshopwine run indirect.exe
```

`webview2/wv2msg.cpp` needs `WebView2.h` from Microsoft's WebView2 SDK (NuGet package
`Microsoft.Web.WebView2`, `build/native/include`), which is not included here.
