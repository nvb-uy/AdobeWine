# Installing AdobeWine

This guide is for any Linux distribution.

## Quick install

One command does everything below: it checks what your system is missing (and tells you
what to install), gets PhotoshopWine, downloads the prebuilt runtime for the release (or
builds it when there is none), creates the prefix, runs the Creative Cloud installer and
waits while you sign in and install Photoshop from Creative Cloud. Download the Creative
Cloud installer from <https://creativecloud.adobe.com/apps/download/creative-cloud> first,
then:

```sh
curl -fsSL https://raw.githubusercontent.com/nvb-uy/PhotoshopWine/master/install.sh | bash
```

It picks the newest `Creative_Cloud_Set-Up*.exe` in your Downloads folder; to use another
one, add its path: `... | bash -s -- /path/to/Creative_Cloud_Set-Up.exe`. If it stops (or
you press Ctrl+C while it waits for Photoshop), run it again and it continues where it
left off. It never uses sudo. PhotoshopWine itself goes to `~/.local/share/photoshopwine`
and the `photoshopwine` command to `~/.local/bin`.

Afterwards:

```sh
photoshopwine                    # open Photoshop (or: photoshopwine file.psd)
photoshopwine update             # update PhotoshopWine, its runtime and the prefix
photoshopwine setup --windows-fonts /path/to/Windows/Fonts   # optional, see below
photoshopwine --help             # everything else: cc, off, fix-signin, launchers, report
```

`photoshopwine` takes the same commands as `adobewine` (used in the rest of this guide),
and with no command it opens Photoshop. The manual steps below are for building from
source yourself.

You need:

- a 64-bit x86 PC with a **Vulkan-capable GPU and driver** (NVIDIA proprietary driver,
  or Mesa RADV / ANV for AMD / Intel), see [COMPATIBILITY.md](COMPATIBILITY.md);
- an **Adobe Creative Cloud subscription** (AdobeWine contains no Adobe software);
- about **25 GB** of free disk space (Wine build ~8 GB, each Adobe app 3-10 GB);
- for the best results, **fonts from your own Windows installation** (a Windows
  partition, or a copy of `C:\Windows\Fonts`). Adobe's interface uses Segoe UI, and
  Photoshop's menu bar crashes without it. Fonts are not included because they are
  Microsoft's and cannot be redistributed. Without them, setup makes Segoe UI from
  Microsoft's open fallback font Selawik if your distribution has it installed.

Nothing is installed system-wide. The Wine build, the downloads and the Windows prefix all
live in `~/.local/share/adobewine` (change it with the `ADOBEWINE_HOME` variable).

## 1. Install the build tools

The build needs the usual Wine build dependencies, a MinGW cross-compiler, git and
winetricks. The commands below are the standard ones for each family. **Only the Arch
route has been tested so far**; please report how these go.

**Arch Linux and Arch-based (CachyOS, EndeavourOS, Manjaro, Garuda):**
```sh
sudo pacman -S --needed base-devel git ccache mingw-w64-gcc winetricks curl zstd ffmpeg \
     libcups libgphoto2 libxcomposite libxinerama libxxf86vm mesa opencl-headers \
     opencl-icd-loader pcsclite perl samba sane sdl2 unixodbc v4l-utils vulkan-headers \
     gst-plugins-good gst-plugins-bad gst-libav
```
Multilib is not needed: the runtime is built in Wine's new WoW64 mode.

**Debian 13 / Ubuntu 24.04 or newer** (enable "source code" repositories first):
```sh
sudo apt build-dep wine
sudo apt install git ccache mingw-w64 winetricks curl zstd xz-utils \
     gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-libav
```

**Fedora 41 or newer:**
```sh
sudo dnf builddep wine
sudo dnf install git ccache mingw64-gcc mingw32-gcc winetricks curl zstd \
     gstreamer1-plugins-good gstreamer1-plugins-bad-free
```

**openSUSE Tumbleweed:**
```sh
sudo zypper source-install --build-deps-only wine
sudo zypper install git ccache mingw64-cross-gcc mingw32-cross-gcc winetricks curl zstd \
     gstreamer-plugins-good gstreamer-plugins-bad
```

**Gentoo:**
```sh
sudo emerge --ask --onlydeps app-emulation/wine-vanilla
sudo emerge --ask dev-vcs/git app-emulation/winetricks app-arch/zstd sys-devel/crossdev
sudo crossdev --target x86_64-w64-mingw32 && sudo crossdev --target i686-w64-mingw32
```
Wine's USE flags `vulkan` and `wow64` should be on.

### Runtime dependencies only

If you use a prebuilt runtime (see "Prebuilt runtime" below) you only need what AdobeWine
needs to *run*:

```sh
# Debian 13: winetricks is in the "contrib" section; enable it first:
sudo sed -i 's/^Components: main$/Components: main contrib/' /etc/apt/sources.list.d/debian.sources
sudo apt update
# Debian / Ubuntu (Ubuntu has winetricks in "universe", enabled by default)
sudo apt install winetricks curl zstd libvulkan1 libgnutls30 libpulse0 libasound2t64 \
     gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
     gstreamer1.0-libav libfreetype6 libfontconfig1 libunwind8 libxkbcommon0 p7zip-full python3-pil
# Fedora
sudo dnf install winetricks curl zstd vulkan-loader gnutls pulseaudio-libs alsa-lib \
     gstreamer1-plugins-base gstreamer1-plugins-good gstreamer1-plugins-bad-free \
     freetype fontconfig libunwind libxkbcommon p7zip python3-pillow
# openSUSE Tumbleweed
sudo zypper install winetricks curl zstd libvulkan1 libgnutls30 libpulse0 libasound2 \
     gstreamer-plugins-base gstreamer-plugins-good gstreamer-plugins-bad \
     libfreetype6 fontconfig libunwind8 libxkbcommon0 7zip python313-Pillow
# Arch
sudo pacman -S --needed desktop-file-utils fontconfig freetype2 gettext glib2 libpcap \
     libunwind libx11 libxcursor libxext libxkbcommon libxi libxrandr wayland \
     vulkan-icd-loader gnutls alsa-lib libpulse gst-plugins-base-libs winetricks curl zstd \
     gst-plugins-good gst-plugins-bad gst-libav p7zip python-pillow
```

These lines are checked automatically in clean containers (`tests/distro/check.sh`), see
[COMPATIBILITY.md](COMPATIBILITY.md#tested-in-containers).

### Prebuilt runtime (experimental outside Arch)

Each release has `adobewine-<version>-x86_64.tar.zst`, built on Arch Linux. It contains
`/opt/adobewine` (the Wine runtime plus the scripts). On other distributions with a
recent glibc it may work:

```sh
sudo tar -C / -xf adobewine-0.2.2-x86_64.tar.zst   # from the latest release
sudo ln -sf /opt/adobewine/bin/adobewine /usr/local/bin/adobewine
```

This has been tested on Debian 13, Ubuntu 24.04, Fedora 42 and openSUSE Tumbleweed: Wine
starts, creates a prefix and runs 64-bit and 32-bit programs. One known limit: the build
links Wine's FFmpeg media decoder (`winedmo`) against Arch's current FFmpeg (libavcodec 63),
which those distributions do not ship yet, so video import uses Wine's GStreamer path instead. For video
work outside Arch, building from source (below) is the better choice.

If `adobewine setup` fails with missing-library or glibc errors, build from source instead.

## 2. Get AdobeWine and build it

```sh
git clone https://github.com/le-birnes/AdobeWine.git
cd AdobeWine
scripts/build-wine.sh          # 20-60 minutes the first time
```

The script downloads Wine 11.18, applies the patches in `patches/` and installs the
result into `~/.local/share/adobewine/runtime`. Later runs are much faster when
`ccache` is installed.

Put the `adobewine` command on your PATH:

```sh
mkdir -p ~/.local/bin && ln -sf "$PWD/bin/adobewine" ~/.local/bin/adobewine
```

## 3. Create the prefix

```sh
adobewine setup --windows-fonts /mnt/windows/Windows/Fonts
```

This downloads DXVK, vkd3d-proton and (on NVIDIA) nvidia-libs, checks their checksums,
installs wine-gecko, wine-mono, Microsoft's shader compiler and the core fonts through
winetricks, and applies the settings the Adobe apps need. Leave out `--windows-fonts` if
you have no Windows fonts at hand; setup then makes Segoe UI from Selawik when that font is
installed, and you can run the command again later with your Windows fonts.

## 4. Install Creative Cloud and sign in

Download the Creative Cloud installer from Adobe
(<https://www.adobe.com/download/creative-cloud>), then:

```sh
adobewine run ~/Downloads/Creative_Cloud_Set-Up.exe
```

If the installer window stays white, use Adobe's
Creative Cloud offline installer instead (see [TROUBLESHOOTING.md](TROUBLESHOOTING.md#installation-and-sign-in)).

Sign in with your Adobe account when the installer asks. If the sign-in window stays
blank or white, close it and run:

```sh
adobewine fix-signin
adobewine cc
```

`fix-signin` makes Adobe's sign-in library use its Internet Explorer-based window instead
of the Chromium-based one, which does not draw under Wine. It does not change anything
about your account or licence.

## 5. Install and start the apps

In the Creative Cloud window, open **Apps** and install the apps you want (Photoshop,
Illustrator, Premiere Pro, Audition, Media Encoder). Then:

```sh
adobewine launchers        # adds them to your desktop's application menu
adobewine photoshop        # or illustrator, premiere, audition, encoder, cc
```

To close everything cleanly (the apps may ask to save): `adobewine off`.

## Updating AdobeWine

```sh
photoshopwine update
```

This updates the checkout (when it has no local changes), gets the new runtime when the
release has one (or rebuilds yours if you built it) and runs setup again. By hand:

```sh
cd AdobeWine && git pull && scripts/build-wine.sh
```

Your prefix and installed apps stay as they are. Run `adobewine setup` again if the release
notes say so.

## Uninstalling

```sh
adobewine off
adobewine launchers --remove
rm -rf ~/.local/share/adobewine     # removes the prefix, including the installed Adobe apps
rm ~/.local/bin/adobewine ~/.local/bin/photoshopwine
rm -rf ~/.local/share/photoshopwine # if you used the one-line installer
```
