# RADiCAL simulation — project notes & recipe

A self-contained reference for (re)building and extending this GEANT4
simulation of the RADiCAL calorimeter. Pairs with [GEANT4_GUIDE.md](GEANT4_GUIDE.md)
(how GEANT4 works) and [THINGS_TO_TRY.md](THINGS_TO_TRY.md) (experiments to run).

---

Reference for (re)building the RADiCAL calorimeter simulation from Perez-Lara
et al., **Nucl. Instr. Meth. A 1068 (2024) 169737**. Build/runtime setup is in
the main [README](../README.md) and `scripts/install_geant4.sh`.

## Design decisions (the scope this simulation implements)
- **Full optical-photon tracking** (NOT energy-only): scintillation in LYSO, WLS
  in DSB1/LuAG filaments, Cerenkov in quartz/Pb-glass, photodetection at cathodes.
- **0.1 mm Tyvek wraps ALL 6 faces of every LYSO crystal** (user spec; the paper
  used 0.008"≈0.2 mm inter-layer sheets; this build uses the 6-face wrap).
- Fully parametrized geometry, all macro-switchable: capillary layout
  (quincunx5 ↔ grid9 Fig.28), tile shape (square/rectangle/hexagon), module size
  (current 14 mm / enhanced 18 mm), N×N arrays (square or hex packing).
- Surrounding detectors ALL at full optical detail: MCP internals, Pb-glass
  Cerenkov, counter scintillation, realistic beam + upstream beam chamber.
- Outputs: per-tile/layer energy, detected p.e., timing resolution, ROOT
  ntuples+histograms. Build covers BOTH single module and N×N array, switchable.

## Paper geometry/material specs (current prototype, Figs. 1-2)
- Module 14×14×135 mm³ Shashlik/Kebap; 29 LYSO(Ce) @1.5 mm interleaved with 28 W
  @2.5 mm; ~25 X0; sampling X0=5.4 mm, Molière R_M=13.7 mm. (LYSO material RadL is
  ~1.22 cm; the 5.4 mm is the sampling structure.) LYSO ρ=7.1 g/cm³.
- 5 capillary holes: 4 corners φ1.3 mm at (±3.5,±3.5) mm + 1 centre φ0.9 mm.
- Quartz capillaries OD 1150 µm / core 950 µm, 183 mm long. DSB1 WLS filament
  900 µm × 15 mm at shower max (T-type, timing); E-type = full-length LuAG:Ce.
- DSB1 absorbs ~425 nm, emits ~495 nm, τ=3.5 ns. LYSO emits ~420 nm, τ~40 ns.
- SiPM Hamamatsu HDR2, 4 per card, both ends → **8 channels** (4 corner capillaries
  × up/down). MCP = Hamamatsu R3809U-50, σ_t~10-20 ps, just upstream of module.
- Beam line (Fig.11c) order: CERN H2 e⁻ 25-150 GeV (25-GeV steps) → A1 (1×1 cm²)
  & A2 (2×2 cm²) trigger counters → beam chamber → MCP → module → 2×2 Pb-glass
  array (4×4×40 cm³, backing/leakage).
- Enhanced design (Fig.28): 18×18 mm², 9 capillaries in 3×3 grid (5 T + 4 E, two
  T/E-assignment variants), E-type LuAG full length; future 3×3 module array; beam
  to 200 GeV.
- Paper timing result: σ_t = a/√E ⊕ b with **a=256 ps√GeV, b=17.5 ps**; **27 ps at
  150 GeV**. Fig.7 = EM-shower energy fraction per LYSO layer (shower max migrates
  layer ~8 at 25 GeV → ~11 at 150 GeV).

## Architecture (files in the repo)
- `radical.cc` main: G4RunManagerFactory (MT) + FTFP_BERT + G4OpticalPhysics +
  ActionInitialization + G4VisExecutive("quiet").
- `RadicalGeometryConfig` (struct, all switchable params + capillary layouts;
  `ModuleHalfZ()`), `RadicalMaterials` (singleton; NIST + custom materials +
  optical property tables; `fBuilt` guard so geometry can rebuild without
  redefining materials; static `fYieldScale`), `RadicalDetectorConstruction`
  (Solid→Logical→Physical; boolean-subtracted capillary holes; Tyvek skin +
  photocathode optical surfaces), `RadicalDetectorMessenger` (/radical/... cmds).
- `RadicalPrimaryGeneratorAction` (realistic e⁻ beam: spot, divergence, momentum
  spread; G4GenericMessenger /radical/beam/...).
- `RadicalCaloSD`/`RadicalCaloHit` (energy in LYSO/Pb-glass/counters);
  `RadicalSteppingAction` (optical-photon detection via boundary Detection status
  + the loop-breaker); `RadicalRunAction`/`RadicalEventAction` (ROOT ntuples
  event/tile/channel + histograms); `RadicalStackingAction` (optical photon cap +
  track-after-charged); `RadicalActionInitialization`.
- `run.sh` launcher: view / run [E] [N] / scan / analyze [dir] / plot [file] /
  build / help. `analysis/analyze.C` (Fig.7 + timing fit), `analysis/myplots.C`
  (TTree::Draw template). `macros/` (init_vis, vis, single_module,
  enhanced_module, array3x3, hex_module, energy_scan, timing_scan,
  timing_scan_hi, run_beam, timing). `docs/THINGS_TO_TRY.md`, `docs/GEANT4_GUIDE.md`.
  `scripts/install_geant4.sh`.

## Critical implementation gotchas (do NOT relearn the hard way)
- **Optical loop-breaker is MANDATORY**: in SteppingAction kill optical photons
  if step# > 1000 or globalTime > 120 ns. Without it, photons do endless total
  internal reflection off the 95% Tyvek and an event NEVER finishes (looks hung).
- **Optical photon volume**: 30000 ph/MeV → up to 1e8-1e9/event at 150 GeV. Use
  `/radical/optical/yieldScale` (set BEFORE /run/initialize — materials build
  once). For the TIMING scan use yieldScale 0.0003 + NO cap so detected p.e. scale
  with E (preserves the 1/√E term). The maxPhotons cap is energy-DEPENDENT bias —
  fine for single-energy/geometry runs, bad for cross-energy timing.
- **WLS Stokes shift**: emission spectrum must be entirely LOWER photon energy
  than the absorption band, else G4OpWLS fatal "WSL01". DSB1/LuAG WLSABSLENGTH set
  huge (1e5 m) except in the blue absorption band.
- **Beam-line overlaps**: the module ENVELOPE is sized for the 183 mm capillaries
  (~±96 mm via `ModuleHalfZ()`), LONGER than the stack — place MCP and Pb-glass
  relative to `ModuleHalfZ()`, not the stack. Overlap check is opt-in
  (`cfg.checkOverlaps` / `/radical/geo/checkOverlaps true`, default off). Log
  phrase is "Overlap is detected" (not "is overlapping").
- **Optical detection in SteppingAction**, not an SD (boundary Detection status is
  more reliable for dielectric_metal cathodes). Energy via CaloSD.
- Copy-number scheme: SiPM copyNo = capIdx (down) / capIdx+50 (up); module=touch
  depth 1 for SiPM; LYSO layer=depth 1, module=depth 2.
- **Quiet runs**: macros set /run/verbose 0 + /process/{em,had}/verbose 0 and main
  uses G4VisExecutive("quiet") → ~38 lines instead of ~650 (otherwise the full
  physics-table dump is overwhelming).
- Undetected channel/MCP times are stored as a **1e9 ns sentinel** → analysis MUST
  cut 0 < t < 500 ns or σ explodes to ~1e11 ps.

## Timing replication — the key story (the "76 ps → 27 ps" arc)
- **Headline metric = INTRINSIC module resolution** = width of
  t_module=(t_down+t_up)/2. The down+up average cancels shower-depth jitter; its
  mean is just the (constant) time-of-flight, so the WIDTH is the resolution. This
  is what the paper quotes after removing its MCP reference.
- The early "72-86 ps" was a BUG, not the detector: σ(t_module-t_MCP) was
  dominated by a broken **MCP reference** (σ~94 ps) from total internal reflection
  in the vacuum gap between the quartz radiator and photocathode. The module
  itself was always ~20 ps. **ALWAYS DECOMPOSE** a resolution — σ(t_module),
  σ(t_MCP), σ((t_down-t_up)/2) — BEFORE "fixing the detector".
- Fix 1: photocathode deposited DIRECTLY on the radiator back face (no gap) +
  5 mm radiator + kMCPThreshold=3 → σ_MCP dropped to ~0.6 ps (too ideal).
- Fix 2 (realistic readout): added `/radical/optical/mcpResolution` (default
  18 ps = R3809U-50 spec) and `/radical/optical/sipmJitter` (per-channel), applied
  to the leading-edge times in EventAction. Final scan (timing_scan_hi.mac →
  results/scan_realistic): intrinsic module a≈210, b≈9 ps (paper 256, 17.5);
  as-measured σ(t_module-t_MCP) lands on the paper curve at high E
  (125→27 ps, 150→30 ps; paper 27).
- Analysis estimator: `analyze.C` uses a trimmed-RMS Gaussian-CORE width
  (±2.5·MAD-sigma) to fit the PEAK like the paper — excludes the non-Gaussian
  low-energy late tails (channels with few p.e.); the full-range RMS blew the
  25 GeV point up to ~180 ps. ROOT gotcha: `h->Fit("gaus","R")` fits the
  predefined gaussian's default [0,1] range → empty fit for data at ~2.5 ns; use
  `"Q"` not `"QR"`.
- Leading-edge timing estimator in EventAction: k-th p.e. threshold time
  (kSiPMThreshold=3, kMCPThreshold=3); average over the per-end channels.

## Performance / workflow
- ~9 s/event at 150 GeV full optical (yieldScale 0.0003, no cap). 60-event 150 GeV
  ≈ 9 min. Graded hi-stats scan (timing_scan_hi.mac, ~70-120 events/energy)
  ≈ 45-50 min on 4 threads (8 cores). Reproduce: `./run.sh scan` (or run
  timing_scan_hi.mac), then `./run.sh analyze <dir>` → analysis/figures/.
- Fig.7 is pure calorimetry (independent of optical) → already matches the paper.

## Possible next steps
- Energy resolution σ_E/E vs E (the paper's other goal) — needs calorimeter-sum
  scoring + fit. Realistic SiPM dark counts / electronic noise. Calibrate light
  yield to the paper's Fig.8 p.e./MeV operating point.
