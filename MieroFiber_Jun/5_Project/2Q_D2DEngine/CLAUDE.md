# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

This is a Visual Studio 2022 (v143 toolset) C++ solution targeting Windows 10. Open `2Q_D2DEngine.sln` and build from Visual Studio. Supported configurations: Debug/Release × Win32/x64.

There is no CLI build script. To build from command line, use:
```
msbuild 2Q_D2DEngine.sln /p:Configuration=Debug /p:Platform=x64
```

## Architecture

A 2D game engine built on **Direct2D** with **FMOD** audio, structured as three projects:

```
GameApp (Executable)
├── Engine (Static Library)
│   └── D2DRenderer (Static Library)
└── D2DRenderer (Static Library)
```

### D2DRenderer
Low-level Direct2D wrapper. Manages `ID2D1Factory`, `ID2D1HwndRenderTarget`, `IWICImagingFactory`, bitmap caching, brush/text format creation. This is a thin abstraction—game code accesses D2D render targets through it.

### Engine
Core framework providing:
- **Component-Entity system**: `GameObject` owns `Component` instances via templates (`CreateComponent<T>()`, `GetComponent<T>()`). Components form a hierarchy: `Component` → `SceneComponent` (transforms) → `RenderComponent` (rendering) → specialized types (`TextureComponent`, `AnimationComponent`, `Collider2D`, `CameraComponent`).
- **World/Scene management**: `WorldManager` switches between `World` subclasses. Each `World` holds `GameObject` arrays indexed by `GROUP_TYPE` enum and manages collision/UI per-scene. Worlds implement `Enter()`/`Exit()` for lifecycle.
- **Collision**: `CollisionManager` with AABB and circle collision. Collision matrix enables/disables group pairs. Callbacks: `OnCollisionEnter/Stay/Exit`, `OnTriggerEnter/Stay/Exit`.
- **FSM**: `FSMComponent` → `FSM` → `FSMState` with `FSMTransition` functors for state condition checks.
- **Event system**: `EventManager` (publisher-subscriber). Components implement `EventListener::HandleEvent()`. Events are queued and dispatched in `EventManager::Update()`.
- **Input**: `InputManager` singleton wrapping Windows keyboard (VK codes) and XInput gamepad. States: `NONE`, `PUSH`, `HOLD`, `END`.
- **Singletons**: `InputManager`, `EventManager`, `SoundManager`, `TimeManager` are all singletons.

### GameApp
A 4-player competitive minigame ("Only Won") using the engine. Players are raccoon characters collecting money, using items, avoiding obstacles (trucks, trains). Key game-specific types:
- `PlayerObject` (×4) with per-player FSMs (`PlayerFSM1`–`PlayerFSM4`) containing `IdleState`, `MoveState`, `StunState`
- Item system: `eItemType` (Throw/Installation/Reinforced) with subtypes (Bottle, Punch, Stungun, Trap, Snare, Shoe, Wave, Snow, Transparency)
- Worlds: `MainWorld`, `InGameWorld`, `GameSettingWorld`, `InstructionWorld`, `MadeByWorld`
- UI components: `ButtonUI`, `BarUI`, `PanelUI`, `PopUpUI` with grid-based gamepad navigation

## Game Loop

`CommonApp::Loop()` runs: `TimeManager::Update()` → `InputManager::Update()` → `WorldManager::Update(dt)` (which updates all GameObjects and their Components, then CollisionManager, then EventManager) → `Render()` (CameraComponent transform, then each RenderComponent draws to D2D render target).

## Key Conventions

- Korean comments throughout the codebase (Korean-speaking development team)
- Resource files (textures, audio) are in `Resource/` directory at the solution level, with some filenames in Korean
- FMOD libraries are at `Resource/fmod/lib/` (x64: `fmod_vc.lib`)
- `GROUP_TYPE` enum defines rendering/collision layers: BACKGROUND, SPAWNER, ITEMBOX, MONEY, OBSTACLE, TRUCK, ITEM, PLAYER, TRAIN, Environment, UI, BARUI, POPUPUI
- `WORLD_TYPE` enum: TEST, JUN, CHAE, MAIN, INSTRUCTION, MADEBY, GAMESETTING, INGAME
- Transform hierarchy uses `D2D1::Matrix3x2F` with relative/world/final transforms propagated through `SceneComponent`
- Assets use a `ReferenceCounter` base class for shared ownership
- `X1nput/` directory exists at solution level (XInput-related)
