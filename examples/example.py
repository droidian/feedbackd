#!/usr/bin/python3
#
# export GI_TYPELIB_PATH=/usr/local/lib/x86_64-linux-gnu/girepository-1.0/

import gi
gi.require_version('Lfb', '0.0')
from gi.repository import Lfb

Lfb.init('org.sigxcpu.lfbexample')
event = Lfb.Event.new('phone-incoming-call')
event.trigger_feedback()
