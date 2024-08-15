.. _fbcli(1):

=====
fbcli
=====

------------------------
Emit events to feedbackd
------------------------

SYNOPSIS
--------
|   **fbcli** [OPTIONS...]


DESCRIPTION
-----------

``fbcli`` can be used to submit events to ``feedbackd`` to trigger
audio, visual or haptic feedback.

OPTIONS
=======

``-h``, ``--help``
   print help and exit

``-E=EVENT``; ``--event=EVENT``
  Submit the given event to ``feedbackd``. Valid events are listes in
  the event naming spec at
  https://source.puri.sm/Librem5/feedbackd/-/blob/main/doc/Event-naming-spec-0.0.0.md

``-t=TIMEOUT``; ``--timeout=TIMEOUT``
  The timeout in seconds after which feedback for the given event should
  be stopped.

``-P=PROFILE``; ``--profile=PROFILE``
  The feedback profile (``full``, ``quiet``, ``silent``)
  to use for the given event. If no event is specified then the global
  feedback profile is changed.

``-w=TIMEOUT``; ``--watch=TIMEOUT``
  Maximum timeout to wait for the feedback for the given event to end and
  ``fbcli`` to exit.

See also
========

``feedbackd(8)`` ``fbd-theme-validate(1)`` ``feedback-themes(5)``
