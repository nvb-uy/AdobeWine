#!/usr/bin/env python3
# Checks the outputs of run.sh: check.py <outdir> <files dir> <files dir as a Windows path>
import json
import os
import sys

OUT, F, W = sys.argv[1], os.path.realpath(sys.argv[2]), sys.argv[3]
failures = 0


def case(name):
    def read(ext):
        try:
            with open(os.path.join(OUT, name + ext)) as f:
                return f.read()
        except OSError:
            return ""
    reqs = [json.loads(line) for line in read(".json").splitlines() if line.strip()]
    return read(".out"), reqs, read(".status").strip()


def check(name, what, ok, detail=""):
    global failures
    if ok:
        print("PASS %s: %s" % (name, what))
    else:
        failures += 1
        print("FAIL %s: %s%s" % (name, what, (" (" + detail + ")") if detail else ""))


def result(out):
    lines = [line for line in out.splitlines() if line.startswith("RESULT")]
    return lines[-1] if lines else "no result"


def finished(name, out, status):
    check(name, "finished", status not in ("124", ""), "timed out" if status == "124" else "did not run")


def opt(req, key, default=None):
    return req["options"].get(key, default)


def choices(req):
    return {c[0]: c for c in opt(req, "choices", [])}


def win(path):
    return W + "\\" + path.replace("/", "\\")


def expect_result(name, out, want):
    check(name, "returns " + want, result(out).lower() == ("RESULT ok " + want).lower(), result(out))


out, reqs, st = case("legacy")
finished("legacy", out, st)
expect_result("legacy", out, win("host/frame_0001.png"))
check("legacy", "file title", "file title frame_0001.png" in out, out)
check("legacy", "one OpenFile request", [r["method"] for r in reqs] == ["OpenFile"], str(reqs))
if reqs:
    check("legacy", "case-insensitive filters", opt(reqs[0], "filters", [None])[0] ==
          ["Projects (*.aep)", [[0, "*.[aA][eE][pP]"]]], str(opt(reqs[0], "filters")))
    check("legacy", "all files as *", opt(reqs[0], "filters", [None, None])[1] == ["All files", [[0, "*"]]],
          str(opt(reqs[0], "filters")))
    check("legacy", "current filter", (opt(reqs[0], "current_filter") or [""])[0] == "Projects (*.aep)",
          str(opt(reqs[0], "current_filter")))
    check("legacy", "title", reqs[0]["title"] == "Open (portal smoke test)", reqs[0]["title"])
    check("legacy", "missing initial folder not sent", "nonexistent" not in str(opt(reqs[0], "current_folder")),
          str(opt(reqs[0], "current_folder")))

out, reqs, st = case("save")
finished("save", out, st)
expect_result("save", out, win("out/new project.aep"))
if reqs:
    check("save", "SaveFile with the suggested name", reqs[0]["method"] == "SaveFile" and
          opt(reqs[0], "current_name") == "untitled.aep", str(reqs[0]))

out, reqs, st = case("savedef")
finished("savedef", out, st)
expect_result("savedef", out, win("out/picture.png"))
check("savedef", "filter index of the chosen type", "filter index 2" in out, out)

out, reqs, st = case("multi")
finished("multi", out, st)
want = "RESULT ok %s| frame_0001.png| frame_0002.png|" % win("host")
check("multi", "files of the first folder", result(out).lower() == want.lower(), result(out))
if reqs:
    check("multi", "multiple", opt(reqs[0], "multiple") is True, str(reqs[0]))

out, reqs, st = case("hook")
finished("hook", out, st)
expect_result("hook", out, win("host/frame_0002.png"))
check("hook", "hook sees WM_INITDIALOG with the app's structure", "custdata 0xc0ffee" in out, out)
ev = [x for x in out.splitlines() if x.startswith("CDN_FILEOK: events ")]
ev = ev[0].split()[2].split(",") if ev else []
check("hook", "notifications", ev[:3] == ["INITDONE", "FOLDERCHANGE", "SELCHANGE"] and "TYPECHANGE" in ev and
      ev[-1] == "FILEOK", ",".join(ev))
check("hook", "template controls set from the portal and dialog hidden",
      "CDN_FILEOK: settings 2, slices 1, metadata 1, radio b 1, dialog visible 0" in out, out)
check("hook", "file path and filter index in CDN_FILEOK",
      ("file path %s, filter index 2" % win("host/frame_0002.png")).lower() in out.lower(), out)
check("hook", "one request", len(reqs) == 1, str(len(reqs)))
if reqs:
    ch = {c[1]: c for c in opt(reqs[0], "choices", [])}
    check("hook", "template choices", sorted(ch) == sorted(["Settings:", "Slices:", "Save metadata", "Option 4"]) and
          ch["Settings:"][2:] == [[["0", "PNG-24"], ["1", "JPEG High"], ["2", "GIF 128 Dithered"]], "0"] and
          ch["Save metadata"][2:] == [[], "false"], json.dumps(opt(reqs[0], "choices")))
    check("hook", "radio group as one list",
          ch.get("Option 4", [0, 0, []])[2:] == [[["0", "Radio A"], ["1", "Radio B"]], "0"],
          json.dumps(opt(reqs[0], "choices")))

out, reqs, st = case("reject")
finished("reject", out, st)
expect_result("reject", out, win("host/frame_0002.png"))
check("reject", "portal shown again after CDN_FILEOK refused", len(reqs) == 2 and "refused" in out, str(len(reqs)))
if len(reqs) == 2:
    check("reject", "second request starts in the chosen folder",
          opt(reqs[1], "current_folder") == F + "/host", str(opt(reqs[1], "current_folder")))

out, reqs, st = case("hooka")
finished("hooka", out, st)
expect_result("hooka", out, win("out/existing.png"))
check("hooka", "ANSI hook gets OPENFILENAMEA", "filter PNG-24 (*.png)" in out, out)
check("hooka", "ANSI CDM_GETFILEPATH", ("file path %s" % win("out/existing.png")).lower() in out.lower(), out)

out, reqs, st = case("item")
finished("item", out, st)
expect_result("item", out, win("host/frame_0001.png"))
check("item", "choices applied before OnFileOk", "OnFileOk: import-as item 3 (hr 0), sequence 1, no-label 0" in out, out)
if reqs:
    ch = choices(reqs[0])
    ok = (ch.get("101") == ["101", "Import As:", [["1", "Footage"], ["2", "Composition"],
                                                  ["3", "Composition - Retain Layer Sizes"]], "2"]
          and ch.get("102") == ["102", "ImporterJPEG Sequence", [], "false"]
          and ch.get("103", [0, ""])[1] != "" and ch["103"][3] == "true"
          and ch.get("105") == ["105", "Format:", [["1", "PNG"], ["2", "TIFF"]], "1"]
          and "104" not in ch and "205" not in ch)
    check("item", "choices sent", ok, json.dumps(opt(reqs[0], "choices")))

out, reqs, st = case("docportal")
if os.path.exists(os.path.join(OUT, "xattr")):
    finished("docportal", out, st)
    expect_result("docportal", out, win("host/frame_0001.png"))
else:
    print("SKIP docportal: no user xattrs here")

out, reqs, st = case("itemsave")
finished("itemsave", out, st)
expect_result("itemsave", out, win("out/picture.jpg"))
check("itemsave", "OnTypeChange before and after", "OnTypeChange: 1" in out and "OnTypeChange: 2" in out, out)
check("itemsave", "OnOverwrite for the completed name", ("OnOverwrite: %s" % win("out/picture.jpg")).lower() in out.lower(), out)
check("itemsave", "OnFileOk sees name, type and controls",
      ("OnFileOk: %s, name picture.jpg, type 2" % win("out/picture.jpg")).lower() in out.lower()
      and "OnFileOk: copy 1, disabled 0, profile 0" in out, out)
check("itemsave", "GetFileName is the name only", "GetFileName: picture.jpg, GetFileTypeIndex: 2" in out, out)
if reqs:
    ch = choices(reqs[0])
    check("itemsave", "enabled controls without mnemonics", sorted(c[1] for c in ch.values()) ==
          ["As a Copy", "ICC Profile:  sRGB IEC61966-2.1"], json.dumps(opt(reqs[0], "choices")))
    check("itemsave", "current folder", opt(reqs[0], "current_folder") == F + "/out", str(opt(reqs[0], "current_folder")))
    check("itemsave", "suggested name", opt(reqs[0], "current_name") == "Untitled-1", str(opt(reqs[0], "current_name")))
    check("itemsave", "accept label", opt(reqs[0], "accept_label") == "Save", str(opt(reqs[0], "accept_label")))
    check("itemsave", "uppercase spec matched case-insensitively",
          opt(reqs[0], "filters", [[0, [[0, ""]]]])[0][1][0] == [0, "*.[pP][sS][dD]"], str(opt(reqs[0], "filters")))

out, reqs, st = case("itemreopen")
finished("itemreopen", out, st)
expect_result("itemreopen", out, win("host/frame_0002.png"))
check("itemreopen", "portal shown again after OnFileOk refused", len(reqs) == 2 and "OnFileOk calls 2" in out,
      "%d requests" % len(reqs))
if len(reqs) == 2:
    check("itemreopen", "second request starts in the chosen folder",
          opt(reqs[1], "current_folder") == F + "/host", str(opt(reqs[1], "current_folder")))

out, reqs, st = case("folder")
finished("folder", out, st)
expect_result("folder", out, win("other"))
if reqs:
    check("folder", "directory request without filters", opt(reqs[0], "directory") is True and
          not opt(reqs[0], "filters"), str(reqs[0]))

out, reqs, st = case("browse")
finished("browse", out, st)
expect_result("browse", out, win("other"))
check("browse", "display name", "display name other" in out, out)
check("browse", "callback initialized", "BFFM_INITIALIZED" in out, out)
if reqs:
    r = reqs[0]
    check("browse", "folder request from the callback", opt(r, "directory") is True and
          opt(r, "current_folder") == F + "/host" and opt(r, "accept_label") == "Choose" and
          r["title"] == "Choose a folder (portal smoke test)", str(r))
else:
    check("browse", "portal reached", False)

out, reqs, st = case("modal")
finished("modal", out, st)
for kind in ("item", "legacy"):
    line = [x for x in out.splitlines() if x.startswith("modal %s:" % kind)]
    ok = False
    if line:
        v = line[0].replace(",", "").split()
        ticks, disabled, after = int(v[3]), int(v[6]), int(v[9])
        ok = ticks >= 5 and disabled >= ticks - 1 and after == 1
    check("modal", "%s: messages pumped, owner disabled meanwhile and enabled after" % kind, ok,
          line[0] if line else out)
check("modal", "folder sent canonical", bool(reqs) and opt(reqs[0], "current_folder") == F + "/host",
      str(opt(reqs[0], "current_folder")) if reqs else "")
check("modal", "parent window passed", len(reqs) == 2 and all(r["parent_window"] == "x11:1234" for r in reqs),
      str([r["parent_window"] for r in reqs]))

for name, n in (("fallback", 0), ("itemfallback", 0), ("vanish", 1), ("off", 0), ("hookauto", 0)):
    out, reqs, st = case(name)
    finished(name, out, st)
    check(name, "wine dialog shown and cancelled", "wine dialog visible" in out and result(out) == "RESULT cancel",
          result(out))
    check(name, "%d portal requests" % n, len(reqs) == n, str(len(reqs)))

out, reqs, st = case("browsefallback")
finished("browsefallback", out, st)
title = [x for x in out.splitlines() if x.startswith("wine dialog visible: ")]
check("browsefallback", "classic dialog without a portal", bool(title) and "portal smoke test" not in title[0] and
      result(out) == "RESULT cancel", title[0] if title else result(out))

out, reqs, st = case("itemauto")
finished("itemauto", out, st)
expect_result("itemauto", out, win("host/frame_0001.png"))

out, reqs, st = case("appdefault")
finished("appdefault", out, st)
expect_result("appdefault", out, win("host/frame_0001.png"))
check("appdefault", "per-app always over global never", len(reqs) == 1, str(len(reqs)))

sys.exit(failures)
