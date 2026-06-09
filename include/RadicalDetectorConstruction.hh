//
// RadicalDetectorConstruction.hh
//
// Builds the full beam-test geometry of Fig. 11c:
//   beam  ->  A1/A2 trigger counters  ->  beam chamber  ->  MCP  ->
//   RADiCAL module(s)  ->  Pb-glass backing calorimeter.
//
// The RADiCAL module geometry is fully parametrized through
// RadicalGeometryConfig (tile shape, capillary layout, module size, array
// tiling) so the alternative designs of Fig. 28 can be studied.
//
#ifndef RADICAL_DETECTOR_CONSTRUCTION_HH
#define RADICAL_DETECTOR_CONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "RadicalGeometryConfig.hh"
#include <vector>

class G4VSolid;
class G4LogicalVolume;
class G4VPhysicalVolume;
class RadicalDetectorMessenger;

class RadicalDetectorConstruction : public G4VUserDetectorConstruction {
 public:
  RadicalDetectorConstruction();
  ~RadicalDetectorConstruction() override;

  G4VPhysicalVolume* Construct() override;
  void ConstructSDandField() override;

  RadicalGeometryConfig& Config() { return fConfig; }
  // Rebuild the geometry after a messenger change (/radical/geo/update).
  void RebuildGeometry();

 private:
  // --- builders ---
  G4LogicalVolume*  BuildModuleLogical();
  void              BuildBeamline(G4LogicalVolume* worldLV);
  G4LogicalVolume*  BuildPhotodetector(const G4String& name, G4double halfX,
                                       G4double halfY, G4double halfZ,
                                       G4bool round);

  // Cross-section solid of one tile (square / rectangle / hexagon) with the
  // capillary holes drilled through it (axis along z = beam).
  G4VSolid* MakeTileSolid(const G4String& name, G4double halfZ);
  // Drill the configured capillary holes out of an existing prism solid.
  G4VSolid* DrillHoles(const G4String& name, G4VSolid* base, G4double halfZ);

  void DefineVisAttributes();

  RadicalGeometryConfig      fConfig;
  RadicalDetectorMessenger*  fMessenger = nullptr;

  // logical volumes that become sensitive detectors (set in Construct)
  G4LogicalVolume* fLysoLV       = nullptr;  // calorimeter energy
  G4LogicalVolume* fSipmLV        = nullptr;  // module SiPM readout
  G4LogicalVolume* fMcpCathodeLV  = nullptr;  // MCP timing reference
  G4LogicalVolume* fPbReadoutLV   = nullptr;  // Pb-glass Cerenkov readout
  G4LogicalVolume* fPbGlassLV     = nullptr;  // Pb-glass energy (leakage)
  G4LogicalVolume* fCounterReadLV = nullptr;  // A1/A2 scintillator readout
  std::vector<G4LogicalVolume*> fScintLV;     // LYSO (kept for reference)
  std::vector<G4LogicalVolume*> fCounterLV;   // A1/A2 plastic scintillators

  G4VPhysicalVolume* fWorldPV = nullptr;
};

#endif  // RADICAL_DETECTOR_CONSTRUCTION_HH
