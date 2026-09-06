[README.md](https://github.com/user-attachments/files/31884292/README.md)
# C_LAN_Chatroom

A peer-to-peer LAN chat application written in C, featuring a terminal UI (ncurses), raw TCP socket networking with multithreading, and — the interesting part — a **custom Linux kernel character device driver** used as the application's message store instead of a file or database.

![Language](https://img.shields.io/badge/language-C-00599C.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![UI](https://img.shields.io/badge/UI-ncurses-2E8B57.svg)
![Kernel Module](https://img.shields.io/badge/includes-kernel%20module-orange.svg)
![Build](https://img.shields.io/badge/build-Make-red.svg)

## Table of Contents

- [Overview](#overview)
- [Highlights](#highlights)
- [Architecture](#architecture)
- [Tech Stack](#tech-stack)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Build & Run](#build--run)
- [Usage](#usage)
- [Configuration Notes](#configuration-notes)
- [Known Limitations](#known-limitations)
- [Possible Improvements](#possible-improvements)
- [Team](#team)

## Overview

`C_LAN_Chatroom` lets machines on the same local network discover each other and chat in real time from the terminal — with no central server. Every instance of the app is *both* a TCP server and a TCP client: on startup it scans the local subnet for other running instances, connects to whichever it finds, and from then on broadcasts every message it sends to all connected peers, forming a full-mesh network.

Instead of storing chat history in a plain file or an external database, message persistence is implemented as a small **Linux kernel module** that exposes a character device (`/dev/chatdb`). The user-space app talks to it with plain `open()`/`read()`/`write()`/`lseek()` calls, exactly like any other device file.

## Highlights

- **Custom kernel-space storage layer** — a real out-of-tree kernel module (`char_device.c`) implementing `file_operations` (`open`, `read`, `write`, `release`, `llseek`), backed by an in-kernel array of messages guarded by a `mutex`.
- **Serverless peer discovery** — no fixed server/client roles; each node listens on TCP `50000` *and* actively scans the local `/24` subnet on startup to find and connect to other peers (`ConnectToIPs`).
- **Multithreaded networking** — a dedicated `pthread` accepts incoming connections, and every peer connection gets its own receiver thread, so the UI never blocks on network I/O.
- **Custom wire protocol** — a simple `TIMESTAMP=...|SENDER=...|MESSAGE=...` pipe-delimited format shared between the socket layer and the kernel device, used both for transmission and storage.
- **Terminal UI with ncurses** — a multi-window layout (status header, scrolling chat log with alternating colors, input line with manual echo and backspace handling), driven entirely by raw `ncurses` calls.

## Architecture

```
┌──────────────────────────── Node A ─────────────────────────────┐
│                                                                   │
│   ncurses TUI (main.c)                                           │
│        │  read_message()  /  display_recent_messages()          │
│        ▼                                                         │
│   network.c  ── orchestrates sockets, threads & persistence      │
│        │                                │                        │
│        │ TCP :50000 (accept + connect)  │ open/read/write/lseek  │
│        ▼                                ▼                        │
│   sockets + pthreads              /dev/chatdb (kernel module)    │
│   • 1 accept thread                • ChatMessage[MAX_MESSAGES]   │
│   • 1 receive thread per peer      • mutex-protected             │
└─────────────────────────┬─────────────────────────────────────────┘
                           │  TCP mesh — every node connects
                           │  to every other node on the LAN
                           ▼
┌──────────────────────────── Node B ─────────────────────────────┐
│                              ...same stack...                    │
└───────────────────────────────────────────────────────────────────┘
```

On startup, each node passes its own IP as an argument so it can skip itself while scanning the subnet. Incoming and outgoing connections are tracked in the same peer table, so a node that both scans and gets scanned still ends up with exactly one connection per peer.

## Tech Stack

| Layer | Technology |
|---|---|
| Language | C |
| Terminal UI | ncurses (`ncursesw`) |
| Networking | POSIX sockets (TCP/IPv4), BSD socket API |
| Concurrency | POSIX threads (`pthread`) |
| Persistence | Custom Linux kernel module (character device) |
| Build system | GNU Make |
| Target OS | Linux (kernel module requires matching kernel headers) |

## Project Structure

```
C_LAN_Chatroom/
├── src/
│   ├── main.c / main.h            # ncurses UI, input handling, app entry point
│   ├── network.c / network.h      # sockets, threads, peer discovery, protocol
│   ├── char_device.c / char_device.h  # kernel module: /dev/chatdb driver
│   └── model.h                    # shared ChatMessage struct & size limits
├── Makefiles/
│   ├── Makefile                   # top-level orchestrator (app + module)
│   ├── Makefile.chat_app          # builds the user-space ncurses app
│   └── Makefile.char_device       # builds the out-of-tree kernel module
├── init.sh                        # first-time build + module load
├── run.sh                         # load module (if needed) + launch the app
├── delete.sh                      # unload module + clean build artifacts
└── .vscode/                       # editor settings
```

## Prerequisites

This project only runs on Linux, since it builds and loads a kernel module.

```bash
sudo apt update
sudo apt install gcc build-essential make
sudo apt install linux-headers-$(uname -r)
sudo apt install libncurses5-dev libncursesw5-dev
```

You'll also need `sudo` access to insert/remove the kernel module and to run the app (it opens the device file it creates).

## Build & Run

```bash
git clone https://github.com/Dev-Levy/C_LAN_Chatroom.git
cd C_LAN_Chatroom

# First-time setup: builds the app + kernel module, then loads the module
./init.sh

# Launch the chat app (also (re)loads the module if it isn't already loaded)
./run.sh
```

When you're done, unload the module and clean up build artifacts:

```bash
./delete.sh
```

Manual build (without the scripts) works the same way via the top-level `Makefile`:

```bash
cd Makefiles
make            # builds both the app and the kernel module
sudo insmod ../build/char_device/char_device.ko
sudo ../build/app/chat_app $(hostname -I)
```

## Usage

1. Run `./run.sh` on every machine on the same LAN that should join the chat.
2. Enter a username when prompted.
3. Type a message and press **Enter** to broadcast it to all connected peers.
4. Type `exit` to leave the chat and shut the app down cleanly.

New peers on the subnet are picked up automatically; connect/disconnect events update the "Connected Users" counter and status line in the header.

## Configuration Notes

A few values are currently hardcoded in `src/network.c` and `src/model.h` rather than configurable at runtime:

- **Port:** `50000` (both listening and scanning)
- **Subnet scanned for peers:** `192.168.1.0/24`
- **Max simultaneous connections per node:** `10`
- **Max stored messages:** `128` (oldest messages are dropped once the limit is reached)
- **Message/sender size limits:** 512 / 32 characters

If your LAN uses a different subnet, `ConnectToIPs` will need its address range updated to match.

## Known Limitations

Being upfront about these matters for a portfolio piece as much as the features do:

- Peer discovery assumes a `192.168.1.0/24` network — it won't find peers on other ranges without a code change.
- No authentication or encryption — messages are sent as plain text over TCP.
- Message history lives in kernel memory; it's cleared when the module is unloaded or the machine reboots (it isn't written to disk).
- IPv4 only, and capped at 10 concurrent peer connections.
- No automated tests.

## Possible Improvements

- Make the port and subnet configurable (CLI flag or config file) instead of hardcoded.
- Persist messages to disk from the kernel module, or swap in a lightweight embedded database.
- Add basic authentication and TLS for message transport.
- Replace subnet brute-forcing with a proper discovery protocol (e.g. UDP broadcast).
- Add unit tests for the message-parsing and protocol-serialization logic.

## Team

Originally built as a group project. Repository maintained by [Dev-Levy](https://github.com/Dev-Levy).
