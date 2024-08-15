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
_build/run _build/src/feedbackd
```

To run under gdb use

``` sh
FBD_GDB=1 _build/run _build/src/feedbackd
```

You can introspect and get the current theme with

```sh
gdbus introspect --session --dest org.sigxcpu.Feedback --object-path /org/sigxcpu/Feedback
```

To run feedback for an event, use [fbcli](#fbcli)

See `examples/` for a simple python example using GObject introspection.

# How it works

We're using a [event naming spec](./doc/Event-naming-spec-0.0.0.md)
similar to http://0pointer.de/public/sound-naming-spec.html to name
events. This will allow us to act as a system sound library so
applications only need to call into this library and things like
the quiet and silent profile work out of the box.

Any feedback triggered by a client via an event will be stopped latest when the
client disconnects from DBus. This makes sure all feedbacks get canceled if the
app that triggered it crashes.

## Feedback theme
Events are then mapped to a specific type of feedback (sound, led, vibra) via a
device specific theme - since devices have different capabilities and
different users different needs.

Feedbackd is shipped with a default theme `default.json`.
You can add your own themes in multiple ways:

1. By exporting an environment variable `FEEDBACK_THEME` with a path to a
   valid theme file (not recommended, use for testing only), or
2. By creating a theme file under `$XDG_CONFIG_HOME/feedbackd/themes/default.json`.
   If `XDG_CONFIG_HOME` environment variable is not set or empty, it will
   default to `$HOME/.config`, or
3. By creating a theme file under `$XDG_CONFIG_HOME/feedbackd/themes/custom.json`.
   You only specify the values you want to change in that theme and add an entry
   ```json
   {
      "name: "custom"
      "parent-theme": "default"
	  "profiles" : [
	   ...(entries you want to change go here)...
	   ]
   }
   ```
   next to the `name` entry in. This has the upside that your theme
   gets way smaller and that new entries added to the default theme
   will automatically be used by your theme too. See
   [here](./tests/data/user-config/feedbackd/themes/custom.json) for
   an example. Once you have the file in place, tell feedbackd the
   them you want to use:

        gsettings set org.sigxcpu.feedbackd theme custom

   When you want to go back to the default theme just do:

        gsettings reset org.sigxcpu.feedbackd theme

   Note that you can name your theme as you wish but avoid theme names
   starting with `__` or `$` as this namespace is reserved. This is the
   preferred way to specify a custom theme.

4. By adding your theme file to one of the folders in the `XDG_DATA_DIRS`
   environment variable, appended with `feedbackd/themes/`. This folder isn't
   created automatically, so you have to create it yourself. Here's an example:
   ```bash
   # Check which folders are "valid"
   $ echo $XDG_DATA_DIRS
   [ ... ]:/usr/local/share:/usr/share
   
   # Pick a folder that suits you. Note that you shouldn't place themes in
   # /usr/share, because they would be overwritten by updates!
   # Create missing directories
   $ sudo mkdir -p /usr/local/share/feedbackd/themes
   
   # Add your theme file!
   $ sudo cp my_awesome_theme.json /usr/local/share/feedbackd/themes/
   ```

Upon reception of `SIGHUP` signal, the daemon process will proceed to retrigger
the above logic to find the themes, and reload the corresponding one. This can
be used to avoid having to restart the daemon in case of configuration changes.

Check out the companion [feedbackd-device-themes][1] repository for a
selection of device-specific themes. In order for your theme to be recognized
it must be named properly. Currently, theme names are based on the `compatible`
device-tree attribute. You can run the following command to get a list of valid
filenames for your custom theme (**Note**: You must run this command on the
device you want to create the theme for!):

```bash
$ cat /sys/firmware/devicetree/base/compatible | tr '\0' "\n"
```

Example output (for a Pine64 PinePhone):

```bash
$ cat /sys/firmware/devicetree/base/compatible | tr '\0' "\n"
pine64,pinephone-1.2
pine64,pinephone
allwinner,sun50i-a64
```

Thus you could create a custom feedbackd theme for the Pinephone by placing a
modified theme file in
`/usr/local/share/feedbackd/themes/pine64,pinephone.json`

If multiple theme files exist, the selection logic follows these steps:

1. It picks an identifier from the devicetree, until none are left
2. It searches through the folders in `XDG_DATA_DIRS` in order of appearence,
   until none are left
3. If a theme file is found in the current location with the current name,
   **it will be chosen** and other themes are ignored.

If no theme file can be found this way (i.e. there are no identifiers and
folders left to check), `default.json` is chosen instead. Given the above
examples:

- `/usr/local/share/feedbackd/themes/pine64,pinephone-1.2.json` takes
  precedence over `/usr/local/share/feedbackd/themes/pine64-pinephone.json`
- `/usr/local/share/feedbackd/themes/pine64-pinephone.json` takes precedence
  over `/usr/share/feedbackd/themes/pine64-pinephone-1.2.json`
- etc...

For available feeddback types see the [feedback-themes][](5) manpage.

You can check the feedback theme and the classes (prefixed with Fbd)
for available properties. Note that the feedback theme API (including
the theme file format) is not stable but considered internal to the
daemon.

## Profiles
The profile determines which parts of the theme are in use:

- `full`: Use configured events from the `full`, `quiet` and `silent` parts of
  the feedback them.
- `quiet`: Use `quiet` and `silent` part from of the feedback theme. This usually
  means no audio feedback.
- `silent`: Only use the `silent` part from the feedback theme. This usually means
  to not use audio or vibra.

It can be set via a GSetting

```sh
  gsettings set org.sigxcpu.feedbackd profile full
```
## fbcli

`fbcli` can be used to trigger feedback for different events. Here are some examples:

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

## Examples

Here's some examples that show how to use libfeedback in your application:

### C

The command line tool [`fbcli`](./cli/fbcli.c) can be used as example
on how to use libfeedback from C.

### Python

There's an [`example.py`](./examples/example.py) script demonstrating
how to use the introspection bindings and how to trigger feedback via
an event.

### Rust

The [libfeedback-rs](https://gitlab.gnome.org/guidog/libfeedback-rs) Rust
bindings ship an [example](https://gitlab.gnome.org/guidog/libfeedback-rs/-/blob/main/libfeedback/examples/hello-world.rs?ref_type=heads)
to demo the usage.

## Per app profiles
One can set the feedback profile of an individual application
via `GSettings`. E.g. for an app with app id `sm.puri.Phosh`
to set the profile to `quiet` do:

```
GSETTINGS_SCHEMA_DIR=_build/data/ gsettings set org.sigxcpu.feedbackd.application:/org/sigxcpu/feedbackd/application/sm-puri-phosh/ profile quiet
```

# Documentation

- [Libfeedback API](https://honk.sigxcpu.org/projects/feedbackd/doc/)
- [Event naming spec draft](./doc/Event-naming-spec-0.0.0.md)
- [Feedback-theme-spec draft](./doc/Feedback-theme-spec-0.0.0.md)

[debian/control]: ./debian/control#L5
[1]: https://source.puri.sm/Librem5/feedbackd-device-themes
[feedback-themes]: ./doc/feedback-themes.rst
