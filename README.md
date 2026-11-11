
## Latest Update — idkOS v1.3.0 🚇

### Home Themes & Calendar Update

**Three looks. One OS. More functionality.**

idkOS V1 continues to evolve!

Version 1.3.0 introduces three selectable home screen themes, an offline Calendar integrated with Cache reminders, and improvements to the user interface.

### What's New

**Three Home Screen Themes**

- **Subway** — A colorful, subway-inspired interface with station-style navigation.
- **Metro Minimal** — A clean, modern layout with clock and weather information.
- **Classic V1** — The familiar original idkOS home screen.

Change themes through:

**Settings → Display → H (Home theme)**

Theme preferences are saved and restored after sleep and reboot.

### Calendar & Cache Integration

- Offline monthly calendar.
- Cache reminders displayed on their scheduled dates.
- Highlighted current and selected dates.
- View reminders for a selected day.
- No internet connection required.

**Calendar Shortcuts**

- `Fn + 6` — Open Calendar
- `[` and `]` — Change months
- `T` — Jump to today
- `Enter` — Open the selected day's reminders

Calendar is read-only. Reminders are created and edited through Cache.

### V1 Development Continues

Although idkOS V2 is currently in development, V1 remains actively supported.

Future updates will continue to focus on stability, practical improvements, and making idkOS more useful for everyday use.

**Useful when you need it. Quiet when you don't.**

[View all idkOS releases](https://github.com/Shizz828/idkOS/releases)


# idkOS  <img width="1672" height="941" alt="image" src="https://github.com/user-attachments/assets/b9dcab34-67b1-4a67-9465-70235e424ba8" />  
<img width="1152" height="1536" alt="image" src="https://github.com/user-attachments/assets/b807646a-d916-4f03-b022-3150b59f8c31" />  <img width="1152" height="1536" alt="image" src="https://github.com/user-attachments/assets/9e57e42c-5b20-4565-843c-9ce3f1cb164c" /> <img width="1152" height="1536" alt="image" src="https://github.com/user-attachments/assets/55a8b1f6-54f9-45bc-a219-5c9c48ac4d2a" /> <img width="1152" height="1536" alt="image" src="https://github.com/user-attachments/assets/8495899d-437a-407b-805f-a31d8d52050c" /> <img width="1152" height="1536" alt="image" src="https://github.com/user-attachments/assets/573ab84c-b492-42f0-a3db-5373fec548ca" /> <img width="1152" height="1536" alt="image"
 src="https://github.com/user-attachments/assets/1ac04b4b-f4f3-4379-8b07-7c2ab5ac7c53" /> <img width="1152" height="1536" alt="image" src="https://github.com/user-attachments/assets/40318483-9430-40d5-9d80-20c096dfda89" />








**idkOS** is a lightweight, keyboard-first pocket computing environment built for the **M5Stack Cardputer ADV**.

The project started as an experiment in making the Cardputer more useful as an everyday device: something that can handle notes, reminders, weather, lightweight information, terminal access, and remote SSH without trying to become a full desktop operating system.

The goal is simple:

**Useful when you need it. Quiet when you don't.**

idkOS V1 is the first public release and establishes the stable foundation for the project.

---

## Features

### Cache

Cache is the built-in notes and reminders application.

It supports:

- Notes stored on microSD
- Quick capture
- Reminders
- Simple recurrence
- Offline access

Cache is designed around the Cardputer keyboard so notes can be created and reviewed without needing another device.

---

### Weather

Weather provides current weather information directly on the Cardputer ADV.

V1 includes:

- Current conditions
- Temperature
- Location-based weather
- Persistent Wi-Fi configuration

Weather uses the network configured through idkOS Settings.

---

### Crypto

Crypto provides lightweight cryptocurrency information from the Cardputer.

The API configuration is stored locally so the key does not need to be re-entered after every reboot.

---

### Terminal

idkOS includes a local terminal for basic device and microSD tasks.

The Terminal is intentionally lightweight and is not intended to replace a full Linux shell.

---

### SSH

Connect from the Cardputer ADV to remote machines over SSH.

This makes idkOS useful as a small remote terminal for:

- Linux systems
- home servers
- development machines
- other SSH-enabled devices

---

### Calendar

Calendar shows an offline month view with Cache reminders marked on their days.

- Month grid with today and the selected day highlighted
- Reminders for the selected day shown beside the grid
- **Enter** opens a full list of that day's reminders
- **[** and **]** change month, **T** jumps to today

Calendar is read-only: reminders are created and edited in Cache.

---

### Home themes

The Home screen comes in three styles:

- **Subway** (default) — cyan subway route with station stops
- **Metro Minimal** — clean, modern look with picture icons
- **Classic V1** — the original idkOS Home

Change it in **Settings → Display → H (Home theme)**. The choice is saved and kept after sleep and restarts.

---

## Navigation

idkOS is designed primarily around the Cardputer keyboard.

### Standard navigation

- **Arrow keys** — navigate menus and lists
- **Enter** — select / confirm
- **Esc** — back / exit
- **G0** — return Home

### Shortcuts

- **Fn + 1 through Fn + 5** — Cache, Weather, Crypto, Terminal, Settings
- **Fn + 6** — Calendar
- **Fn + S** — SSH
- **Fn + Esc** — pocket-safe sleep

Terminal, SSH sessions, and text-entry fields retain their appropriate keyboard behavior.

A shortcut reference is also available inside Settings.

---

## Sleep and idle behavior

idkOS includes power-aware behavior intended for everyday portable use.

The system can dim or turn off the display after inactivity while preserving the current session.

**Fn + Esc** provides a pocket-safe sleep mode for times when the device is being carried.

---

## Wi-Fi

Wi-Fi configuration is managed globally through **Settings**.

Once a supported network is saved, idkOS can reuse it for applications such as Weather and Crypto.

Saved network information is intended to persist across normal reboots.

---

## Hardware

idkOS V1 is built specifically for:

**M5Stack Cardputer ADV**

Do not assume compatibility with the original Cardputer, Cardputer Zero, or other ESP32 devices unless a release specifically lists support for them.

Firmware compiled for the wrong device may fail to boot or may have incorrect display, keyboard, storage, or hardware behavior.

---

## microSD

A microSD card is recommended for the full idkOS experience, particularly for Cache data and file access.

Back up important files before flashing or experimenting with firmware.

---

## Installation

The recommended public release is provided as a precompiled Cardputer ADV firmware binary.

### M5Launcher

idkOS can be installed using M5Launcher when using a compatible Cardputer ADV build.

Depending on your Launcher setup, firmware can be installed from:

- the Launcher online catalog when listed
- a downloaded `.bin` file on microSD
- Launcher WebUI
- a compatible direct firmware link

Always use the idkOS binary intended specifically for the **Cardputer ADV**.

### Manual flashing

Advanced users may also flash the supplied firmware binary using a compatible ESP32 flashing method.

Follow the instructions for your chosen flashing tool and verify that the correct Cardputer ADV target is selected.

---

## First start

After installing idkOS:

1. Boot the Cardputer ADV.
2. Open **Settings**.
3. Configure Wi-Fi if required.
4. Open the applications you plan to use.
5. Configure any required application-specific API information.

Once configured, supported settings should persist across normal reboots.

---

## Crypto key setup

Crypto uses a free CoinGecko Demo API key, loaded from a computer over USB. You need Python 3 with `pyserial` (`pip install pyserial`) and this repository.

```sh
python3 scripts/configure_coingecko.py                  # paste the key (hidden); saved locally in secrets/
python3 scripts/provision_crypto.py --port /dev/ttyACM0 # with idkOS running; close any serial monitor first
```

The key is stored on the device and is not shown on screen.

---

## V1 philosophy

V1 is intentionally focused.

It does not attempt to include every possible Cardputer feature.

The priority for this release is:

- stability
- fast navigation
- useful everyday tools
- readable UI
- predictable storage
- low-friction operation

Experimental features are developed separately before they are considered for public releases.

---

## V1 applications

The public V1 release focuses on the core idkOS experience.

Experimental creator-build features and games are not included in this release.

---

## Project status

idkOS is an independent community project for the M5Stack Cardputer ADV.

It is not an official M5Stack product.

Future versions are expected to expand the interface and utility set while keeping the ADV version lightweight and practical.

---

## Safety and backups

Custom firmware always carries some risk.

Before flashing:

- back up important microSD files
- confirm you have the correct firmware for your hardware
- keep a known-good recovery method available

Do not store your only copy of important information on an experimental embedded device.

---

## License

idkOS is released under the [MIT License](LICENSE).

Third-party components and data sources keep their own licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

---

## Version

**idkOS V1**

Built for the **M5Stack Cardputer ADV**.


## The Story Behind idkOS


The idea behind idkOS started with two things: an Idea Saver I came across on Instagram and my first experience using PicoWare on the M5Stack Cardputer ADV.

When I saw the Idea Saver, I liked the concept of having a small, portable device for capturing notes and ideas. That eventually became the inspiration for Cache.

Later, after getting my Cardputer ADV and experimenting with PicoWare, I realized I could create my own OS and set it up the way I wanted. Instead of relying entirely on what someone else had built, I could decide which features to include, how everything looked, and how it all worked together.

My goal was simple: make the Cardputer something I could actually use in everyday life.

I wasn't trying to replace my computer or phone, and I didn't know much about the security side of the ADV yet. I wanted a lightweight, practical environment that took advantage of what the Cardputer already offered: a keyboard, portability, Wi-Fi, and local storage.

More importantly, I wanted useful features that I could take with me and access offline whenever possible.

What started as an experiment gradually turned into something bigger. I began adding features like notes, reminders, weather, cryptocurrency information, and terminal access. Along the way, I learned more about the hardware, worked through bugs, and discovered just how much I could actually do with this little device.

Eventually, I realized I wasn't just experimenting anymore. I was building something I wanted to keep using and improving.

**Now, about that name...**

Naming this project turned out to be harder than building parts of it.

Every time I came up with a name I liked, I discovered that another project was already using it.

After going through name after name, I finally got frustrated and said:

**"I don't know what to call this shit. idkOS!!"**

And just like that, the name stuck.


