//
// RadicalDetectorMessenger.cc
//
#include "RadicalDetectorMessenger.hh"
#include "RadicalDetectorConstruction.hh"
#include "RadicalMaterials.hh"
#include "RadicalStackingAction.hh"
#include "RadicalEventAction.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithoutParameter.hh"

RadicalDetectorMessenger::RadicalDetectorMessenger(
    RadicalDetectorConstruction* det)
    : fDet(det) {
  fGeoDir = new G4UIdirectory("/radical/geo/");
  fGeoDir->SetGuidance("RADiCAL geometry configuration");
  fOptDir = new G4UIdirectory("/radical/optical/");
  fOptDir->SetGuidance("Optical-photon runtime controls");

  fPreset = new G4UIcmdWithAString("/radical/geo/preset", this);
  fPreset->SetGuidance("Module preset: current (14mm) | enhanced (18mm)");
  fPreset->SetParameterName("preset", false);
  fPreset->SetCandidates("current enhanced");

  fShape = new G4UIcmdWithAString("/radical/geo/tileShape", this);
  fShape->SetGuidance("Tile cross-section: square | rectangle | hexagon");
  fShape->SetParameterName("shape", false);
  fShape->SetCandidates("square rectangle hexagon");

  fLayout = new G4UIcmdWithAString("/radical/geo/layout", this);
  fLayout->SetGuidance("Capillary layout: quincunx5 | grid9");
  fLayout->SetParameterName("layout", false);
  fLayout->SetCandidates("quincunx5 grid9");

  fTiling = new G4UIcmdWithAString("/radical/geo/arrayTiling", this);
  fTiling->SetGuidance("Array tiling: square | hex");
  fTiling->SetParameterName("tiling", false);
  fTiling->SetCandidates("square hex");

  fNx = new G4UIcmdWithAnInteger("/radical/geo/arrayNx", this);
  fNx->SetParameterName("nx", false);
  fNy = new G4UIcmdWithAnInteger("/radical/geo/arrayNy", this);
  fNy->SetParameterName("ny", false);

  fSizeX = new G4UIcmdWithADoubleAndUnit("/radical/geo/tileSizeX", this);
  fSizeX->SetParameterName("sx", false);
  fSizeX->SetUnitCategory("Length");
  fSizeY = new G4UIcmdWithADoubleAndUnit("/radical/geo/tileSizeY", this);
  fSizeY->SetParameterName("sy", false);
  fSizeY->SetUnitCategory("Length");
  fHexR = new G4UIcmdWithADoubleAndUnit("/radical/geo/hexR", this);
  fHexR->SetGuidance("Hexagon circumradius (centre->vertex)");
  fHexR->SetParameterName("R", false);
  fHexR->SetUnitCategory("Length");
  fTyvek = new G4UIcmdWithADoubleAndUnit("/radical/geo/tyvek", this);
  fTyvek->SetGuidance("Tyvek wrap thickness per face");
  fTyvek->SetParameterName("t", false);
  fTyvek->SetUnitCategory("Length");

  fBeamline = new G4UIcmdWithABool("/radical/geo/beamline", this);
  fMCP = new G4UIcmdWithABool("/radical/geo/mcp", this);
  fPbGlass = new G4UIcmdWithABool("/radical/geo/pbglass", this);
  fCounters = new G4UIcmdWithABool("/radical/geo/counters", this);

  fCheckOverlaps = new G4UIcmdWithABool("/radical/geo/checkOverlaps", this);
  fCheckOverlaps->SetGuidance("Print a per-volume overlap check at construction");

  fUpdate = new G4UIcmdWithoutParameter("/radical/geo/update", this);
  fUpdate->SetGuidance("Rebuild the geometry after changing parameters");

  fYieldScale = new G4UIcmdWithADouble("/radical/optical/yieldScale", this);
  fYieldScale->SetGuidance(
      "Scintillation yield multiplier (<1 for high-energy runs). "
      "Set BEFORE /run/initialize.");
  fYieldScale->SetParameterName("scale", false);

  fMaxPhotons = new G4UIcmdWithAnInteger("/radical/optical/maxPhotons", this);
  fMaxPhotons->SetGuidance("Per-event optical photon cap (0 = no cap)");
  fMaxPhotons->SetParameterName("n", false);

  fMcpRes = new G4UIcmdWithADoubleAndUnit("/radical/optical/mcpResolution", this);
  fMcpRes->SetGuidance("MCP reference single-shot time resolution (e.g. 18 ps)");
  fMcpRes->SetParameterName("t", false);
  fMcpRes->SetUnitCategory("Time");

  fSipmJit = new G4UIcmdWithADoubleAndUnit("/radical/optical/sipmJitter", this);
  fSipmJit->SetGuidance("Per-SiPM-channel electronics time jitter");
  fSipmJit->SetParameterName("t", false);
  fSipmJit->SetUnitCategory("Time");
}

RadicalDetectorMessenger::~RadicalDetectorMessenger() {
  delete fPreset; delete fShape; delete fLayout; delete fTiling;
  delete fNx; delete fNy; delete fSizeX; delete fSizeY; delete fHexR;
  delete fTyvek; delete fBeamline; delete fMCP; delete fPbGlass;
  delete fCounters; delete fCheckOverlaps; delete fUpdate;
  delete fYieldScale; delete fMaxPhotons; delete fMcpRes; delete fSipmJit;
  delete fGeoDir; delete fOptDir;
}

void RadicalDetectorMessenger::SetNewValue(G4UIcommand* cmd, G4String v) {
  auto& cfg = fDet->Config();

  if (cmd == fPreset) {
    cfg.ApplyPreset(v == "enhanced" ? ModulePreset::Enhanced18
                                    : ModulePreset::Current14);
  } else if (cmd == fShape) {
    cfg.tileShape = (v == "hexagon")     ? TileShape::Hexagon
                    : (v == "rectangle") ? TileShape::Rectangle
                                         : TileShape::Square;
  } else if (cmd == fLayout) {
    cfg.layout = (v == "grid9") ? CapillaryLayout::Grid9
                                : CapillaryLayout::Quincunx5;
    cfg.RebuildCapillaries();
  } else if (cmd == fTiling) {
    cfg.arrayTiling = (v == "hex") ? ArrayTiling::Hex : ArrayTiling::Square;
  } else if (cmd == fNx) {
    cfg.arrayNx = fNx->GetNewIntValue(v);
  } else if (cmd == fNy) {
    cfg.arrayNy = fNy->GetNewIntValue(v);
  } else if (cmd == fSizeX) {
    cfg.tileSizeX = fSizeX->GetNewDoubleValue(v);
  } else if (cmd == fSizeY) {
    cfg.tileSizeY = fSizeY->GetNewDoubleValue(v);
  } else if (cmd == fHexR) {
    cfg.hexCircumradius = fHexR->GetNewDoubleValue(v);
  } else if (cmd == fTyvek) {
    cfg.tyvekThickness = fTyvek->GetNewDoubleValue(v);
  } else if (cmd == fBeamline) {
    cfg.includeBeamline = fBeamline->GetNewBoolValue(v);
  } else if (cmd == fMCP) {
    cfg.includeMCP = fMCP->GetNewBoolValue(v);
  } else if (cmd == fPbGlass) {
    cfg.includePbGlass = fPbGlass->GetNewBoolValue(v);
  } else if (cmd == fCounters) {
    cfg.includeCounters = fCounters->GetNewBoolValue(v);
  } else if (cmd == fCheckOverlaps) {
    cfg.checkOverlaps = fCheckOverlaps->GetNewBoolValue(v);
  } else if (cmd == fUpdate) {
    fDet->RebuildGeometry();
  } else if (cmd == fYieldScale) {
    RadicalMaterials::SetYieldScale(fYieldScale->GetNewDoubleValue(v));
  } else if (cmd == fMaxPhotons) {
    RadicalStackingAction::SetMaxOpticalPhotons(fMaxPhotons->GetNewIntValue(v));
  } else if (cmd == fMcpRes) {
    RadicalEventAction::SetMCPResolution(fMcpRes->GetNewDoubleValue(v));
  } else if (cmd == fSipmJit) {
    RadicalEventAction::SetSiPMJitter(fSipmJit->GetNewDoubleValue(v));
  }
}
