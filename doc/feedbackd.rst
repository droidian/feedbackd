.. _feedbackd(8):

=========
feedbackd
=========

--------------------------------------
A daemon to provide feedback on events
--------------------------------------

SYNOPSIS
--------
|   **feedbackd** [OPTIONS...]


DESCRIPTION
===========

``Feedbackd`` is a daemon that runs in the users session to trigger
event feedback such as playing a sound, trigger a haptic motor or blink
a LED.

The feedback triggered by a given event is determined by the feedback theme in
use. Events are submitted via a DBus API.

Any feedback triggered by a client via an event will be stopped latest when the
client disconnects from DBus. This makes sure all feedbacks get canceled if the
app that triggered it crashes.

For details refer to the event and feedback theme specs at
`<https://source.puri.sm/Librem5/feedbackd/>`__

Options
=======

``-h``, ``--help``
   print help and exit

See also
========

``fbcli(1)`` ``fbd-theme-validate(1)`` ``feedback-themes(5)``
