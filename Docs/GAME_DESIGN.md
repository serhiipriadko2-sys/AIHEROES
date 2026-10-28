# AI HEROES · Game design v0.1

## Pillar
Heroes-like strategic turns and armies, in a freely rotatable **full 3D** world. The player is the strategist; a growing AI hero is their adviser and tactical commander.

## Player vs AI authority
**Player** chooses expansion, recruitment, upgrades, whether to attack, how to spend resources, when to end a turn. **AI** advises on those choices, but never overrides them; during combat, the AI runs the battle.

## MVP loop
1. See your headquarters and discover mines / guarded nodes.
2. Recruit militia and scouts.
3. Ask for advice (initially a limited offline rules adviser).
4. Click a guarded node to order an attack.
5. AI resolves battle automatically; player sees casualties and a trace.
6. Capture an income node, end turn, gain resources.
7. Buy a cognition/protocol upgrade and change how the hero reasons.

## Cognition as progression (planned, not false model claims)
| Tier | Player-visible ability | Actual engine status |
|---|---|---|
| Seed / 1B theme | Basic factual status, simple orders | Rule-based simulation |
| Analyst / 3B theme | Threat analysis, warnings, battle forecast | Rule-based upgrade |
| Archivist / 3B theme | Previous battle lessons, structured journal | In-memory battle log |
| Strategist / 8B theme | Deeper target selection and multi-step planning | Deterministic tactical policy |

A future adapter may load a real local/mobile model or use opt-in remote inference. Do not promise an actual 1B/8B weight swap without measuring latency, RAM, battery and licensing. Protocols must unlock features, not claim to alter a model's weights.

## Design constraints
- Turn logic is authoritative; text generation is advisory.
- The adviser receives a typed, filtered game-state snapshot; not raw system files or secrets.
- The battle controller returns a structured command; validation happens in engine code.
- Same initial state + policy version yields identical combat result (to permit tests/replays).
- The player can reject any advice and initiate/avoid attacks.
- AI failures and limits must be visible.

## Later milestones
1. v0.1 3D map, nodes, resources, battles, upgrades.
2. v0.2 save/load, pathfinding, recruitable unit variety, deeper economies.
3. v0.3 on-device model experiments, typed prompts, optional file knowledge, consent and safe memory.
4. v0.4 Android input/UI, asset pipeline via Blender, optimizations, Android packaging.
5. v1 larger maps, faction NPCs, strategic AI rivals, richer combat animations.
