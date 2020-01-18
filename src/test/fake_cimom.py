#!/usr/bin/python
# -*- Coding:utf-8 -*-

import sys
import os
import signal
import tempfile
import shutil
import subprocess
import pexpect
import threading

class IOThread(threading.Thread):
    def __init__(self, command, args=None, events=None, **kw):
        threading.Thread.__init__(self)
        self.kw = kw
        self.command = command
        if args is None:
            self.args = []
        else:
            self.args = args
        self.events = {}
        if events is not None:
            for ev in events:
                self.events[ev] = threading.Event()

        self.is_timeout = False
        self.end_event = threading.Event()

    def event(self, event):
        return self.events[event]

    def run(self, events=None):
        self.p = pexpect.spawn(self.command, self.args, **self.kw)
        while True:
            try:
                index = self.p.expect(self.events.keys() + ["\n"])
            except pexpect.EOF, e:
                self.end_event.set()
                break
            except pexpect.TIMEOUT, e:
                self.is_timeout = True
                self.end_event.set()
                break

            s = self.p.before
            if isinstance(self.p.after, basestring):
                s += self.p.after
            sys.stderr.write(s)
            if index < len(self.events):
                event = self.events[self.events.keys()[index]]
                event.before = s
                event.set()

    def kill(self):
        self.p.terminate(force=True)

class CIMOM(object):
    startString = "localConnectServer started"
    providerWaitString = "-#- Pausing for provider:.*"
    def __init__(self, mof_file, reg_file, provider_dir, debug=False):
        self.debug = debug
        self.temp_dir = None
        self._create_config(mof_file, reg_file, provider_dir)
        self._register()

        print "------ sfcbd"
        env = os.environ
        if self.debug:
            env["SFCB_PAUSE_PROVIDER"] = "LMI_IPConfigurationService"

        self.sfcb = IOThread("/usr/sbin/sfcbd", ["-c", self.config_file], env=env, events=[self.startString, self.providerWaitString])
        self.sfcb.start()
        self.sfcb.event(self.startString).wait()

    def __del__(self):
        if self.temp_dir is not None:
            if os.path.isdir(self.temp_dir):
                shutil.rmtree(self.temp_dir)

    def _create_config(self, mof_file, reg_file, provider_dir):
        self.mof_file = mof_file
        self.reg_file = reg_file
        self.provider_dir = provider_dir
        self.p = None

        self.temp_dir = tempfile.mkdtemp()
        self.config_file = os.path.join(self.temp_dir, "sfcb.conf")
        f = open(self.config_file, "w")
        f.write("""httpPort: 54320
providerDirs: /usr/lib64/sfcb/ %(provider_dir)s
registrationDir: %(temp_dir)s/registration
httpSocketPath: %(temp_dir)s/openlmi-sfcbHttpSocket""" % {
    'provider_dir': os.path.abspath(self.provider_dir),
    'temp_dir': self.temp_dir
})
        f.close()

    def _register(self):
        print "Starting sfcbd ..."
        staging_dir = os.path.join(self.temp_dir, "stage")
        os.mkdir(staging_dir)
        registration_dir = os.path.join(self.temp_dir, "registration")
        os.mkdir(registration_dir)

        shutil.copy("/var/lib/sfcb/stage/default.reg", staging_dir)
        print "------ sfcbstage"
        subprocess.call(["/usr/bin/sfcbstage", "-s", staging_dir, "-r", self.reg_file, "/usr/share/openlmi-providers/05_LMI_Qualifiers.mof", self.mof_file])

        print "------ sfcbrepos"
        subprocess.call(["/usr/bin/sfcbrepos", "-s", staging_dir, "-r", registration_dir, "-f"])

    def run_debugger(self):
        if self.debug:
            event = self.sfcb.event(self.providerWaitString)
            event.wait()
            line = event.before
            self.provider_pid = int(line[line.find("-pid: ") + 6:])
            print "Provider PID:", self.provider_pid

            self.gdb = pexpect.spawn("/usr/bin/gdb",
                ["-q", "-p", "%s" % self.provider_pid,
                 "-ex", "up 2", "-ex", "set debug_break=1",
                 "-ex", "set follow-fork-mode child"])
            threading.Thread(target=self.gdb.interact).start()

    def poll(self):
        return self.sfcb.isalive()

    def terminate(self):
        os.kill(self.sfcb.p.pid, signal.SIGTERM)
