This is similar in spirit (and heavily based on) to the [Sound naming spec][].

# Overview

This specification gives direction on how to name the Event types
(triggering feedbacks such as running the haptic motor or blinking an
LED) that are available for use by applications, when creating a
feedback theme. It does so by laying out a standard naming scheme for
Event creation, as well as providing a minimal list of must have
Events, and a larger list with many more examples to help with the
creation of extended Events for third party applications, with
different event types and usage.

# Context

The list of default contexts for the feedback theme are:

- Alerts: Events to alert the user of an action or event which may
  have a major impact on the system or their current use
- Notifications: Events to trigger feedback to notify the user that
  the system, or their current use case has changed state in some way,
  e.g. new email arriving
- Actions:	Event that notify the user on their actions.
- Input Event: This triggers feedbacks that give direct response to
  input events from the user, such as key presses on an on screen
  keyboard

# Event naming guides

Here we define some guidelines for when creating new Event names
that extend the standardized list of Event names defined here, in
order to provide Events for more specific events and usages.

Event names are in the en_US.US_ASCII locale. This means that the
characters allowed in the Event names must fall within the US-ASCII
character set. As a further restriction, all Event names may only
contain lowercase letters, numbers, underscore, dash, or period
characters. Spaces, colons, slashes, and backslashes are not
allowed. Also, sound names must be spelled as they are in the en_US
dictionary.

Events for branded applications should be named the same as the binary
executable for the application, prefixed by the string “x-”, to avoid
name space clashes with future standardized names. Example:
“x-openoffice-foobar”.

## Standard Event names

This section describes the standard Event names that should be used
by artists when creating themes, and by developers when writing
applications which will use the Feedback Theme Specification.

### Alerts

- battery-low: The Event used when the battery is low (below 20%, for example).
- power-unplug-battery-low: The power cable has been unplugged and the battery level is low.

### Notifications

- message-new-instant: The event used when a new IM is received.
- message-new-sms:  The event used when a new sms is received.
- message-new-email:  The event used when a new email is received.
- message-missed-email: The event used when an email  was received but not seen by the user.
- message-missed-instant: The event used when a instant message was received but not seen by the user.
- message-missed-sms: The event used when a sms message was received but not seen by the user.
- phone-incoming-call: This Event is used when a phone/voip call is coming in.
- phone-missed-call: This Event is used when a phone/voip call is was incoming but not answered.
- battery-caution: The event used when the battery is nearing exhaustion (below 40%, for example)
- battery-full:	The event used when the battery is fully loaded up.
- device-added: The event used when a device has become available to the desktop, i.e. due to USB plugging.
- power-plug: The power cable has been plugged in.
- power-unplug: The power cable has been unplugged.
- alarm-clock-elapsed: A user configured alarm elapsed.

### Actions

- message-sent-instant: The sound used when a new IM is sent.
- bell-terminal: The sound to use as a terminal bell.
- theme-demo: A event that should be played for demoing this theme. Usually
  this should just be an alias for a very representative sound (such as
  a incoming phone call) of a theme that would work nicely as a demo event for
  a theme in the theme selector dialog.

### Input Event

- button-pressed:	The Event used when a button is pressed.
- window-close:     The sound used when an existing window is closed.

[Sound naming spec]: http://0pointer.de/public/sound-naming-spec.html
