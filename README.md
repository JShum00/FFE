# Frontline Forge Engine (FFE)

A custom fork of id Tech 4, derived from the RBDOOM-3-BFG codebase, designed to support the development of a cinematic World War II first-person shooter.

---

## 🎯 Overview

Frontline Forge Engine (FFE) is a restructuring of id Tech 4 focused on building a grounded, narrative-driven FPS experience.

This engine powers:

> **Crimson Star: Ruins of Ash**  
> A cinematic WWII FPS set during the Battle of Stalingrad.

FFE transforms the original engine by removing DOOM-specific gameplay systems and introducing features suited for human-scale combat, squad-based encounters, and environmental storytelling.

---

## 🧠 Design Philosophy

FFE is built around:

- Cinematic, linear mission design  
- Hybrid realism combat (grounded but playable)  
- Squad presence as environmental pressure  
- Strong atmosphere through lighting and audio  
- Controlled, scripted battlefield chaos  

This is not a DOOM port. It is a foundation for a new game.

---

## 🔧 Key Modifications (Planned & In Progress)

- Replacement of monster AI with human combat AI  
- Modernized input and player movement  
- Weapon systems redesigned for WWII combat  
- Mission scripting for cinematic sequences  
- Improved audio systems for battlefield immersion  
- Cleanup of legacy DOOM-specific systems  

---

## 📁 Project Structure
/engine Core engine code (id Tech 4 fork)
/game Game-specific logic and systems
/assets Game content (non-GPL)
/docs Design documents and technical notes
/tools Build tools and utilities
---

## ⚙️ Building the Engine

### 🐧 Linux (Recommended)

#### 1. Install Dependencies

**Debian / Ubuntu:**

Install ISPC:
```sudo snap install ispc```

---

#### 2. Install DirectX Shader Compiler (DXC)

Download from:
https://github.com/microsoft/DirectXShaderCompiler/releases

Then:
```
chmod +x dxc
mv dxc ~/.local/bin/
```

---

#### 3. Generate Build Files
```
cd neo/
./cmake-linux-release.sh
```
---

#### 4. Compile
```
cd ../build
make -j$(nproc)
```
---

#### 5. Run
```./RBDoom3BFG```

(Note: binary renaming will be done later in development)

---

## 📦 Game Data (Required)

FFE requires original DOOM 3 BFG assets for now.

You must provide:
- `base/` folder from DOOM 3 BFG Edition

Options:
- Steam install
- GOG install
- SteamCMD

Place in:
```<repo_root>/base/```

---

## ⚖️ License

This project is licensed under:

**GNU GPL-3.0-or-later**

- Engine code modifications must remain open source  
- Game assets (models, audio, maps) are NOT covered by GPL  

See:
- `LICENSE.md`
- `LICENSE_EXCEPTIONS.md`

---

## 🧱 Credits

- id Software — original id Tech 4 engine  
- Robert Beckebans & contributors — RBDOOM-3-BFG  

---

## 🚧 Project Status

Early development.

Current focus:
- Engine setup and validation  
- System refactoring  
- First playable vertical slice (Stalingrad, July 1942)  

---

## 🪖 Vision

To create a WWII FPS that emphasizes:

- the confusion of first contact  
- the exhaustion of prolonged combat  
- the transformation of a city into ruins  

---

## 📌 Notes

- This project is in active development  
- Many systems are placeholders or under heavy modification  
- Expect instability during early stages  

---
