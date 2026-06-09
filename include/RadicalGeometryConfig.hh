//
// RadicalGeometryConfig.hh
//
// Central, messenger-driven description of the RADiCAL geometry.  Every
// parametrized option requested for this project lives here so the detector
// construction, the messenger and the analysis all share one source of truth.
//
// Geometry / material values are taken from:
//   C. Perez-Lara et al., "Study of time resolution measurements and prospects
//   for energy resolution of an ultra-compact sampling calorimeter (RADiCAL)
//   module at EM shower maximum ...", Nucl. Instr. Meth. A 1068 (2024) 169737.
//
#ifndef RADICAL_GEOMETRY_CONFIG_HH
#define RADICAL_GEOMETRY_CONFIG_HH

#include "globals.hh"
#include "G4SystemOfUnits.hh"
#include <vector>

// ---------------------------------------------------------------------------
// Enumerations for the switchable geometry options
// ---------------------------------------------------------------------------

// Outer cross-sectional shape of every LYSO / W tile in a module.
enum class TileShape { Square, Rectangle, Hexagon };

// Capillary hole pattern in the tile cross section.
//   Quincunx5 : 4 corner holes + 1 centre hole              (Fig. 2, current)
//   Grid9     : 3x3 grid of 9 holes                         (Fig. 28, enhanced)
enum class CapillaryLayout { Quincunx5, Grid9 };

// How multiple modules are packed into an array.
enum class ArrayTiling { Square, Hex };

// Role of an individual capillary (drives which WLS filament fills it).
enum class CapType { TType, EType, Calibration };

// Convenience presets matching the two modules described in the paper.
enum class ModulePreset { Current14, Enhanced18 };

// ---------------------------------------------------------------------------
// One capillary position in the tile cross section (module-local x,y)
// ---------------------------------------------------------------------------
struct CapillarySpec {
  G4double x = 0.;          // transverse position (module frame)
  G4double y = 0.;
  G4double holeDiameter = 1.3 * CLHEP::mm;     // drilled hole diameter
  G4double outerDiameter = 1.15 * CLHEP::mm;   // quartz capillary OD
  G4double coreDiameter = 0.95 * CLHEP::mm;    // quartz capillary core ID
  G4double filamentDiameter = 0.90 * CLHEP::mm; // WLS filament diameter
  CapType type = CapType::TType;
};

// ---------------------------------------------------------------------------
// Full geometry configuration
// ---------------------------------------------------------------------------
class RadicalGeometryConfig {
 public:
  RadicalGeometryConfig() { ApplyPreset(ModulePreset::Current14); }

  // ---- Sampling stack (per module) ---------------------------------------
  G4int    nLysoPlates   = 29;
  G4double lysoThickness = 1.50 * CLHEP::mm;   // LYSO:Ce plate thickness
  G4int    nWPlates      = 28;
  G4double wThickness    = 2.50 * CLHEP::mm;   // tungsten absorber thickness

  // Tyvek wrap that surrounds each LYSO crystal (project requirement: 0.1 mm
  // on all six faces; the paper used 0.008" ~ 0.2 mm inter-layer sheets).
  G4double tyvekThickness = 0.10 * CLHEP::mm;

  // ---- Tile cross section -------------------------------------------------
  TileShape tileShape = TileShape::Square;
  G4double  tileSizeX = 14.0 * CLHEP::mm;      // full width  (square/rect)
  G4double  tileSizeY = 14.0 * CLHEP::mm;      // full height (square/rect)
  // For hexagonal tiles: circumradius (centre -> vertex). Flat-to-flat = R*sqrt(3).
  G4double  hexCircumradius = 8.08 * CLHEP::mm; // ~ matches 14 mm flat-to-flat

  // ---- Capillaries --------------------------------------------------------
  CapillaryLayout layout = CapillaryLayout::Quincunx5;
  G4double capillaryLength = 183.0 * CLHEP::mm;   // full capillary length
  // WLS filament geometry
  G4double tFilamentLength = 15.0 * CLHEP::mm;    // T-type filament (shower max)
  // Shower-max z of the filament centre, measured from the upstream face of
  // the stack.  Tuned in the paper to layers ~8-13; ~ 11 * (1.5+2.5) mm.
  G4double showerMaxZ = 44.0 * CLHEP::mm;
  std::vector<CapillarySpec> capillaries;         // filled by RebuildCapillaries()

  // ---- Photosensors -------------------------------------------------------
  G4double sipmThickness   = 0.6 * CLHEP::mm;     // active Si thickness
  G4double sipmWindow      = 0.3 * CLHEP::mm;     // protective window
  G4double readoutCardThk  = 1.6 * CLHEP::mm;     // FR4 front-end card
  G4bool   sipmBothEnds    = true;                // upstream + downstream

  // ---- Module array -------------------------------------------------------
  G4int       arrayNx = 1;                        // modules across
  G4int       arrayNy = 1;
  ArrayTiling arrayTiling = ArrayTiling::Square;
  G4double    moduleGap   = 0.2 * CLHEP::mm;      // gap between modules

  // ---- Beam-line elements (Fig. 11c) -------------------------------------
  G4bool includeBeamline   = true;
  G4bool includeCounters   = true;   // A1 (1x1 cm2), A2 (2x2 cm2) triggers
  G4bool includeMCP        = true;   // Hamamatsu R3809U-50 timing reference
  G4bool includePbGlass    = true;   // 2x2 lead-glass backing calorimeter
  G4bool includeBeamChamber= true;   // upstream (x,y) beam chamber

  // longitudinal placement of beam-line elements (z along beam, world frame)
  G4double counterA1_z   = -300.0 * CLHEP::mm;
  G4double counterA2_z   = -200.0 * CLHEP::mm;
  G4double beamChamber_z = -150.0 * CLHEP::mm;
  G4double mcp_z         =  -90.0 * CLHEP::mm;   // just upstream of module
  G4double module_z      =    0.0 * CLHEP::mm;   // module stack centre
  G4double pbGlassGap    =   30.0 * CLHEP::mm;   // gap module -> Pb glass front

  // ---- Derived helpers ----------------------------------------------------
  // One sampling cell = one Tyvek-wrapped LYSO + one W plate.
  G4double LayerPitch() const {
    return lysoThickness + 2. * tyvekThickness + wThickness;
  }
  // Total stack length (N LYSO wraps + (N-1) W plates between them).
  G4double StackLength() const {
    return nLysoPlates * (lysoThickness + 2. * tyvekThickness) +
           nWPlates * wThickness;
  }
  // Half-extent of the wrapped crystal cross section (square/rect bounding box).
  G4double WrapHalfX() const { return 0.5 * tileSizeX + tyvekThickness; }
  G4double WrapHalfY() const { return 0.5 * tileSizeY + tyvekThickness; }

  // Transverse pitch used when arranging modules in an array.
  G4double ModulePitchX() const { return tileSizeX + 2. * tyvekThickness + moduleGap; }
  G4double ModulePitchY() const { return tileSizeY + 2. * tyvekThickness + moduleGap; }

  G4int NumModules() const { return arrayNx * arrayNy; }

  // Apply one of the paper's module presets (also rebuilds capillaries).
  void ApplyPreset(ModulePreset preset);

  // (Re)compute the capillary positions from the current layout + tile size.
  void RebuildCapillaries();
};

#endif  // RADICAL_GEOMETRY_CONFIG_HH
