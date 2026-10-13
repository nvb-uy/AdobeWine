# PhotoshopWine

An unofficial project to run Adobe Photoshop on Linux, installed and signed in through Adobe's own Creative Cloud, with as much compatibility as possible.

**Disclaimer:** PhotoshopWine makes Photoshop work on Linux. It does not include or download any Adobe software, and it does not crack, bypass or modify Adobe licensing: you need your own Creative Cloud subscription. No support will be given to anyone trying to crack or pirate it.

PhotoshopWine is a fork of [AdobeWine](https://github.com/le-birnes/AdobeWine) that builds on its patches and focuses only on Photoshop. You can still install other Adobe apps through Creative Cloud, but only Photoshop is officially supported here. For other Adobe software we recommend AdobeWine.

## The goal

With PhotoshopWine you get a near-native experience with the latest Photoshop versions.

- GPU acceleration, including local ML models (100%, currently only tested with NVIDIA)
- All tools working properly (100%)
- A design consistent with your desktop environment instead of Wine's Windows-style dialogs (80%, most of the UI works, tested with KDE)
- Performance close to Windows or better¹ (80%, about 200% over AdobeWine on NVIDIA 580 drivers, almost native performance)
- Visual improvements: antialiasing and text issues fixed² (100%, native-like look achieved)

¹ Some of these optimizations were submitted to AdobeWine, but others are Photoshop-specific and run better on PhotoshopWine.
² The antialiasing and text fixes were submitted to AdobeWine.

## Install

PhotoshopWine is built from source for now. See [docs/INSTALL.md](docs/INSTALL.md). Once it is installed:

```sh
adobewine setup --windows-fonts /path/to/Windows/Fonts  # create the prefix (once)
adobewine run ~/Downloads/Creative_Cloud_Set-Up.exe       # install Creative Cloud, sign in
adobewine cc                                              # install Photoshop from Creative Cloud
adobewine launchers                                       # add Photoshop to your app menu
```

When something breaks, see [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

## Tested on

Kubuntu 26.04, KDE Plasma 6.6 on Wayland, NVIDIA RTX 4090. Other distributions, desktops and GPUs are untested so far. Reports, good or bad, are welcome: see [CONTRIBUTING.md](CONTRIBUTING.md).

## Not affiliated with Adobe

PhotoshopWine is not affiliated with, endorsed by or supported by Adobe. "Adobe", "Photoshop" and "Creative Cloud" are trademarks of Adobe Inc., used here only to say which program this project is meant to run.

## Credits

Built on [AdobeWine](https://github.com/le-birnes/AdobeWine) and upstream [Wine](https://www.winehq.org). The installer patches come from [PhialsBasement](https://github.com/PhialsBasement), the first desktop file dialog work from NickPittas' AE4Linux and Wine MR 10060, and several fixes from switch87.

## License

The patches and scripts are licensed under the GNU LGPL 2.1 or later, the same as Wine (see [LICENSE](LICENSE)). DXVK, vkd3d-proton, nvidia-libs, wine-gecko and wine-mono are downloaded from their own projects and keep their own licenses.
