.. _feedback-themes(5):

===============
feedback-themes
===============

---------------------------------
Theme configuration for feedbackd
---------------------------------

DESCRIPTION
-----------

In order to reflect device specifics and user overrides the feedback
(such as sound, haptic or led) that is being triggered by an event can
be configured and overridden by so called themes. Feedback themes use a JSON
format that can be validated with ``fbd-theme-validate``.

For details on how to create or modify feedback themes see
``feedbackd's documentation`` at https://source.puri.sm/Librem5/feedbackd#feedback-theme

Feedback types
--------------

To build a theme you can use several different feedback types:

- `Sound`:  Plays a sound from the installed sound theme
- `VibraRumble`: A single rumble using the haptic motor
- `VibraPeriodic`: A periodic rumble using the haptic motor
- `Led`: A LED blinking in a periodic pattern

Sound feedback
~~~~~~~~~~~~~~

Sound feedbacks specify an event name from a XDG sound theme. Sound themes
are described in the ``Sound theme spec`` at https://freedesktop.org/wiki/Specifications/sound-theme-spec/

VibraRumble feedback
~~~~~~~~~~~~~~~~~~~~

The `VibraRumble` feedback uses a single property

- `duration`: The duration of the rumble in ms.

LED feedback
~~~~~~~~~~~~

The `Led` feedback type uses two properties to specify the way a LED blinks.

- `color`: This specifies the color a LED should blink with. It supports the fixed color names `red`,
  `green`, `blue` and `white` as well as values in the RGB HEX  format (`#RRGGBB`) where
  `RR`, `GG` and `BB` are two digit  hex value between `00` and `FF` specifying the value of
  each component. E.g. `#00FFFF` corresponds to cyan color.
- `frequency`: The LEDs blinkinig frequencey in mHz.

See also
========

``feedbackd(8)`` ``fbcli(1)`` ``fbd-theme-validate(1)``
