# VultureLabs Qt UI

## Overview
A Qt6 desktop application with:
- Modular UI design (`qtui`)
- Structured logging in NDJSON (`opslog.ndjson`)
- Portable Windows distribution (`dist/` with Qt DLLs)

## Features
- ✅ Boot & Action logging
- ✅ Security and Audit entries
- ✅ Portable build via `windeployqt`
- ✅ Extensible UI for future modules

## Quick Start

# Clone
git clone https://github.com/SenaDev2023/vulturelabs.git
cd vulturelabs/qtui

# Build (example with MSYS2/mingw64)
qmake && make

# Or run packaged app
dist/vulturelabs_qtui.exe


===============================

Logs
Application events are written in data/opslog.ndjson
=================================
{"ts":"2025-08-16 21:22:01","category":"Security","detail":"Make Authentication token for GPT for Azure DevOps Stories"}


