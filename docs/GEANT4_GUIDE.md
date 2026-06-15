# How this GEANT4 simulation is built — a learning guide

This walks through every part of a GEANT4 application, using the RADiCAL
simulation in this repo as the worked example. Read it alongside the source in
`include/`, `src/`, and `radical.cc`.

---

## 1. The mental model: what GEANT4 actually does

GEANT4 ("GEometry ANd Tracking") simulates particles moving through matter. You
give it (a) a **geometry** made of **materials**, (b) a list of **physics
processes**, and (c) some **primary particles** to fire in. It then transports
each particle step by step, invoking physics (scattering, energy loss, decays,
producing secondaries) until everything stops, and lets *you* record whatever
you care about.

Everything is organized as a nested loop:

```
Run            one /run/beamOn N  -> N events, fixed geometry & physics
 └ Event       one primary particle (+ all the secondaries it makes)
    └ Track    one particle's whole life (e-, gamma, optical photon, ...)
       └ Step  a small move; physics + energy deposit happen here
```

You hook into this loop with **user action** classes (Run/Event/Tracking/
Stepping/Stacking actions) that GEANT4 calls at the start/end of each level.
This is the single most important idea: *GEANT4 runs the loop; you supply
callbacks that observe it and write out data.*

---

## 2. Anatomy of a GEANT4 application

A GEANT4 program is `main()` plus a set of classes you derive from GEANT4 base
classes. Three are **mandatory** (you must provide them); the rest are optional
hooks.

| Your class (this repo) | Derives from | Role |
|---|---|---|
| `RadicalDetectorConstruction` | `G4VUserDetectorConstruction` | **geometry + materials** (mandatory) |
| physics list (built in `radical.cc`) | `G4VModularPhysicsList` | **which physics processes** (mandatory) |
| `RadicalPrimaryGeneratorAction` | `G4VUserPrimaryGeneratorAction` | **the beam** (mandatory) |
| `RadicalActionInitialization` | `G4VUserActionInitialization` | wires up all the user actions |
| `RadicalRunAction` | `G4UserRunAction` | start/end of a run → open/close ROOT file |
| `RadicalEventAction` | `G4UserEventAction` | per-event bookkeeping → fill ntuples |
| `RadicalSteppingAction` | `G4UserSteppingAction` | look at every step (we detect photons here) |
| `RadicalStackingAction` | `G4UserStackingAction` | manage the stack of tracks-to-do |
| `RadicalCaloSD` | `G4VSensitiveDetector` | record energy in a volume |
| `RadicalDetectorMessenger` | `G4UImessenger` | expose `/radical/...` UI commands |

`radical.cc` (`main`) glues these together:

```cpp
auto* runManager = G4RunManagerFactory::CreateRunManager(Default);  // MT-capable
runManager->SetUserInitialization(new RadicalDetectorConstruction());   // geometry
G4VModularPhysicsList* physics = factory.GetReferencePhysList("FTFP_BERT");
physics->RegisterPhysics(new G4OpticalPhysics());                        // physics
runManager->SetUserInitialization(physics);
runManager->SetUserInitialization(new RadicalActionInitialization());    // actions
// ... visualization + UI, then either an interactive session or run a macro
```

The `G4RunManager` is the conductor: it owns the geometry, physics, and actions,
builds the cross-section tables, and drives the run/event/track/step loop.

---

## 3. Geometry: the Solid → Logical → Physical "trinity"

Every piece of geometry in GEANT4 is built from three layers. This trips up
every beginner, so internalize it:

1. **Solid** — *just the shape and size.* `G4Box`, `G4Tubs` (tube/cylinder),
   `G4ExtrudedSolid` (our hexagons), boolean combinations (`G4SubtractionSolid`
   for the drilled capillary holes). A solid knows nothing about material.
2. **Logical Volume** (`G4LogicalVolume`) — *shape + material + attributes.*
   Combines a solid with a material, and optionally a sensitive detector,
   optical surface, visualization color, etc. Think "a LYSO crystal, defined."
3. **Physical Volume** (`G4PVPlacement`) — *a placed copy in space.* Puts a
   logical volume at a position/rotation inside a mother volume. One logical
   volume can be placed many times (we place one `LYSO` logical 29 times for the
   29 layers) — each placement gets a **copy number** to tell them apart.

```cpp
auto* box   = new G4Box("LYSO", hx, hy, hz);                      // 1. solid
auto* lysoLV = new G4LogicalVolume(box, M->LYSO(), "LYSO");        // 2. logical
new G4PVPlacement(nullptr, {0,0,z}, lysoLV, "LYSO_pv", mother,     // 3. physical
                  false, copyNo, checkOverlaps);
```

Geometry is a **tree**: every volume sits inside a *mother* volume, up to the
single top-level **World**. In this repo (`RadicalDetectorConstruction.cc`):

```
World (air)
 ├ trigger counters A1, A2 (plastic scintillator) + readout patches
 ├ beam chamber
 ├ MCP tube (vacuum) ─ quartz window ─ photocathode
 ├ Module (one logical volume, placed 1 or N×N times)
 │   ├ Tyvek wrap (placed 29×)  └ LYSO crystal inside it   ← the trinity, reused
 │   ├ Tungsten plate (placed 28×)
 │   ├ Capillary (quartz tube) × 5 or 9  └ WLS filament inside
 │   └ SiPM × 8
 └ Pb-glass blocks (2×2) + readout patches
```

Two techniques worth knowing from this geometry:

- **Boolean solids for the holes.** The capillaries run the full length of the
  module, through every tile. We `G4SubtractionSolid` the capillary cylinders
  out of each LYSO/Tungsten/Tyvek solid so the capillaries can be placed without
  overlapping the tiles. (`DrillHoles()` in `RadicalDetectorConstruction.cc`.)
- **Parametrization.** All the switchable options (tile shape, capillary layout,
  module size, array tiling) live in one struct, `RadicalGeometryConfig`, so the
  same builder code produces many detector variants.

### Construct() vs ConstructSDandField()
`Construct()` builds and returns the World physical volume. `ConstructSDandField()`
is called separately (once per thread in multithreaded mode) to attach sensitive
detectors and magnetic fields — see §6.

---

## 4. Materials

Materials carry density, composition, and (for optics) wavelength-dependent
properties. Three ways to get one (`RadicalMaterials.cc`):

```cpp
auto* nist = G4NistManager::Instance();
fWorld = nist->FindOrBuildMaterial("G4_AIR");        // (a) NIST database
fLYSO = new G4Material("LYSO_Ce", 7.10*g/cm3, 4);    // (b) build from elements
fLYSO->AddElement(Lu, 9); fLYSO->AddElement(Y, 1);
fLYSO->AddElement(Si, 5); fLYSO->AddElement(O, 25);
```

For **optical** physics you also attach a `G4MaterialPropertiesTable` giving
properties vs photon energy:
- `RINDEX` (refractive index) — needed for *any* optical transport and for
  Cerenkov radiation.
- `ABSLENGTH` — bulk absorption length.
- `SCINTILLATIONYIELD`, `SCINTILLATIONTIMECONSTANT1`, `SCINTILLATIONCOMPONENT1`
  (emission spectrum) — makes a material scintillate.
- `WLSABSLENGTH`, `WLSCOMPONENT`, `WLSTIMECONSTANT` — wavelength shifting.

Energies must be in **increasing order**, and (lesson learned) a WLS emission
spectrum must lie at *lower* photon energy than what it absorbs (Stokes shift) or
GEANT4 aborts on energy non-conservation.

**Units.** Always multiply numbers by a unit: `7.10*g/cm3`, `1.5*mm`, `50*GeV`,
`3.5*ns`. GEANT4 stores everything in its internal units; the literals like `mm`
and `ns` are constants from `G4SystemOfUnits.hh`. Never write a bare number for a
physical quantity.

---

## 5. Physics list

The physics list selects which processes are active and which models implement
them. Rather than hand-pick, you usually start from a validated **reference
list**. We use `FTFP_BERT` (the standard choice for EM + hadronic showers) and
add `G4OpticalPhysics` for scintillation/Cerenkov/WLS/photodetection:

```cpp
G4VModularPhysicsList* physics = factory.GetReferencePhysList("FTFP_BERT");
physics->RegisterPhysics(new G4OpticalPhysics());
```

Optical photons are produced only where the materials have the right properties
(a material with `SCINTILLATIONYIELD` scintillates; one with `RINDEX` radiates
Cerenkov for fast charged particles).

---

## 6. Recording data: Sensitive Detectors, Hits, and the stepping action

A volume becomes "sensitive" when you attach a `G4VSensitiveDetector`. GEANT4
then calls its `ProcessHits(step)` for every step inside that volume, and you
decide what to store. Stored quantities go into **Hit** objects collected in a
**hits collection** for the event.

`RadicalCaloSD` (energy in LYSO / Pb-glass / counters):
```cpp
G4bool RadicalCaloSD::ProcessHits(G4Step* step, ...) {
  G4double edep = step->GetTotalEnergyDeposit();   // energy left in this step
  if (edep <= 0.) return false;
  // identify the cell from copy numbers in the touchable history:
  auto* t = step->GetPreStepPoint()->GetTouchable();
  G4int layer  = t->GetCopyNumber(1);   // the Tyvek-wrap copy number
  G4int module = t->GetCopyNumber(2);   // the module copy number
  // accumulate edep into the hit for cell (module,layer)
}
```

The **touchable history** is how you know *which* copy you're in: it's the chain
of mother volumes above the current point, each with its copy number. That's why
we set meaningful copy numbers when placing volumes.

**Optical photon detection is done in the stepping action, not an SD.** Detecting
a photon at a `dielectric_metal` photocathode has subtle SD-triggering behavior,
so `RadicalSteppingAction::UserSteppingAction()` watches every optical-photon
step and, when the optical **boundary process** reports `Detection`, records the
hit (which SiPM/MCP, time, channel). It also kills photons that bounce too many
times (a loop-breaker — see §9).

---

## 7. User actions: the per-run / per-event hooks

- **`RadicalRunAction`** — `BeginOfRunAction` opens the ROOT output file and (in
  its constructor) declares the histograms and ntuples; `EndOfRunAction` writes
  and closes them. In multithreaded mode each worker has its own; results are
  merged automatically.
- **`RadicalEventAction`** — `BeginOfEventAction` clears per-event containers;
  `EndOfEventAction` pulls the hits collections, computes summary quantities
  (total LYSO energy, detected photoelectrons per channel, the leading-edge
  times, the shower centroid) and fills the ntuples.
- **`RadicalSteppingAction`** — sees every step; here, optical-photon detection
  and the loop-breaker.
- **`RadicalStackingAction`** — every new track is "stacked" before tracking;
  we classify optical photons as *waiting* (track them after the charged shower,
  saving memory) and optionally kill them past a per-event cap.

`RadicalActionInitialization` is where these get attached. Note `Build()` (called
per worker thread) registers all of them, while `BuildForMaster()` registers only
the run action (the master thread just merges output).

---

## 8. Talking to the simulation without recompiling: messengers, UI, macros

GEANT4 has a built-in command system. Any command like `/run/beamOn 100` or
`/vis/drawVolume` is interpreted at runtime. You add your *own* commands with a
`G4UImessenger`. `RadicalDetectorMessenger` defines all the `/radical/...`
commands (geometry presets, optical yield, MCP resolution, ...). When the user
types one, `SetNewValue()` fires and updates the configuration.

A **macro** (`.mac`) is just a text file of these commands. This is how you run
parameter scans without touching C++:

```
/radical/geo/preset enhanced     # change the detector
/radical/beam/energyGeV 100      # change the beam
/run/beamOn 200                  # simulate 200 events
```

Order matters: anything that affects the geometry or physics tables must come
*before* `/run/initialize`; beam and run commands come after.

---

## 9. Optical photons — the special challenge

Optical photons are real tracked particles, and a high-energy shower in a bright
scintillator makes a *staggering* number of them (LYSO ≈ 30 000 photons/MeV →
up to ~10⁸–10⁹ per event at 150 GeV). Naively tracking them is infeasible. Three
controls handle this (all configurable):

- **Yield scaling** (`/radical/optical/yieldScale`) — multiply the scintillation
  yield by a small factor for tractable runs; detected p.e. scale ~linearly so
  you can correct offline.
- **Photon cap** (`/radical/optical/maxPhotons`) — a hard per-event ceiling
  (stacking action). *Lesson:* a fixed cap biases energy-dependent studies
  (it keeps a different fraction at each energy), so for the timing scan we use
  no cap + a low yield instead.
- **Loop-breaker** (stepping action) — kill optical photons after ~1000 steps or
  120 ns. Without it, photons doing total internal reflection off the 95%-
  reflective Tyvek bounce essentially forever and an event never finishes.

Optical surfaces (`G4OpticalSurface` + `G4LogicalSkinSurface`/`BorderSurface`)
define what happens at boundaries: the Tyvek wrap is a 95% diffuse reflector; the
SiPM/MCP photocathodes are detection surfaces with a wavelength-dependent
`EFFICIENCY` (the quantum/photon-detection efficiency).

---

## 10. Output and analysis

We use GEANT4's analysis manager (`G4AnalysisManager`) to write **ROOT** ntuples
and histograms (`RadicalRunAction`/`RadicalEventAction`):
- `event` — one row per beam particle (energies, p.e., times, centroid)
- `tile` — one row per LYSO tile hit (for shower profiles, Fig. 7)
- `channel` — one row per SiPM channel

Analysis (`analysis/analyze.C`) reads these with ROOT to make the physics plots.
A `TTree::Draw("y:x","cut")` is the quickest way to plot a column; see
`analysis/myplots.C` and `docs/THINGS_TO_TRY.md`.

---

## 11. Building and running

**Build (CMake).** A GEANT4 app is a normal CMake project that finds GEANT4 and
links it (`CMakeLists.txt`):
```cmake
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})
add_executable(radical radical.cc ${sources})
target_link_libraries(radical ${Geant4_LIBRARIES})
```
You must `source .../geant4.sh` first so CMake can find GEANT4 and the physics
data sets. Then `cmake .. && cmake --build .`. (`./run.sh build` wraps this.)

**Run.**
- *Interactive:* `./radical` (no argument) opens the Qt viewer and an interactive
  command prompt — good for seeing geometry and single events.
- *Batch:* `./radical somefile.mac` runs headless — good for statistics.
- `./run.sh` wraps both (`view`, `run`, `scan`, `analyze`, `plot`).

---

## 12. Real problems we solved (the most instructive part)

Building this surfaced the classic GEANT4 issues — worth knowing:

1. **Volume overlaps.** Placing two solids that intersect gives wrong tracking.
   Turn on overlap checking (`/radical/geo/checkOverlaps true`); we found the
   module *envelope* (sized for the long fibers) was poking into the MCP and
   Pb-glass and fixed the placements.
2. **Optical-photon explosion / infinite TIR loops** — solved with yield scaling
   + the step/time loop-breaker (§9).
3. **WLS energy-conservation crash** — the wavelength shifter tried to emit a
   photon *bluer* than the one it absorbed; fixed by keeping the emission
   spectrum entirely below the absorption band (Stokes shift).
4. **A timing-resolution "bug" that was really a reference problem.** The first
   number (76 ps) looked far off; *decomposing* it (module vs reference vs
   depth term) showed the detector was fine (~20 ps) and our MCP reference model
   was the culprit. Lesson: when a result looks wrong, break it into independent
   contributions before "fixing" the detector.

---

## Where to look next
- GEANT4 Book for Application Developers (official docs).
- The `examples/basic/B1`–`B5` and `examples/extended/optical` that ship with
  GEANT4 (`$G4_INSTALL/share/Geant4/examples`) — minimal versions of everything
  above.
- This repo: start at `radical.cc`, then `RadicalDetectorConstruction.cc`.
