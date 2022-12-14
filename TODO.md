* need a better "extra" that allows shot-module like sharing of:
  * themes
  * wallpapers
  * ... anything else?
  * built in tool to listbrowse/preview/download etc.
  * built in tool should allow for share/upload too
  * on the www side - nicely show and index themes with previews in html
* clean up e profiles, elm profiles and per screen profiles
  * move to a new profile method which has a fallback profile
    * also has to include scaling options like manual or based on dpi
  * then per screen profiles + scaling too  but do this all via the per
    screen profile infra - always go that path
* illume: work on this again given pinephone + librem5 appeared
  * single app at a time with switching mode
    * winlist should do the switching with another layout mode...
  * shelf can expamnd/slow down to show more (indicator/notification)
    * gadget bar too... ? or just gad bar?
  * shelf autohide. Move this to edje.
  * vkbd integrates with proper sizing and layout
    * manual show/hide with various controls (actions, swipes, gadget)
  * finger friendly desklock
    * integrate with vkbd, also allow pin no. or pattern or camera or bt?
  * notification in gadget i shelf (pull down)
    * scrollable list with / X/swipe dismiss
  * handle edge swipes (middle/left/right of edge) with edge bindings
    * also swipe along an edge (in each dir)
  * add swiping to type in vkbd
  * home screen launcher + switcher (single instance mode debugged and on)
  * pre-launch window show with icon/splash img/past screenshot show
    * replace when app actually starts and shows window
  * aggressive sigstop of bg apps unless whitelisted or when desklocked/blanked
  * aggressive cpufreq limiting when desklocked/blanked
  * ambient light sensor control of backlight
  * hide/show mouse pointer base on real mouse device
  * vibrator support api in e (efl)
  * hide/show vkbd based on real kbd device pluggd/unplugged
  * pull in convertible for rotation
  * need to handle ofono / incoming call+sms notifications without
    dialler always running
  * longpress bindings for key/mouse/etc.
  * use xinput for edge bindings and other gestures (always listen to ev)
  * need apps:
    * dialler/phone incoming call handling
    * sms app
    * camera app (video and still)
    * gallery app
    * audio recorder + playback
    * browser (just use ffox etc.?)
    * ofono config
      * bt tethering
      * usb tehtering
      * wifi tethering
  * fix vkbd to work in wl - e_kbd_send.c - look at ecore_wl2_input.c
* clean up some old actions that are handled better by newer ones
  * like suspend intelligently is better than regular suspend but
    suspend now is very explicit and worth keeping
* clock - add event/alarm/appointment tracking and notifications etc.
  * this perhaps can be merged with the cron front-end
  * cmdline tools to import/export events and alarms
  * dbus api to imports/export calendar
  * some kind of intrgration to ical/outlook and other calendar data
  * display events/alarms in calendar
  * ability to wake up from screen sleep and show an alarm or
    notification even if locked
  * ability to wake up from suspend/hibernate like above
  * ability to even power on/boot from nothing
    * the above will require RTC and other deep integration...
* magnifier - need a magnifier tool
  * live follow the mouse and zoom around it in some corner /region of
    the screen
  * toggle on/off or bring up only while key or button pressed
  * freeze the zooms like a screenshot/xmag like thing
  * offer coordinates and measuring by pixel
* pam/auth - exotic auth things like fingerprint readers etc. need to work
  * smart card
  * face auth (howdy / pam-face / pam-face-authentication / pam-facial-auth)
  * yubikey
  * usb drive (pam_usb)
  * support setting up smartcard as auth
  * support setting up face auth (howdy / pam-face / pam-face-authentication / pam-facial-auth)
  * support setting up yubikey as auth
  * support setting up usb drive for auth (pam_usb)
* wifi/bt net: associate app/desktop(s) or actions to run when:
  * bt device comes or goes (alongside lock device feature)
  * wifi appears or disappears
  * wifi or wired network connected or disconnected to/from
  * perhaps need a more generic "events" bindings and these are events?
* clipboard manager - monitor clipboard changes + history + select from it
* scriptlets: stdin/out module that provides gui services to some
  back-end script or process where e just talks via stdin/out.
  things to provide:
  * only run scriptlet every N seconds to poll then send back data
  * only run scriptlet at specific times of day (cron-like)
  * only run scriptlet when some event happens
    * dbus signals (some way to define what to listen for)
    * acpi bindings
    * key bindings
    * mouse bindings
    * startup, shutdown, suspend, resume, lock/unlock, ...
    * ... screen add/del, backlight dim/undim, ...
    * ... blank/unblank, ac plug/unplug, battery full, ...
    * ... battery charge begin, battery charge end, ...
    * ... network connect/disconnect (connman), ...
    * ... bt device find/lose, ...
    * ... volume/mute/audio dev change, virtual desktop change, ...
    * ... app window appear/disappear/exit, ...
    * ... something goes fullscreen/unfluscreen, ...
    * other events on the system or in e?
  * run script all the time and talk to it and "keep state"
  * provide menu content in menu specific tree...
  * provide popups like pager mid-screen one with content like below...
  * provide gadgets with display content like below...
  * provide gadget popups (like mixer/clock) with content like below...
  * provide full windows with content like below...
  * possible content in a tree:
    * probably have to provide markdown of below in some way...
      * thinkimg of mustache style {{{ }}} ?
      * shell friendly so it doesnt have special meanings in sh
    * image (png/jpg/gif/video ... whatever)
    * edje theme element/file fallback
      * send/get signals
      * set text parts
      * send/get messages
      * create, delete, swallow other edje/image objects
    * box
    * table
    * grid
    * scroller (content holder)
    * lists to select from (icon, text, icon at end, groups - genlist)
    * button
    * radio
    * check
    * label/text
    * entry (editable single/multi line, passwd)
    * separator
    * frame (content holder)
    * slider
    * progress/busy spinner
    * dropdown/hoversel
    * graph (multiple overlayed with N points horiz or vert, line/solid)
    * dials (like cpufreq)
    * mixers like ... mixer
    * temperature guages like temperature
    * scrollable logs with per line/item content objects
    * list trees (genlist)
    * grid lists (gengrid)
    * efm show dir
    * efm open specific file with default mime handler
    * efm open specific file with mime handler chooser
  * tell scriptlet where it is stored (env var)
  * tmpdir setup/deletion for scriptlet (env var)
  * more permanent config dir setup/storage for scriptlet (env var)
  * pop up specified menu content
  * display screen popup with content
  * display gadget popup with content
  * get events from content (clicked, selected, etc.)
  * get state of content
  * create timers and get called on timeout
  * built in wget via e/ecore_con_url
  * built in tcp/tcp+ssl netcat provided by ecore_con
  * direct notification display (via notification module) e/stdin/out
  * provide built-in editor to edit scripts and auto-re-run as you edit
    * entry or elm code based?
    * massive lowering of barrier ro create/change scriptlets
    * offer a share mechanism like shots?
      * security issue - they run. verification needed?
* S keyboard backlight controls, not just screen
* login manager mode - be able to function as a login manager/gui since
  most everything you want in a login manager is already present in e
  like suspend/resume on lid open/close, multi screen config and even
  auto-extension, backlight and other such controls.
* console logger much like quake console like dropdown for logging etc
  (api to append logs. always stores to disk to specific file.
   add button to explicitly share like screenshots. add higher level
   logging like tables, icons, timelines and graphs that can be output
   in text emulation and to screen - change eina log to go into here
   with eina_log_print_cb_set() )
* settings: config dialog redo and simplification
  * simplify the config and remove useless options
    * focus on new simpler config dialog(s) first
      * cover all the major e config uses now
      * drop some weird/niche/unused stuff
    * once new config is done, remove old config dialogs
  * integrate more config of elm into e
  * add more system setup stuff like:
    * system locale config
    * system locales supported (locale.gen)
    * system timezone as well as user timezone
    * user timezone (set TZ env var)
    * set system clock
    * set up ntp
    * set user password
    * user cron front-end config/editor
    * allow separate LC_MESSAGES, LC_TIME, LC_NUMERIC, LC_NAME, etc.
* dnd: remove e_dnd and use elm's dnd instead
* gadgets:
  * add a simpler interface for registering a gadcon (eg use size hints)
  * long press for right-mouse-menu
  * change to unified shared right-mouse-menu always and simpler content
  * add alt + drag/resize
  * remove all new gadget code
  * break any new gadgets outside of e
* efm: smartctl status info and monitoring
* efm: lvm/raid setup array and status support
* efm: use elm for window
* efm: use elm scroller for fm view
* efm: use elm for file popup
* efm: add disk space used bar for disk volume icons
* efm: add space used "du -sh" summary for all folders
* efm: add filesystem cache (all dirs cached in files and updated in bg)
  * should be a full filesystem shadow stored to cache db files
  * async update/scan of dir and present updates when done and have changes
  * store file, stat info, other metadata, mimetype, icon, space used...
* efm: thumbs for music getting album art like rage
* efm: thumbs for videos with movie posters like rage
* efm: show symlink info in icon
* efm: fuse support (mtp, sshfs, cifs/smb etc.)
* efm: rsync integration for fsync to/from targets
  * may need ssh setup too (add to authorized keys on other end)
* efm: partitioning front end
* efm: filesystem formatting front-end
* efm: tune2fs/fsck front-end
* start: start menu could do with more featured content
  * like you see on windows for example (search and multiple columns etc.)
  * a touch screen android-style scrolling fullscreen app icon style
* S music-control: allow manual addition of new named mpris services
* connman:
  * divide the popup list with group headers like bz5 into tech
  * provide fuller status per line (reception and other info?)
  * massively flesh out connman vpn support in the gui
    * vpnmanager + create method support
  * one day look at network manager support
  * look into iwd too
* bz5: add obex agent support integrated with efm for sharing via bt
  and downloads when received via obex bt (~/Downloads/...)
* battery: redesign popup to list other battery details like the
  charge if you have multiple batteries for real or separately the
  battery levels of e.g. bt mice, kbd's and other such devices
* battery: on click display popup like wireless/bluez5
  in popup show detailed list of battery devices and data per device
  * like bluez5 use genlist with groups for core batteris vs other misc devices
  * filter out hid devices with batteries from core battery state
  * display other devices in above list separately
  * handle string capacity values like Full, High, Normal, Low, Critical...
* randr: get auto layout to handle screens laid out like:
```
  [dp2-2][dp2-1] <- 2 external monitors
      [edp-1] <- laptop main display
```
* randr: when losing a zone/screen, put windows in purgatory and then have
  purgatory screens available via pager to drag onto a current screen,
  or the specific virtual desktops from purgator to be accessible even
  if not on a real screen/zone
* device manager: new tool to browser /sys etc. and display nicely present
  hardware and device drivers and state
* sys top: new tool displays cpu/mem "top" - evisum. we want it to be shipped
  with e though so it's always there guaranteed have back-end system status
  logging to files all day long that always starts with e. gui is a front-end
  view to this back-end data log and follows it as it updates.
  * can go back in time then to previous logs
  * should have the ability to draw nice graphs etc.
* ddc: provide ddc controls for all the other properties than backlight
  (contrast, sharpness, r, g, b, correction, color temp, osd on/off etc.)
* ddc: provide abstract interface to query rotation of monitors...
* edge bindings - selecting edge dialog needs a redo (no edje for all content)
* e menu - add obj cache for menu and menu item objects to reduce costs
  by far the biggest cost in showing/hiding menus is realize/unrealize
  and this is almost all object creation/setup/deletion costs. menus
  can be more responsive (on low end systems) if... we avoid this. to
  avoid... have a cache of spare objecta to re-use.
* e comp object: we dirty far too often in wayland especially with
  glmark and this leads to a stream of gen and del textures. is this
  really needed and ca we cut it down?
* tiling: doesnt retain layout across restarts
* tiling: doesnt handle impossible layouts well - had to remove min size
* tiling: doesnt honor size stepping too well
* tiling: probably need to flatten out to table for saner layout
* tiling: probably could do with analysis on features vs i3 etc. to be
  more "up to snuff" and competitive
* tiling could be smoother in its layout handling
  * smoothly move/resize windows from old to new positions/sizes
  * don't delay many responses and do it instantly with the above anim
  * probably can do with more actions for bindings for moving/sizing
* packagekit: much btter support with pre-made config/profiles for the
  package manager (autodetect if possible)
  * not just updates but also install/uninstall of pkgs with a minimal
    front-end to back-end cmdline tools (use shell wrappers?)
* cpufreq: support pstate: hwp_dynamic_boost, no_turbo, display turbo_pct,
  num_pstates
* advanced per-device config (xinput device by device). current is simple
  generic "global" config e.g. for the mouse
* look into libratbag too for gaming mouse stuff if its worth using it or
  just doing the same
  stuff ourselves as we will probably have to via libinput
* some kind of game mode and detection that will do things like:
  * disable screen dimming/blanking
  * set cpu to max performance
  * lower priority/nice of non-game processes or even sigstop procs
  * disable notifications
  * explicit toggle and possible auto-detect of games running
* per app environment launch config - allow different TZ, LOCALE etc.
  per app and set up env appropriately
* apps - ability to flag apps as "single run" so launching again from
  menu/ibar will just focus the current window/jump to it etc.
* back-end e system process for monitoring/polling and launching etc.
  * split e up into front vs back-end with back-end doing things like
    * monitoring processes/system (see above)
    * launching apps (cheaper to fork a small process)
* synergy-like e <-> e mouse, kbd, cnp, dnd proxy/sharing
* cpufreq - move to config dialog for settings
  * set governor for ac vs battery (or powerstate levels)
  * set pstate min and max for ac vs battery (or power state levels)
* saving power
  * have new property/atom for hidden but visible in a pager orsomething
    * efl - dont suspend render when this is the case but drop fps to 1/4
  * look into x grabserver for long periods (eg 1s) then ungrab for
    0.1 sec then grab again to throttle apps from doing drawing while
    blanked
  * look into SIGSTOP/CONT child processes and their children when blanked
    * optional SIGSTOP while iconified
    * optional SIGSTOP when on another desktop
    * need to allow a CONT every now and again to unblock buffers
* set backlight and dim timeout per powersave level
* battery - set powersave levels based on ac or battery level
* efl/eo: start using efl loop as core loop
  * begin to move some login out into efl threads+loops
    * to change ui - callsync/async back to ui/main loop
  * have a deferred deletion of evas objects in idler
    * hide instead of del, stash in invisible place, delete l8r in idler
  * have object stash for adding basic objects
    * have generic/basic object stash
    * have edje object stash
* mixer: improve ui in emixer
  * emixer - support selecting recording dev for apps like pavucontrol
  * offer ability to show vu output in mixer gadget
* weather
  * bring in forecasts into core e
