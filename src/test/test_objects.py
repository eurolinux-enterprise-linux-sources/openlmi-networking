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
# This test checks if all associations defined in objects.json are present
#

import sys
import os
import pywbem
import subprocess
import json
import pprint

from test_base import TestBase

wbemconnection = None

def readObjects(fileName):
    f = open(fileName, "r")
    return json.load(f)

def run_and_read(command):
    p = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
    if p.wait() != 0:
        return None
    return p.communicate()[0]

def read(d, key):
    v = d.get(key, None)
    if v is not None:
        if v.startswith("%"):
            return wbemconnection.ExecQuery("WQL", v[1:])
        else:
            return v
    return None

def readList(d, key):
    l = read(d, key)
    if type(l) in [str, unicode]:
        return l.strip().split("\n")
    else:
        return l

def readInt(d, key):
    try:
        i = read(d, key)
    except pywbem.CIMError:
        return 0
    if i is None:
        return None
    if type(i) == list:
        return len(i)
    else:
        return int(i)

def dict2str(d, indent=4, level=0):
    s = ""
    i = (" " * indent) * level
    t = type(d)
    if t == str or t == unicode:
        s += '"%s"' % d
    elif t == list:
        i2 = i + (" " * indent)
        s += "[\n"
        for item in d:
            s += i2 + dict2str(item, level=level+1) + ",\n"
        s += i + "]"
    elif t == dict:
        i2 = i + (" " * indent)
        s += "{\n"
        for key, value in d.items():
            s += i2 + dict2str(key, level=level+1) + ": " + dict2str(value, level=level+1) + ",\n"
        s += i + "}"
    else:
        s += str(d)
    return s

class TestObjects(TestBase):
    def setUp(self):
        TestBase.setUp(self)
        global wbemconnection
        wbemconnection = self.wbemconnection
        fileName = "objects.json"

        exeDir = os.path.dirname(sys.argv[0])
        for path in [os.curdir, os.path.join(os.curdir, "test"), exeDir, os.path.join(exeDir, "test")]:
            objectsPath = os.path.join(path, fileName)
            if os.path.exists(objectsPath):
                break
        else:
            self.fail("Unable to find file %s" % fileName)
        try:
            self.assoc = readObjects(objectsPath)
        except ValueError, e:
            self.fail("Unable to parse %s: %s" % (objectsPath, str(e)))

    def test_classes(self):
        for cls in self.assoc["classes"]:
            try:
                instances = self.wbemconnection.ExecQuery("WQL", "select * from %s" % cls["name"])
            except pywbem.CIMError, e:
                self.fail("Error querying the class %s: %s" % (cls["name"], e[1]))

            unusedValues = {}
            values = cls.get("values", [])
            for item in values:
                for key in item.keys():
                    val = readList(item, key)
                    unusedValues[key] = val[:]

            for instance in instances:
                for key, unusedValuesItem in unusedValues.items():
                    self.assertTrue(key in instance, "Class %s don't have key %s, definition: \n%s" % (cls["name"], key, dict2str(cls)))
                    try:
                        value = instance[key]
                    except KeyError:
                        self.fail("Class %s don't have required key %s, definition: \n%s" % (cls["name"], key, dict2str(cls)))

                    try:
                        unusedValuesItem.remove(value)
                    except ValueError:
                        self.fail("Class %s has key %s with value %s, expected one of %s, definition: \n%s" % (cls["name"], key, value, str(unusedValuesItem), dict2str(cls)))

            classCount = readInt(cls, "count")
            if classCount is not None:
                self.assertEqual(len(instances), classCount, "Class %s has wrong number of instances: %d, expected %d, definition: \n%s" % (cls["name"], len(instances), classCount, dict2str(cls)))

    def test_associations(self):
        for assoc in self.assoc["associations"]:
            try:
                instances = self.wbemconnection.ExecQuery("WQL", "select * from %s" % assoc["name"])
            except pywbem.CIMError, e:
                self.fail("Unable to query for class %s: %s" % (assoc["name"], str(e)))

            instanceCount = 0
            for instance in instances:
                fromClass = instance[assoc["from_role"]].classname
                toClass = instance[assoc["to_role"]].classname
                filtr = assoc.get("filter", None)
                if filtr is not None:
                    if "from_class" in filtr and fromClass != filtr["from_class"]:
                        continue
                    if "to_class" in filtr and toClass != filtr["to_class"]:
                        continue
                self.assertEqual(fromClass, assoc["from"], "Class %s should be associated to %s instead of %s via role %s, definition: \n%s" % (assoc["name"], assoc["from"], fromClass, assoc["from_role"], dict2str(assoc)))
                self.assertEqual(toClass, assoc["to"], "Class %s should be associated to %s instead of %s via role %s, definition: \n%s" % (assoc["name"], assoc["to"], toClass, assoc["to_role"], dict2str(assoc)))
                instanceCount += 1

            # Check the count
            assocCount = readInt(assoc, "count")
            if assocCount is not None:
                self.assertEqual(instanceCount, assocCount, "Class %s has wrong number of instances: %d, expected %d, definition: \n%s" % (assoc["name"], instanceCount, assocCount, dict2str(assoc)))

