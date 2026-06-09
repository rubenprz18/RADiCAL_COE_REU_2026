#!/usr/bin/env bash
#
# run.sh -- one-stop launcher for the RADiCAL simulation.
# It sources GEANT4 for you, builds if needed, and runs common tasks.
#
#   ./run.sh view              open the interactive 3D viewer (see the detector)
#   ./run.sh run [E] [N]       run N electrons at E GeV  (default 150 GeV, 60 events)
#   ./run.sh scan              full 25-150 GeV energy scan (for the timing fit)
#   ./run.sh analyze [DIR]     make Fig.7 + timing-fit plots from a scan
#   ./run.sh plot [FILE]       make starter graphs from one data file
#   ./run.sh build             (re)compile the simulation
#   ./run.sh help              show this help
#
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$HERE/build"
EXE="$BUILD/radical"
G4ENV="${GEANT4_ENV:-$HOME/geant4/v11.4.1/bin/geant4.sh}"

# --- source GEANT4 (sets PATH, data-set variables, etc.) -----------------
if [ ! -f "$G4ENV" ]; then
  echo "ERROR: GEANT4 environment not found at:"
  echo "   $G4ENV"
  echo "Set GEANT4_ENV=/path/to/geant4.sh and retry."
  exit 1
fi
set +u; source "$G4ENV"; set -u 2>/dev/null || true

build() {
  echo ">> building ..."
  mkdir -p "$BUILD"
  ( cd "$BUILD" && cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" >/dev/null && cmake --build . --parallel )
}
ensure_built() { [ -x "$EXE" ] || build; }

cmd="${1:-help}"
case "$cmd" in
  build)
    build ;;

  view)
    ensure_built
    echo ">> opening interactive viewer (close the window to quit) ..."
    cd "$HERE"            # so 'macros/init_vis.mac' resolves
    "$EXE" ;;

  run)
    ensure_built
    E="${2:-150}"; N="${3:-60}"
    OUT="$HERE/results/run_${E}GeV"; rm -rf "$OUT"; mkdir -p "$OUT"
    cat > "$OUT/run.mac" <<EOF
/run/numberOfThreads 4
/run/verbose 0
/control/verbose 0
/process/em/verbose 0
/process/had/verbose 0
/radical/geo/preset current
/radical/geo/beamline true
/radical/optical/yieldScale 0.0003
/radical/optical/maxPhotons 0
/run/initialize
/radical/beam/particle e-
/radical/beam/energyGeV $E
/radical/beam/sigmaEfrac 0.005
/radical/beam/sigmaX 2 mm
/radical/beam/sigmaY 2 mm
/run/printProgress 10
/run/beamOn $N
EOF
    echo ">> running $N electrons at $E GeV (full optical) ..."
    echo "   This takes a few minutes; progress prints every 10 events."
    echo "   (full log: $OUT/run.log   |   press Ctrl+C to stop)"
    echo ""
    ( cd "$OUT" && "$EXE" run.mac 2>&1 | tee run.log )
    echo ""
    echo ">> done. Data: $OUT/radical_run0.root"
    echo "   Make graphs with:  ./run.sh plot $OUT/radical_run0.root" ;;

  scan)
    ensure_built
    OUT="$HERE/results/scan"; rm -rf "$OUT"; mkdir -p "$OUT"
    echo ">> running 25-150 GeV scan (this takes a while) ..."
    ( cd "$OUT" && "$EXE" "$HERE/macros/timing_scan.mac" )
    echo ">> done. 6 files in $OUT . Now run:  ./run.sh analyze" ;;

  analyze)
    DIR="${2:-$HERE/results/scan}"
    [ -d "$DIR" ] && ls "$DIR"/radical_run*.root >/dev/null 2>&1 || DIR="$BUILD/scan_out"
    mkdir -p "$HERE/analysis/figures"
    echo ">> analyzing $DIR ..."
    ( cd "$HERE/analysis/figures" && root -l -b -q "$HERE/analysis/analyze.C(\"$DIR\")" )
    echo ">> plots written to $HERE/analysis/figures/" ;;

  plot)
    FILE="${2:-$HERE/results/run_150GeV/radical_run0.root}"
    [ -f "$FILE" ] || FILE="$BUILD/run150/radical_run0.root"
    [ -f "$FILE" ] || FILE="$BUILD/scan_out/radical_run5.root"
    if [ ! -f "$FILE" ]; then
      echo "No data file found. Make some first, e.g.:  ./run.sh run 150"; exit 1
    fi
    mkdir -p "$HERE/analysis/figures"
    echo ">> plotting $FILE ..."
    ( cd "$HERE/analysis/figures" && root -l -b -q "$HERE/analysis/myplots.C(\"$FILE\")" )
    echo ">> wrote $HERE/analysis/figures/myplots.png" ;;

  help|*)
    sed -n '3,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//' ;;
esac
