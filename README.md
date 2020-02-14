Haptic/visual/audio feedback for GNOME
======================================
[![Code coverage](https://source.puri.sm/Librem5/feedbackd/badges/master/coverage.svg)](https://source.puri.sm/guido.gunther/feedbackd/commits/master)

feedbackd provides a DBus daemon (feedbackd) to act on events to provide
haptic, visual and audio feedback. It offers a library (libfeedback) and
GObject introspection bindings to ease using it from applications.

## License

feedbackd is licensed under the GPLv3+ while the libfeedback library is
licensed under LGPL 2.1+.

## Getting the source

```sh
git clone https://source.puri.sm/Librem5/feedbackd
cd feedbackd
```

The master branch has the current development version.

## Dependencies
On a Debian based system run

```sh
sudo apt-get -y install build-essential
sudo apt-get -y build-dep .
```

For an explicit list of dependencies check the `Build-Depends` entry in the
[debian/control][] file.

## Building

We use the meson (and thereby Ninja) build system for feedbackd.  The quickest
way to get going is to do the following:

    meson . _build
    ninja -C _build
    ninja -C _build test
    ninja -C _build install

## Running
### Running from the source tree
To run the daemon use

```sh
_build/run
```
You can introspect and get the current theme with

```sh
gdbus introspect --session --dest org.sigxcpu.Feedback --object-path /org/sigxcpu/Feedback
```

and to request feedback for an event

```sh
gdbus call --session --dest org.sigxcpu.Feedback --object-path /org/sigxcpu/Feedback --method org.sigxcpu.Feedback.Feedback 'my.app.id' 'phone-incoming-call' '[]' 0
```

See `examples/` for a simple python example using GObject introspection.

# How it works

We're using a [event naming spec](./Event-naming-spec-0.0.0.md)
similar to http://0pointer.de/public/sound-naming-spec.html to name
events. This will allow us to act as a system sound library so
applications only need to call into this library and things like
the quiet and silent profile work out of the box.

## Feedback theme
Events are then mapped to a specific type of feedback (sound, led, vibra) via a
device specific theme (since devices have different capabilities).

There's currently only a single hardcoded theme named `default`. The currently
available feedback types are:

- Sound (an audible sound from the sound naming spec)
- VibraRumble: haptic motor rumbling
- VibraPeriodic: periodic feedback from the haptic motor

You can check the feedback theme and the classes (prefixed with Fbd)
for available properties. Note that the feedback theme API (including
the theme file format) is not stable but considered internal to the
daemon.

## Profiles
The profile determines which parts of the theme are in use:

- `full`: Use conigured events form the `full`, `quiet` and `silent` parts of
  the feedback them.
- `quiet`: Use `quiet` and `silent` part from of the feedback theme. This usually
  means no audio feedback.
- `silent`: Only use the `silent` part from the feedback theme. This usually means
  to not use audio or vibra.

It can be set via a GSetting

```sh
  gsettings set org.sigxcpu.feedbackd profile full
```
## fbdcli

`fbdcli` can be used to trigger feedback for different events. Here are some examples:

### Phone call
Run feedbacks for event `phone-incoming-call` until explicitly stopped:

```
_build/cli/fbcli -t 0 -E phone-incoming-call
```

### New instant message
Run feedbacks for event `message-new-instant` just once:

```
_build/cli/fbcli -t -1 -E message-new-instant
```

### Alarm clock
Run feedbacks for event `message-new-instant` for 10 seconds:

```
_build/cli/fbcli -t 10 -E alarm-clock-elapsed
```

# Documentation

- [Libfeedback API](https://honk.sigxcpu.org/projects/feedbackd/doc/)
- [Event naming spec draft](./Event-naming-spec-0.0.0.md)
- [Feedback-theme-spec draft](./Feedback-theme-spec-0.0.0.md)

[debian/control]: ./debian/control#L5
