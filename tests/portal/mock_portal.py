#!/usr/bin/env python3
# Minimal org.freedesktop.portal.FileChooser for testing comdlg32's portal path without a desktop.
# Needs dbus-python + PyGObject (Ubuntu: python3-dbus python3-gi; run with the system python).
#
#   mock_portal.py <log.json> <spec.json>
#
# Every OpenFile/SaveFile request is appended to <log.json> (one JSON object per line: method,
# parent_window, title, options) and answered from the next entry of the JSON list in <spec.json>;
# the last entry is reused once the list runs out. An entry may have:
#   uris      list of URIs returned (default none)
#   code      response code (default 0); "exit" quits without answering, "never" never answers
#   filter    current_filter returned: a filter index (0-based) or a filter name
#   choices   "keep" (initial values), "toggle" (check buttons flipped, lists get their last item,
#             the default) or "last" (lists get their last item, check buttons kept)
#   delay_ms  delay before the Response signal (default 0: sent before the method returns)
# FileChooser's version property is 4.
import json
import os
import sys

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

BUS = "org.freedesktop.portal.Desktop"
PATH = "/org/freedesktop/portal/desktop"
IFACE = "org.freedesktop.portal.FileChooser"


def plain(v):
    if isinstance(v, (dbus.Array, list)):
        if getattr(v, "signature", None) == "y":
            return bytes(v).rstrip(b"\0").decode("utf-8", "replace")
        return [plain(x) for x in v]
    if isinstance(v, (dbus.Struct, tuple)):
        return [plain(x) for x in v]
    if isinstance(v, (dbus.Dictionary, dict)):
        return {str(k): plain(x) for k, x in v.items()}
    if isinstance(v, dbus.Boolean):
        return bool(v)
    if isinstance(v, (dbus.String, dbus.ObjectPath)):
        return str(v)
    if isinstance(v, (dbus.UInt32, dbus.Int32, dbus.UInt64, dbus.Int64, dbus.Byte)):
        return int(v)
    return v


class Request(dbus.service.Object):
    @dbus.service.signal("org.freedesktop.portal.Request", signature="ua{sv}")
    def Response(self, code, results):
        pass


class FileChooser(dbus.service.Object):
    def __init__(self, conn, log, spec):
        super().__init__(conn, PATH)
        self.conn, self.log, self.spec, self.n = conn, log, spec, 0

    def answer(self, method, sender, parent, title, options):
        entry = {"method": method, "parent_window": str(parent), "title": str(title), "options": plain(options)}
        with open(self.log, "a") as f:
            f.write(json.dumps(entry) + "\n")
        spec = self.spec[min(self.n, len(self.spec) - 1)] if self.spec else {}
        token = str(options.get("handle_token", "t%d" % self.n))
        self.n += 1
        path = "/org/freedesktop/portal/desktop/request/%s/%s" % (sender[1:].replace(".", "_"), token)
        req = Request(self.conn, path)
        code = spec.get("code", 0)
        if code == "exit":
            GLib.timeout_add(100, lambda: os._exit(0))
            return dbus.ObjectPath(path)
        if code == "never":
            return dbus.ObjectPath(path)

        mode = spec.get("choices", "toggle")
        choices = []
        for cid, _label, items, initial in options.get("choices", []):
            if mode == "keep":
                choices.append((cid, initial))
            elif len(items) == 0:
                choices.append((cid, initial if mode == "last" else ("false" if initial == "true" else "true")))
            else:
                choices.append((cid, items[-1][0]))
        results = {"uris": dbus.Array(spec.get("uris", []), signature="s")}
        if choices:
            results["choices"] = dbus.Array([dbus.Struct(c, signature="ss") for c in choices], signature="(ss)")
        want = spec.get("filter")
        filters = options.get("filters", [])
        for i, (name, patterns) in enumerate(filters):
            if want == i or want == name:
                results["current_filter"] = dbus.Struct(
                    (name, dbus.Array([dbus.Struct(p, signature="us") for p in patterns], signature="(us)")),
                    signature="sa(us)")

        def emit():
            req.Response(dbus.UInt32(code), dbus.Dictionary(results, signature="sv"))
            return False
        delay = spec.get("delay_ms", 0)
        if delay:
            GLib.timeout_add(delay, emit)
        else:
            emit()
        return dbus.ObjectPath(path)

    @dbus.service.method(IFACE, in_signature="ssa{sv}", out_signature="o", sender_keyword="sender")
    def OpenFile(self, parent, title, options, sender=None):
        return self.answer("OpenFile", sender, parent, title, options)

    @dbus.service.method(IFACE, in_signature="ssa{sv}", out_signature="o", sender_keyword="sender")
    def SaveFile(self, parent, title, options, sender=None):
        return self.answer("SaveFile", sender, parent, title, options)

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature="ss", out_signature="v")
    def Get(self, interface, prop):
        if interface == IFACE and prop == "version":
            return dbus.UInt32(4)
        raise dbus.exceptions.DBusException("no such property", name="org.freedesktop.DBus.Error.UnknownProperty")

    @dbus.service.method(dbus.PROPERTIES_IFACE, in_signature="s", out_signature="a{sv}")
    def GetAll(self, interface):
        return {"version": dbus.UInt32(4)} if interface == IFACE else {}


def main():
    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    with open(sys.argv[2]) as f:
        spec = json.load(f)
    name = dbus.service.BusName(BUS, bus, do_not_queue=True)  # noqa: F841 (keeps the name)
    FileChooser(bus, sys.argv[1], spec)
    print("mock portal ready", flush=True)
    GLib.MainLoop().run()


if __name__ == "__main__":
    main()
