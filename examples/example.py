#!/usr/bin/python3
#
# export GI_TYPELIB_PATH=/usr/local/lib/x86_64-linux-gnu/girepository-1.0/

import gi
import time
gi.require_version('Lfb', '0.0')
from gi.repository import Lfb

Lfb.init('org.sigxcpu.lfbexample')
event = Lfb.Event.new('phone-incoming-call')
event.trigger_feedback()
# feedbackd terminates feedback from clients that disconnect from DBus to avoid
# feedback not being stopped. So wait a bit before quitting:
time.sleep(3)
