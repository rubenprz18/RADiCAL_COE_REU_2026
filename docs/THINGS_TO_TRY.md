# Things to try in the RADiCAL GEANT4 simulation

A menu of experiments for getting comfortable with the simulation. Everything
here is verified to work in this build.

**Where to type these:** start the viewer with `./run.sh view`, then type commands
one at a time in the **Session:** box at the bottom of the Qt window (or the
terminal). You can also put a list of them in a `.mac` file and run
`./radical myfile.mac`. Lines starting with `#` are comments.

After most changes, `/run/beamOn 1` fires one particle so you can see the effect.

---

## A. Shoot different particles (the physics changes a lot)
```
/radical/beam/energyGeV 20
/radical/beam/particle e-      # electron  -> classic EM shower
/run/beamOn 1
/radical/beam/particle gamma   # photon    -> invisible until it converts, then showers
/run/beamOn 1
/radical/beam/particle mu-     # muon      -> a straight "minimum-ionizing" track, punches through
/run/beamOn 1
/radical/beam/particle pi-     # pion      -> messy hadronic interactions
/run/beamOn 1
/radical/beam/particle proton
/run/beamOn 1
/radical/beam/particle e+      # positron
/run/beamOn 1
```
Compare a `mu-` (one clean line straight through) with an `e-` (a fan of blue/red/green) — that contrast is the whole idea of a calorimeter.

## B. Change the energy (watch the shower get deeper)
```
/radical/beam/particle e-
/radical/beam/energyGeV 2     ; /run/beamOn 1     # short, shallow shower
/radical/beam/energyGeV 150   ; /run/beamOn 1     # long, deep shower
```

## C. Aim and shape the beam
```
/radical/beam/meanX 4 mm        # move the beam toward a corner capillary
/radical/beam/meanY 0 mm
/radical/beam/sigmaX 5 mm       # a wide, fuzzy beam spot
/radical/beam/sigmaY 5 mm
/radical/beam/divergence 2      # angular spread in milliradians
/radical/beam/sigmaEfrac 0.02   # 2% energy spread
/run/beamOn 20                  # fire 20 and let them accumulate on screen
```

## D. Morph the detector geometry, then redraw
Change parameters, then `/radical/geo/update` to rebuild and `/vis/drawVolume` to redraw.
```
# the enhanced 18 mm module with 9 capillaries (Fig. 28)
/radical/geo/preset enhanced
/radical/geo/layout grid9
/radical/geo/update
/vis/drawVolume

# hexagonal crystals
/radical/geo/tileShape hexagon
/radical/geo/hexR 9 mm
/radical/geo/update
/vis/drawVolume

# a 3x3 hexagonally-packed array of modules
/radical/geo/arrayNx 3
/radical/geo/arrayNy 3
/radical/geo/arrayTiling hex
/radical/geo/update
/vis/drawVolume

# declutter: turn off the Pb-glass and MCP to see just the module
/radical/geo/pbglass false
/radical/geo/mcp false
/radical/geo/update
/vis/drawVolume

# back to the standard prototype
/radical/geo/preset current
/radical/geo/pbglass true
/radical/geo/mcp true
/radical/geo/update
/vis/drawVolume
```

## E. See the light (optical photons)
By default the green scintillation/WLS/Cerenkov photons are hidden so the shower
is readable. Turn them on to watch light travel to the sensors:
```
/vis/filtering/trajectories/photons/active false
/radical/beam/energyGeV 10
/run/beamOn 1
# hide them again:
/vis/filtering/trajectories/photons/active true
```
(For a brighter event, set `/radical/optical/yieldScale 0.005` **before** `/run/initialize`.)

## F. Viewing tricks
```
/vis/viewer/set/style wireframe        # see-through wireframe
/vis/viewer/set/style surface          # solid again
/vis/viewer/set/viewpointThetaPhi 90 0 # look straight down the beam axis
/vis/viewer/set/viewpointThetaPhi 70 20

# slice the detector open with a cutting plane (great for seeing the layers)
/vis/viewer/set/sectionPlane on 0 0 0 mm 1 0 0
/vis/viewer/set/sectionPlane off

# or cut a quarter away
/vis/viewer/set/cutawayMode union
/vis/viewer/addCutawayPlane 0 0 0 m 1 0 0
/vis/viewer/clearCutawayPlanes

# hide or recolour individual parts
/vis/geometry/set/visibility Tungsten 0 false        # hide the absorber
/vis/geometry/set/visibility Tungsten 0 true
/vis/geometry/set/colour LYSO 0 0 1 0 0.5            # make LYSO green & half-transparent
```

## G. Statistics & reproducibility
```
/run/beamOn 1000                 # lots of events (no graphics overhead in batch)
/random/setSeeds 12345 67890     # fix the random seed -> identical results every time
/run/printProgress 100           # print a line every 100 events
```

## H. Inspect the detector in text (no graphics)
```
/material/g4/printMaterial LYSO_Ce   # density, radiation length, composition
/material/g4/printMaterial G4_W
/run/dumpCouples                     # every material + its production cuts
/units/list                          # all the units you can type (mm, cm, GeV, ns, ...)
/control/manual /radical             # list ALL of this project's custom commands
help                                 # browse the full command tree (interactive)
```

## I. Tweak the physics (advanced)
```
/run/setCut 0.1 mm     # production cut: smaller = more detailed secondaries, slower
/run/setCut 1 mm       # back to default-ish
```

---

## Mini-experiments to actually *measure* something
These produce data files you can plot with `./run.sh plot` or `event->Draw(...)`:

1. **Muon vs electron in the calorimeter.** Run 200 `mu-` and 200 `e-` at 50 GeV,
   compare `eLYSO` — the muon deposits a small, narrow amount (a MIP), the
   electron deposits a broad shower.
2. **Energy resolution.** `./run.sh run 100 500`, then plot `eLYSO` and fit a
   Gaussian — its width/mean is the resolution at 100 GeV.
3. **Position scan.** Fire with `/radical/beam/meanX` at 0, 3.5, 7 mm and watch
   which SiPM channel (`channel->Draw("npe","channel==N")`) lights up most.
4. **Shower depth vs energy.** The `tile` longitudinal profile (see `myplots.C`)
   peaks deeper as energy rises — that's Fig. 7.

When you want a variable that isn't recorded yet, it can be added to the
simulation's ROOT output — just ask.
