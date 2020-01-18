#!/usr/bin/python
# -*- Coding:utf-8 -*-
#
# Copyright (C) 2012 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Radek Novacek <rnovacek@redhat.com>
#
# Fake NetworkManager-like API for testing networking provider
#

import sys
import dbus
import dbus.service
import gobject

from dbus.mainloop.glib import DBusGMainLoop

def debug(s):
    sys.stderr.write("-fake- %s\r\n" % s.replace("\n", "\r\n-fake- "))

class Device(dbus.service.Object):
    index = 1
    def __init__(self, bus_name, name=None, type=None):
        self.index = Device.index
        Device.index += 1
        self.object_path = "/org/freedesktop/NetworkManager/Devices/%d" % self.index
        dbus.service.Object.__init__(self, bus_name, self.object_path)
        if name is None:
            self.name = 'eth%d' % self.index
        else:
            self.name = name
        self.type = type if type is not None else 1
        self.properties = {
                'State': dbus.UInt32(100),
                'Interface': self.name,
                'IpInterface': self.name,
                'DeviceType': dbus.UInt32(self.type),
                'Ip4Config': dbus.ObjectPath("/"),
                'Ip6Config': dbus.ObjectPath("/"),
                'Dhcp4Config': dbus.ObjectPath("/"),
                'Dhcp6Config': dbus.ObjectPath("/")
        }
        self.subProperties = dbus.Dictionary({
                'HwAddress': "AA:BB:CC:DD:EE:%02X" % self.index
            }, signature="sv")
        if self.type == 10:
            # Bonding
            self.subProperties["Slaves"] = dbus.Array(signature="o")

    @dbus.service.signal(dbus_interface="org.freedesktop.NetworkManager.Device.Bond",
                         signature="a{sv}")
    def PropertiesChanged(self, props):
        debug("Device.PropertiesChanged %s" % props)

    @dbus.service.method(dbus_interface="org.freedesktop.DBus.Properties",
                         in_signature="s",
                         out_signature="a{sv}")
    def GetAll(self, interface):
        debug("Device.GetAll")
        if interface == "org.freedesktop.NetworkManager.Device":
            return self.properties
        else:
            return self.subProperties

    @dbus.service.method(dbus_interface="org.freedesktop.DBus.Properties",
                         in_signature="ss",
                         out_signature="v")
    def Get(self, interface, prop):
        if interface == "org.freedesktop.NetworkManager.Device":
            return self.properties[prop]
        else:
            return self.subProperties[prop]


class ActiveConnection(dbus.service.Object):
    def __init__(self, bus_name, index, device, connection):
        self.object_path = "/org/freedesktop/NetworkManager/ActiveConnection/%d" % index
        dbus.service.Object.__init__(self, bus_name, self.object_path)
        self.index = index
        self.properties = {
            'Devices': [device.object_path],
            'Connection': connection.object_path,
            'State': 2
        }

    @dbus.service.method(dbus_interface="org.freedesktop.DBus.Properties",
                         in_signature="s",
                         out_signature="a{sv}")
    def GetAll(self, interface):
        debug("GetAll")
        return self.properties

    @dbus.service.method(dbus_interface="org.freedesktop.DBus.Properties",
                         in_signature="ss",
                         out_signature="v")
    def Get(self, interface, prop):
        debug("Get")
        if interface == 'org.freedesktop.NetworkManager.Connection.Active':
            if prop == 'State':
                return dbus.UInt32(self.properties[prop])



class Manager(dbus.service.Object):
    def __init__(self, bus_name):
        self.bus_name = bus_name
        self.object_path = "/org/freedesktop/NetworkManager"
        dbus.service.Object.__init__(self, bus_name, self.object_path)

        self.devices = {}
        self.activeConnections = {}
        self.connections = {}

        self.activeConnectionsIndex = 0
        self.connectionsIndex = 0

        self.addEthDevice()
        self.addEthDevice()

    def addEthDevice(self):
        d = Device(self.bus_name)
        self.addDevice(d)
        return d

    def addDevice(self, d):
        self.devices[d.object_path] = d
        self.DeviceAdded(d.object_path) # emit signal

    def connectionByUuid(self, uuid):
        for con in self.connections.values():
            if con.hash.get("connection", []).get("uuid", "") == uuid:
                return con
        return None

    def deviceByName(self, name):
        for dev in self.devices.values():
            if dev.name == name:
                return dev
        return None

    def deviceByMAC(self, mac):
        for dev in self.devices.values():
            if dev.subProperties.get("HwAddress") == mac:
                return dev
        print "No device with mac address:", mac
        return None


    @dbus.service.method(dbus_interface="org.freedesktop.NetworkManager",
                         in_signature="ooo",
                         out_signature="o")
    def ActivateConnection(self, connection, device, specific_object):
        debug("ActivateConnection")
        ac = ActiveConnection(self.bus_name, self.activeConnectionsIndex, self.devices[device], self.connections[connection])
        self.activeConnections[ac.object_path] = ac
        self.activeConnectionsIndex += 1
        return ac.object_path

    @dbus.service.method(dbus_interface="org.freedesktop.NetworkManager",
                         in_signature="",
                         out_signature="o")
    def DeactivateConnection(self):
        debug("DeactivateConnection")
        return ""

    @dbus.service.method(dbus_interface="org.freedesktop.NetworkManager",
                         in_signature="a{sa{sv}}oo",
                         out_signature="oo")
    def AddAndActivateConnection(self, connection, device, specific_object):
        debug("AddAndActivateConnection")
        return ("", "")

    @dbus.service.method(dbus_interface="org.freedesktop.NetworkManager",
                         in_signature="",
                         out_signature="ao")
    def GetDevices(self):
        debug("GetDevices")
        return [device.object_path for device in self.devices.values()]

    @dbus.service.method(dbus_interface="org.freedesktop.DBus.Properties",
                         in_signature="s",
                         out_signature="a{sv}")
    def GetAll(self, interface):
        debug("GetAll")
        return { "ActiveConnections": dbus.Array([], signature="o") }

    @dbus.service.signal(dbus_interface="org.freedesktop.NetworkManager",
                         signature="o")
    def DeviceRemoved(self, device):
        debug("DeviceRemoved %s" % device)

    @dbus.service.signal(dbus_interface="org.freedesktop.NetworkManager",
                         signature="o")
    def DeviceAdded(self, device):
        debug("DeviceAdded %s" % device)

    @dbus.service.signal(dbus_interface="org.freedesktop.NetworkManager",
                         signature="a{sv}")
    def PropertiesChanged(self, props):
        debug("PropertiesChanged %s" % props)

    @dbus.service.signal(dbus_interface="org.freedesktop.NetworkManager",
                         signature="u")
    def StateChanged(self, state):
        debug("StateChanged %s" % state)

class Settings(dbus.service.Object):
    def __init__(self, bus_name, manager):
        self.bus_name = bus_name
        self.manager = manager
        dbus.service.Object.__init__(self, bus_name, "/org/freedesktop/NetworkManager/Settings")
        self.settings = self.manager.connections
        self.settingsIndex = 0

    @dbus.service.method(dbus_interface="org.freedesktop.NetworkManager.Settings",
                         in_signature="a{sa{sv}}",
                         out_signature="o")
    def AddConnection(self, connection):
        c = Connection(self.manager, self.bus_name, self.settingsIndex, connection, self)
        self.settingsIndex += 1
        self.settings[c.object_path] = c
        con = connection.get("connection", [])
        if con.get("type", "") == "bond":
            debug("Bonding")
            # it's bonding, we need to add bond device
            d = Device(self.bus_name, connection["bond"]["interface-name"], 10)
            self.manager.addDevice(d)
        if "master" in con.keys():
            master_setting = self.manager.connectionByUuid(con["master"])
            master_name = master_setting.hash.get("bond", []).get("interface-name", "")
            device = self.manager.deviceByName(master_name)
            mac = ":".join(["%.2X" % x for x in connection.get("802-3-ethernet").get("mac-address")])
            device.subProperties["Slaves"].append(dbus.ObjectPath(self.manager.deviceByMAC(mac).object_path))
            device.PropertiesChanged(device.subProperties)
        self.NewConnection(c.object_path)
        return c.object_path

    @dbus.service.method(dbus_interface="org.freedesktop.NetworkManager.Settings",
                         in_signature="",
                         out_signature="ao")
    def ListConnections(self):
        return self.settings.keys()

    @dbus.service.signal(dbus_interface="org.freedesktop.NetworkManager.Settings",
                         signature="o")
    def NewConnection(self, objectpath):
        debug("NewConnection %s" % objectpath)


class Connection(dbus.service.Object):
    def __init__(self, manager, bus_name, index, hash, settings):
        self.manager = manager
        self.object_path = "/org/freedesktop/NetworkManager/Settings/%d" % index
        self.hash = hash
        self.settings = settings
        dbus.service.Object.__init__(self, bus_name, self.object_path)

    @dbus.service.method(dbus_interface="org.freedesktop.NetworkManager.Settings.Connection",
                         in_signature="",
                         out_signature="a{sa{sv}}")
    def GetSettings(self):
        return self.hash

    @dbus.service.method(dbus_interface="org.freedesktop.NetworkManager.Settings.Connection",
                         in_signature="",
                         out_signature="")
    def Delete(self):
        if self.hash["connection"]["type"] == "bond":
            iface = self.hash["bond"]["interface-name"]
            dev = self.manager.deviceByName(iface)
            self.manager.DeviceRemoved(dev.object_path)
            self.manager.devices.pop(dev.object_path)
        self.settings.settings.pop(self.object_path)
        self.Removed()

    @dbus.service.signal(dbus_interface="org.freedesktop.NetworkManager.Settings.Connection",
                         signature="")
    def Removed(self):
        debug("Removed %s" % self.object_path)

class Controller(dbus.service.Object):
    def __init__(self, bus_name, manager):
        self.bus_name = bus_name
        self.manager = manager
        dbus.service.Object.__init__(self, bus_name, "/org/fedoraproject/OpenLMINetworking/Controller")

    @dbus.service.method(dbus_interface="org.fedoraproject.OpenLMINetworking.Controller",
                         in_signature="s",
                         out_signature="")
    def AddDevice(self, name):
        self.manager.addEthDevice()

class FakeNM(object):
    loop = None
    doStart = True
    def __init__(self):
        mainloop = DBusGMainLoop()

        self.bus = dbus.SessionBus(mainloop=mainloop)

        bus_name = dbus.service.BusName('org.freedesktop.FakeNetworkManager', self.bus)
        manager = Manager(bus_name)
        setting = Settings(bus_name, manager)
        controller = Controller(bus_name, manager)

        self.loop = gobject.MainLoop()
        self.loop.run()

if __name__ == "__main__":
    gobject.threads_init()
    f = FakeNM()
