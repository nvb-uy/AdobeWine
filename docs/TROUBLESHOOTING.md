# Troubleshooting

First steps for any problem:

1. Every run writes a log to `~/.local/share/photoshopwine/logs/<app>-<date>.log`; the path is
   printed when you start an app from a terminal.
2. `photoshopwine report` builds a report (system, versions, log tail) with personal data
   removed, shows it to you, and can send it to the project. See [REPORTING.md](REPORTING.md).
3. More detail in the log: start the app with `WINEDEBUG=+err,+seh,+loaddll photoshopwine`.

## Installation and sign-in

| Symptom | What to do |
|---|---|
| `no PhotoshopWine runtime` | Run the installer again, or `scripts/build-wine.sh` |
| The Creative Cloud installer stays on "Loading..." | `photoshopwine setup` installs wine-gecko; run it again and check its output |
| The sign-in window is blank or white | Close it, `photoshopwine fix-signin`, then `photoshopwine cc` |
| The small Creative Cloud web installer (about 3 MB) shows only a white window | Fixed for build 2.14.0.82 (found by NickPittas, [AdobeWine #2](https://github.com/le-birnes/AdobeWine/issues/2)): patches 0055-0057 (since AdobeWine 0.2.0). If it still stays white, use Adobe's Creative Cloud **offline installer** (search Adobe Help for "Creative Cloud offline installer") and run it with `photoshopwine run`. Only use installers from adobe.com |
| Creative Cloud's Apps tab stays on its placeholder the first time | Click Home, then Apps again |
| An app says it is not licensed | Sign out and in again in Creative Cloud (avatar menu); the apps take the licence from it |

## Display

| Symptom | What to do |
|---|---|
| Black or empty windows, nothing renders | Check `vulkaninfo --summary` lists your GPU; on hybrid laptops try `PHOTOSHOPWINE_NVIDIA_OFFLOAD=1` |
| Photoshop menu bar missing or white, UI frozen after start (100 % CPU) | Segoe UI is missing: `photoshopwine setup --windows-fonts <your Windows Fonts folder>`, or install the Selawik fonts and run `photoshopwine setup` again |
| Text in some dialogs is blank | Same as above: add your Windows fonts |
| A panel draws only after resizing | Please report it with `photoshopwine report`; resizing the window is the workaround |
| Wrong scaling on HiDPI screens | Each launch takes your desktop's scale (Xft.dpi). To choose another: `photoshopwine setup --dpi 144` (96 = 100 %, 144 = 150 %, 192 = 200 %; `--dpi auto` goes back to the desktop's), then restart the apps |
| Photoshop: no graphics processor (Preferences > Performance shows none, GPU features greyed out), or Camera Raw: "requires GPU acceleration to edit photos" | Run `photoshopwine setup` again: it checks the Direct3D 12 feature level and sets `VKD3D_FEATURE_LEVEL` (12_0 or 12_1) in the prefix when the driver offers only 11_x |

## Media

| Symptom | What to do |
|---|---|
| MP4/MKV files do not import or play | Install GStreamer plugins: `gst-plugins-good`, `gst-plugins-bad`, `gst-libav` (names vary by distribution) |
| Premiere/Media Encoder: no CUDA renderer, no hardware export | NVIDIA only: check `/proc/driver/nvidia/version` exists and run `photoshopwine setup` again (it installs nvidia-libs) |
| No sound | Wine uses PulseAudio/PipeWire; check the output device in `photoshopwine winecfg` > Audio |

## Closing and crashes

| Symptom | What to do |
|---|---|
| "recovered from a crash" after a normal close | Close apps with their own Quit or `photoshopwine off`, not by killing Wine |
| Audition asks about a recovered session at every start | Choose **Delete** once |
| Everything hangs | `photoshopwine off --force` stops all Wine processes (unsaved work is lost) |

## Starting over

```sh
photoshopwine off --force
mv ~/.local/share/photoshopwine/prefix ~/.local/share/photoshopwine/prefix.old
photoshopwine setup --windows-fonts <your Windows Fonts folder>
```
Then install Creative Cloud again. Delete `prefix.old` once the new prefix works.
