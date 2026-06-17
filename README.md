# RADiCAL_COE_REU_2026

GEANT4 simulation and analysis of the **RADiCAL** ultra-compact sampling
calorimeter, following the geometry and materials described in:

> C. Perez-Lara *et al.*, *"Study of time resolution measurements and prospects
> for energy resolution of an ultra-compact sampling calorimeter (RADiCAL)
> module at EM shower maximum over the energy range 25 GeV – 150 GeV"*,
> Nucl. Instr. Meth. A **1068** (2024) 169737.
> [doi:10.1016/j.nima.2024.169737](https://doi.org/10.1016/j.nima.2024.169737)

This is a **full optical-photon** simulation of the CERN H2 beam-test setup
(Fig. 11c of the paper): the electron beam, the trigger counters, the beam
position chamber, the MCP timing reference, the RADiCAL module(s), and the
lead-glass backing calorimeter — with scintillation, wavelength shifting,
Cerenkov radiation and photodetection all tracked.

---

## 0. Quick start (`run.sh`)

`run.sh` sources GEANT4, builds if needed, and runs the common tasks:

```bash
cd ~/Documents/Research/RADiCAL/RADiCAL_COE_REU_2026
./run.sh view          # open the interactive 3D viewer (see the detector)
./run.sh run 150       # shoot 60 electrons at 150 GeV -> results/run_150GeV/*.root
./run.sh scan          # full 25-150 GeV scan (for the timing fit)
./run.sh analyze       # make Fig.7 + timing plots in analysis/figures/
./run.sh help          # list commands
```

In the viewer window, type `/run/beamOn 1` to fire one electron and watch the
shower. See §4 for navigation and §6a for the analysis.

## 1. What is modeled

### RADiCAL module (current prototype, 14 × 14 × 135 mm³, Figs. 1–2)
| Component | Implementation |
|---|---|
| Sampling stack | 29 LYSO:Ce plates (1.5 mm) interleaved with 28 W plates (2.5 mm), ≈ 25 X₀ |
| **Tyvek wrap** | **0.1 mm Tyvek surrounding all six faces of every LYSO crystal** (diffuse reflector, 95 %) |
| Capillaries | Fused-silica tubes (OD 1150 µm / core 950 µm), 5-hole quincunx — 4 corners φ1.3 mm at (±3.5, ±3.5) mm + φ0.9 mm centre |
| WLS | DSB1 filament (900 µm × 15 mm) at shower max in T-type; LuAG:Ce full-length in E-type |
| SiPM readout | 8 channels = 4 corner capillaries × upstream/downstream ends (Hamamatsu HDR2-like PDE) |

### Beam line (Fig. 11c)
- **Beam** — CERN H2-style e⁻ gun with finite spot, divergence and momentum spread (25–150 GeV).
- **A1/A2 trigger counters** — 1×1 cm² and 2×2 cm² plastic scintillators with optical readout.
- **Beam chamber** — thin upstream (x, y) reference plane.
- **MCP** — Hamamatsu R3809U-50-style quartz window + bialkali photocathode; Cerenkov from the crossing beam gives the ~15 ps timing reference.
- **Pb-glass calorimeter** — 2×2 lead-glass array (4×4×40 cm³), Cerenkov + leakage-energy scoring.

### Alternate geometries (Fig. 28 and beyond) — all macro-switchable
- **Capillary layouts**: 5-hole quincunx (Fig. 2) ↔ 9-hole 3×3 grid (Fig. 28, 5 T-type + 4 E-type).
- **Tile cross-section**: `square` (default) / `rectangle` / `hexagon`.
- **Module size**: `current` (14 mm) / `enhanced` (18 mm) presets.
- **Arrays**: N×N modules, `square` grid or `hex` close-packed.

---

## 2. Prerequisites & GEANT4 install

Requires **GEANT4 11.x** (multithreaded, GDML, Qt), **CMake ≥ 3.16**, and a C++17
compiler. ROOT is optional (for reading the output).

Homebrew no longer ships a `geant4` formula, so a helper builds it from source
using the Homebrew toolchain:

```bash
./scripts/install_geant4.sh          # builds GEANT4 v11.4.1 -> ~/geant4/v11.4.1
source ~/geant4/v11.4.1/bin/geant4.sh # put this in your shell profile
```

(If you already have a GEANT4 install, just `source` its `geant4.sh` instead.)

---

## 3. Build

```bash
source ~/geant4/v11.4.1/bin/geant4.sh
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build . --parallel
```

This produces the `radical` executable and copies the run macros into `build/macros/`.

---

## 4. Run

**Interactive (Qt visualization):**
```bash
./radical                      # opens a viewer; loads macros/init_vis.mac
```

**Batch:**
```bash
./radical macros/single_module.mac   # current module, full beam line, 50 GeV
./radical macros/enhanced_module.mac # 18 mm, 9-capillary (Fig. 28)
./radical macros/hex_module.mac      # hexagonal tiles, hex-packed cluster
./radical macros/array3x3.mac        # 3×3 module array
./radical macros/energy_scan.mac     # 25–150 GeV scan (paper) -> one ROOT file/energy
```

Each `/run/beamOn` writes `radical_run<N>.root`.

### Key UI commands
```
# geometry (apply, then /radical/geo/update to rebuild)
/radical/geo/preset       current | enhanced
/radical/geo/tileShape    square | rectangle | hexagon
/radical/geo/layout       quincunx5 | grid9
/radical/geo/arrayNx 3
/radical/geo/arrayNy 3
/radical/geo/arrayTiling  square | hex
/radical/geo/tileSizeX 16 mm
/radical/geo/hexR 10 mm
/radical/geo/tyvek 0.1 mm
/radical/geo/mcp|pbglass|counters|beamline  true|false
/radical/geo/update

# beam
/radical/beam/particle e-
/radical/beam/energyGeV 100
/radical/beam/sigmaEfrac 0.01
/radical/beam/sigmaX 2 mm
/radical/beam/meanX 0 mm
/radical/beam/divergence 0.2     # mrad

# optical runtime (see caveat below)
/radical/optical/yieldScale 0.01
/radical/optical/maxPhotons 500000
```

---

## 5. ⚠️ Optical photon yield-scale (important)

LYSO produces ~30 000 photons/MeV. A 150 GeV shower depositing several GeV would
create **10⁸–10⁹ optical photons per event** — far too many to track. Two knobs
keep runs tractable:

- `/radical/optical/yieldScale s` — multiplies scintillation yield by `s`
  (**set before `/run/initialize`**). The detected p.e. then scale ~linearly,
  so multiply `npe` by `1/s` offline to recover the physical light level.
- `/radical/optical/maxPhotons n` — hard per-event cap on optical photons.

The provided macros use `yieldScale` 0.005–0.02. For absolute light-yield /
timing studies, raise the scale and lower the beam energy or event count.

---

## 6. Output (ROOT, via the G4 analysis manager)

**Ntuples**
- `event` — per-event summary: `beamE, beamX/Y, eLYSO, ePbGlass, eCounters,
  npeSiPM, npeMCP, tMCP, tModuleDown/Up, xCentroid, yCentroid`.
- `tile` — per-LYSO-tile energy: `module, layer, edep, t, x, y` (shower profiles, Figs. 7 & 9).
- `channel` — per-SiPM channel: `module, channel (0–3 down, +50 up), npe, tFirst`.

**Histograms** — `hLong` (energy per layer), `hSiPMnpe`, `hTiming`
(t_module − t_MCP), `hELyso`, `hEPb`, `hCentroid` (shower centroid at shower max).

---

## 6a. Reproducing Fig. 7 and the timing fit

```bash
# 1. run the 25-150 GeV scan (one ROOT file per energy)
cd build && ./radical macros/timing_scan.mac      # -> scan_out/radical_run0..5.root
# 2. analyze
cd .. && root -l -b -q 'analysis/analyze.C("build/scan_out")'
```

Produces (in `analysis/figures/`):
- **`fig7_longitudinal.png`** — fraction of EM-shower energy per LYSO tile vs
  layer, overlaid for the six energies. Reproduces Fig. 7: shower max sits at
  layer ~8 for 25 GeV and migrates to layer ~11 by 150 GeV (logarithmic growth
  of shower depth with energy).
- **`timing_resolution.png`** — σ_t per energy from the width of
  `t_module − t_MCP`, fitted to **σ_t = a/√E ⊕ b**, with the paper's
  (a = 256 ps√GeV, b = 17.5 ps) curve overlaid.

**Result.** The analysis reports the *intrinsic module* timing resolution (the
quantity the paper quotes after removing its MCP reference) — the width of the
`t_module` distribution, where `t_module = (t_down + t_up)/2` averages the
upstream/downstream ends so the shower-depth jitter cancels (a Gaussian-core
width, like the paper's peak fit). With a realistic readout model
(`macros/timing_scan_hi.mac`), the higher-statistics scan reproduces
**σ_t = a/√E ⊕ b** with **a ≈ 210 ± 18 ps·√GeV, b ≈ 9 ps** (paper: 256, 17.5),
the module curve tracking just below the paper's. The *as-measured*
`σ(t_module − t_MCP)`, including a realistic ~18 ps MCP reference, lands on the
paper's curve at high energy (125 GeV → 27 ps, 150 GeV → 30 ps; paper 27 ps).

**Readout realism.** The MCP reference and SiPM electronics are configurable:
`/radical/optical/mcpResolution 18 ps` sets the reference single-shot resolution
(real R3809U-50 ≈ 15–20 ps; default 18 ps) and `/radical/optical/sipmJitter`
adds per-channel SiPM electronics jitter (default 0). Set `mcpResolution 0` to
recover the ideal-reference (intrinsic-only) case.

**A cautionary tale (the "76 ps" bug).** An early run reported σ ≈ 72–86 ps and
looked far off. Decomposing the budget showed the *module* was already ~20 ps —
the inflation came entirely from a broken MCP reference (σ ≈ 94 ps) caused by
total internal reflection in a vacuum gap between the quartz radiator and the
photocathode. Depositing the photocathode directly on the radiator (as in the
real Hamamatsu R3809U-50) dropped the reference jitter to <1 ps, so the directly
measured `σ(t_module − t_MCP)` collapsed onto the intrinsic ~20 ps. Lesson: when
a resolution looks wrong, decompose it into its independent contributions before
"fixing" the detector. (See `docs/` and `analysis/analyze.C`.)

### Energy resolution (σ_E/E vs E)

`analyze.C` also fits the **energy resolution** σ_E/E = a/√E ⊕ b/E ⊕ c from the
width of the per-event LYSO energy (`eLYSO`). Energy deposition is independent of
optical photons, so `macros/eres_scan.mac` (and `eres_scan_array.mac`) run with
the scintillation yield ≈ 0 for fast, high-statistics scans.

- **Single module:** σ_E/E ≈ **12%/√E ⊕ 1.7%** — the stochastic term already
  approaches the FCC-hh goal (10%), but the constant term is inflated by shower
  leakage from a thin (25 X₀) module that is only ~1 Molière radius wide.
- **3×3 array** (`eres_scan_array.mac`): σ_E/E ≈ **8.8%/√E ⊕ 0.9%** — transverse
  containment improves **both** terms; the curve sits on the FCC-hh goal
  (10%/√E ⊕ 0.3/E ⊕ 0.7%) at high energy. This reproduces the paper's argument
  that an array is needed for the full EM energy resolution.

`analysis/eres_compare.C` overlays the two (→ `energy_resolution_compare.png`).

### Hexagonal cell (FCC-MIT-2024 proposed geometry)

The FCC-MIT-2024 presentation (J. Wetzel) proposes a **hexagonal module** with a
**7-capillary layout**: a centre capillary + a hexagonal ring of 6 alternating
T-type / E-type. Built with `/radical/geo/tileShape hexagon` +
`/radical/geo/layout hex7` (schematic: `analysis/figures/hex7_layout.png`; scan:
`macros/hex7_timing_scan.mac`; comparison: `analysis/hex_compare.C` →
`hex_vs_square.png`). Findings vs the square module:

- **~5× more detected light** — the central capillary sits at the shower core
  (highest light density), which the square's 4 corner capillaries miss.
- **Better timing**, especially at low/mid energy where photon statistics
  dominate (25 GeV: ~33 ps vs ~44 ps), reaching ~12–19 ps at high energy.
- Single-module *energy* resolution is slightly worse (~13.6% vs 11.8%
  stochastic) — a 14 mm flat-to-flat hexagon has ~13% less area than a 14 mm
  square (more transverse leakage) — but hexagons tile a plane with no gaps,
  the natural cell for a close-packed array.

Timing uses the fast T-type (DSB1) channels only; the slower LuAG E-type
channels carry energy/light. The capillary type is encoded in the SiPM channel
number (T-type < 100, E-type ≥ 100).

Dedicated hexagon graphs are in **`analysis/figures/hex/`**:
`timing_resolution.png` — σ_t = a/√E ⊕ b with **a ≈ 171 ps·√GeV, b ≈ 0**
(lower stochastic term than the square's ~210 and the paper's 256, from the ~5×
light; ~12–20 ps at high energy); and `energy_resolution.png` — σ_E/E ≈
**14%/√E ⊕ 1.7%**. (Timing from `hex7_timing`, energy from the clean `eres_hex7`
500-event scan.)

## 7. Source layout

```
radical.cc                     main(): run manager, FTFP_BERT + G4OpticalPhysics
include/, src/
  RadicalGeometryConfig        all switchable parameters + capillary layouts
  RadicalMaterials             LYSO, W, Tyvek, quartz, DSB1, LuAG, Pb-glass,
                               plastic scint, photocathodes + optical tables
  RadicalDetectorConstruction  parametrized geometry (tiles, holes, capillaries,
                               beam line, arrays) + Tyvek/photocathode surfaces
  RadicalDetectorMessenger     /radical/geo and /radical/optical commands
  RadicalPrimaryGeneratorAction realistic beam (+ /radical/beam commands)
  RadicalCaloSD / RadicalCaloHit   energy in LYSO / Pb-glass / counters
  RadicalSteppingAction        optical-photon detection at photocathodes
  RadicalStackingAction        optical-photon runtime control
  RadicalRunAction / EventAction   ROOT ntuples + histograms
macros/                        run + visualization macros
scripts/install_geant4.sh      source build of GEANT4
```

## 8. Known simplifications (good first refinement targets)

- Optical constants (indices, absorption/WLS lengths, PDE/QE curves) are
  representative literature values centralized in `RadicalMaterials.cc`.
- SiPM/MCP/Pb-glass readouts model the photocathode as a detection surface; no
  electronic pulse shaping, gain, dark counts or SiPM saturation.
- LYSO ²⁷⁶Lu intrinsic radioactivity and temperature effects are not included.
- The beam chamber is passive (records the generated beam position).
