//
// RadicalDetectorConstruction.cc
//
#include "RadicalDetectorConstruction.hh"
#include "RadicalDetectorMessenger.hh"
#include "RadicalMaterials.hh"
#include "RadicalCaloSD.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4ExtrudedSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpticalSurface.hh"
#include "G4SDManager.hh"
#include "G4TwoVector.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4RunManager.hh"

#include <vector>
#include <cmath>

namespace {
// Build a prism (extruded along z) with the requested cross-section shape.
G4VSolid* MakePrism(const G4String& name, TileShape shape, G4double halfX,
                    G4double halfY, G4double circumR, G4double halfZ) {
  if (shape == TileShape::Hexagon) {
    std::vector<G4TwoVector> poly;
    for (int k = 0; k < 6; ++k) {
      const G4double a = (60.0 * k + 30.0) * deg;   // flat-top hexagon
      poly.emplace_back(circumR * std::cos(a), circumR * std::sin(a));
    }
    std::vector<G4ExtrudedSolid::ZSection> z = {
        {-halfZ, G4TwoVector(0, 0), 1.0}, {+halfZ, G4TwoVector(0, 0), 1.0}};
    return new G4ExtrudedSolid(name, poly, z);
  }
  // Square and Rectangle are both boxes (halfX == halfY for square).
  return new G4Box(name, halfX, halfY, halfZ);
}
}  // namespace

RadicalDetectorConstruction::RadicalDetectorConstruction() {
  fConfig.RebuildCapillaries();
  fMessenger = new RadicalDetectorMessenger(this);
}

RadicalDetectorConstruction::~RadicalDetectorConstruction() { delete fMessenger; }

// ---------------------------------------------------------------------------
G4VSolid* RadicalDetectorConstruction::MakeTileSolid(const G4String& name,
                                                     G4double halfZ) {
  G4VSolid* base = MakePrism(name + "_base", fConfig.tileShape,
                             0.5 * fConfig.tileSizeX, 0.5 * fConfig.tileSizeY,
                             fConfig.hexCircumradius, halfZ);
  return DrillHoles(name, base, halfZ);
}

G4VSolid* RadicalDetectorConstruction::DrillHoles(const G4String& name,
                                                  G4VSolid* base,
                                                  G4double halfZ) {
  G4VSolid* solid = base;
  int i = 0;
  for (const auto& c : fConfig.capillaries) {
    auto* hole = new G4Tubs(name + "_h" + std::to_string(i++), 0.,
                            0.5 * c.holeDiameter, halfZ * 1.2, 0., twopi);
    solid = new G4SubtractionSolid(name + "_d" + std::to_string(i), solid, hole,
                                   nullptr, G4ThreeVector(c.x, c.y, 0.));
  }
  return solid;
}

// ---------------------------------------------------------------------------
G4LogicalVolume* RadicalDetectorConstruction::BuildModuleLogical() {
  auto* M = RadicalMaterials::Instance();
  const auto& cfg = fConfig;

  // --- envelope -----------------------------------------------------------
  G4double bX = (cfg.tileShape == TileShape::Hexagon)
                    ? cfg.hexCircumradius
                    : 0.5 * cfg.tileSizeX;
  G4double bY = (cfg.tileShape == TileShape::Hexagon)
                    ? cfg.hexCircumradius
                    : 0.5 * cfg.tileSizeY;
  bX += cfg.tyvekThickness;
  bY += cfg.tyvekThickness;
  const G4double endSpace = cfg.sipmThickness + cfg.sipmWindow + cfg.readoutCardThk;
  const G4double envHalfZ =
      0.5 * std::max(cfg.StackLength(), cfg.capillaryLength) + endSpace + 2. * mm;

  auto* envSolid = new G4Box("Module_env", bX + 1. * mm, bY + 1. * mm, envHalfZ);
  auto* envLV = new G4LogicalVolume(envSolid, M->World(), "Module");

  // --- shared wrapped-LYSO logical ---------------------------------------
  const G4double lysoHalf = 0.5 * cfg.lysoThickness;
  const G4double wrapHalf = lysoHalf + cfg.tyvekThickness;

  // Tyvek wrap (cross-section expanded by the wrap thickness).
  G4VSolid* tyvekBase = MakePrism(
      "Tyvek_base", cfg.tileShape, 0.5 * cfg.tileSizeX + cfg.tyvekThickness,
      0.5 * cfg.tileSizeY + cfg.tyvekThickness,
      cfg.hexCircumradius + cfg.tyvekThickness, wrapHalf);
  G4VSolid* tyvekSolid = DrillHoles("Tyvek", tyvekBase, wrapHalf);
  auto* tyvekLV = new G4LogicalVolume(tyvekSolid, M->Tyvek(), "TyvekWrap");
  new G4LogicalSkinSurface("TyvekSkin", tyvekLV, M->TyvekSurface());

  G4VSolid* lysoSolid = MakeTileSolid("LYSO", lysoHalf);
  fLysoLV = new G4LogicalVolume(lysoSolid, M->LYSO(), "LYSO");
  new G4PVPlacement(nullptr, {}, fLysoLV, "LYSO_pv", tyvekLV, false, 0, true);
  fScintLV.push_back(fLysoLV);

  // Tungsten absorber.
  G4VSolid* wSolid = MakeTileSolid("W", 0.5 * cfg.wThickness);
  auto* wLV = new G4LogicalVolume(wSolid, M->Tungsten(), "Tungsten");
  new G4LogicalSkinSurface("WSkin", wLV, M->PolishedAbsorber());

  // --- stack the layers along z ------------------------------------------
  G4double z = -0.5 * cfg.StackLength();
  for (G4int i = 0; i < cfg.nLysoPlates; ++i) {
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, z + wrapHalf), tyvekLV,
                      "Wrap_pv", envLV, false, i, true);
    z += 2. * wrapHalf;
    if (i < cfg.nWPlates) {
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, z + 0.5 * cfg.wThickness),
                        wLV, "W_pv", envLV, false, i, true);
      z += cfg.wThickness;
    }
  }

  // --- capillaries + WLS filaments ---------------------------------------
  const G4double showerMaxLocal = -0.5 * cfg.StackLength() + cfg.showerMaxZ;
  int capIdx = 0;
  for (const auto& c : cfg.capillaries) {
    auto* capTube = new G4Tubs("Capillary", 0., 0.5 * c.outerDiameter,
                               0.5 * cfg.capillaryLength, 0., twopi);
    auto* capLV = new G4LogicalVolume(capTube, M->Quartz(), "Capillary");
    new G4PVPlacement(nullptr, G4ThreeVector(c.x, c.y, 0.), capLV, "Cap_pv",
                      envLV, false, capIdx, true);

    if (c.type != CapType::Calibration) {
      G4double filLen = (c.type == CapType::EType) ? cfg.capillaryLength
                                                   : cfg.tFilamentLength;
      G4double filZ = (c.type == CapType::EType) ? 0. : showerMaxLocal;
      G4Material* filMat = (c.type == CapType::EType) ? M->LuAG() : M->DSB1();
      auto* filSolid = new G4Tubs("Filament", 0., 0.5 * c.filamentDiameter,
                                  0.5 * filLen, 0., twopi);
      auto* filLV = new G4LogicalVolume(filSolid, filMat, "WLSFilament");
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, filZ), filLV, "Fil_pv",
                        capLV, false, capIdx, true);
    }
    ++capIdx;
  }

  // --- SiPM photosensors at both capillary ends --------------------------
  // 4 corner capillaries x 2 ends = 8 channels (matches the paper's readout).
  G4double sipmRefR = 0.;
  for (const auto& c : cfg.capillaries) sipmRefR = std::max(sipmRefR, 0.5 * c.outerDiameter);
  auto* sipmSolid = new G4Tubs("SiPM", 0., sipmRefR, 0.5 * cfg.sipmThickness, 0., twopi);
  fSipmLV = new G4LogicalVolume(sipmSolid, M->Silicon(), "SiPM");
  new G4LogicalSkinSurface("SiPMSkin", fSipmLV, M->PhotocathodeSurface());

  const G4double zDown = 0.5 * cfg.capillaryLength + 0.5 * cfg.sipmThickness;
  capIdx = 0;
  for (const auto& c : cfg.capillaries) {
    if (c.type != CapType::Calibration) {
      // downstream channel = capIdx, upstream channel = capIdx + 50
      new G4PVPlacement(nullptr, G4ThreeVector(c.x, c.y, +zDown), fSipmLV,
                        "SiPM_pv", envLV, false, capIdx, true);
      if (cfg.sipmBothEnds)
        new G4PVPlacement(nullptr, G4ThreeVector(c.x, c.y, -zDown), fSipmLV,
                          "SiPM_pv", envLV, false, capIdx + 50, true);
    }
    ++capIdx;
  }

  return envLV;
}

// ---------------------------------------------------------------------------
G4LogicalVolume* RadicalDetectorConstruction::BuildPhotodetector(
    const G4String& name, G4double halfX, G4double halfY, G4double halfZ,
    G4bool round) {
  auto* M = RadicalMaterials::Instance();
  G4VSolid* s;
  if (round)
    s = new G4Tubs(name, 0., halfX, halfZ, 0., twopi);
  else
    s = new G4Box(name, halfX, halfY, halfZ);
  auto* lv = new G4LogicalVolume(s, M->Bialkali(), name);
  new G4LogicalSkinSurface(name + "Skin", lv, M->PhotocathodeSurface());
  return lv;
}

// ---------------------------------------------------------------------------
void RadicalDetectorConstruction::BuildBeamline(G4LogicalVolume* worldLV) {
  auto* M = RadicalMaterials::Instance();
  const auto& cfg = fConfig;

  // --- A1 / A2 trigger counters (plastic scintillator) -------------------
  if (cfg.includeCounters) {
    struct C { G4double size; G4double z; G4int id; };
    std::vector<C> counters = {{10. * mm, cfg.counterA1_z, 0},
                               {20. * mm, cfg.counterA2_z, 1}};
    auto* readLV = BuildPhotodetector("CounterReadout", 0.5 * 20. * mm,
                                      2. * mm, 0.3 * mm, false);
    fCounterReadLV = readLV;
    for (auto& c : counters) {
      auto* sc = new G4Box("Counter", 0.5 * c.size, 0.5 * c.size, 2.5 * mm);
      auto* scLV = new G4LogicalVolume(sc, M->PlasticScint(), "Counter");
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, c.z), scLV, "Counter_pv",
                        worldLV, false, c.id, true);
      fCounterLV.push_back(scLV);
      // scintillation readout patch on the +x edge
      new G4PVPlacement(nullptr, G4ThreeVector(0.5 * c.size + 0.3 * mm, 0, c.z),
                        readLV, "CntRead_pv", worldLV, false, c.id, true);
    }
  }

  // --- upstream beam position chamber (passive thin gas) ------------------
  if (cfg.includeBeamChamber) {
    auto* ch = new G4Box("BeamChamber", 30. * mm, 30. * mm, 5. * mm);
    auto* chLV = new G4LogicalVolume(ch, M->Vacuum(), "BeamChamber");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, cfg.beamChamber_z), chLV,
                      "BeamChamber_pv", worldLV, false, 0, true);
  }

  // --- MCP timing reference (Hamamatsu R3809U-50) ------------------------
  if (cfg.includeMCP) {
    const G4double tubeR = 12.5 * mm, tubeHalfZ = 25. * mm;
    auto* tube = new G4Tubs("MCPtube", 0., tubeR, tubeHalfZ, 0., twopi);
    auto* tubeLV = new G4LogicalVolume(tube, M->Vacuum(), "MCPtube");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, cfg.mcp_z), tubeLV,
                      "MCPtube_pv", worldLV, false, 0, true);
    // quartz entrance window (Cerenkov radiator for the crossing beam)
    auto* win = new G4Tubs("MCPwindow", 0., 11. * mm, 1.25 * mm, 0., twopi);
    auto* winLV = new G4LogicalVolume(win, M->Quartz(), "MCPwindow");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, -tubeHalfZ + 1.25 * mm),
                      winLV, "MCPwindow_pv", tubeLV, false, 0, true);
    // bialkali photocathode just behind the window
    fMcpCathodeLV = BuildPhotodetector("MCPcathode", 11. * mm, 0., 0.05 * mm, true);
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, -tubeHalfZ + 2.6 * mm),
                      fMcpCathodeLV, "MCPcathode_pv", tubeLV, false, 0, true);
  }

  // --- Pb-glass backing calorimeter: 2x2 array, 4x4x40 cm total ----------
  if (cfg.includePbGlass) {
    const G4double blk = 20. * mm;        // 2x2 cm cross section per block
    const G4double blkHalfZ = 200. * mm;  // 40 cm deep
    const G4double frontZ = cfg.module_z + 0.5 * fConfig.StackLength() +
                            cfg.pbGlassGap;
    const G4double centerZ = frontZ + blkHalfZ;
    auto* glass = new G4Box("PbGlass", 0.5 * blk, 0.5 * blk, blkHalfZ);
    auto* glassLV = new G4LogicalVolume(glass, M->LeadGlass(), "PbGlass");
    // store for the energy SD
    auto* readLV = BuildPhotodetector("PbReadout", 0.5 * blk, 0.5 * blk, 0.3 * mm, false);
    fPbReadoutLV = readLV;
    int id = 0;
    for (int iy = 0; iy < 2; ++iy)
      for (int ix = 0; ix < 2; ++ix) {
        G4double x = (ix - 0.5) * blk, y = (iy - 0.5) * blk;
        new G4PVPlacement(nullptr, G4ThreeVector(x, y, centerZ), glassLV,
                          "PbGlass_pv", worldLV, false, id, true);
        new G4PVPlacement(nullptr,
                          G4ThreeVector(x, y, centerZ + blkHalfZ + 0.3 * mm),
                          readLV, "PbRead_pv", worldLV, false, id, true);
        ++id;
      }
    fPbGlassLV = glassLV;  // energy SD attached in ConstructSDandField
  }
}

// ---------------------------------------------------------------------------
G4VPhysicalVolume* RadicalDetectorConstruction::Construct() {
  RadicalMaterials::Instance()->Build();
  auto* M = RadicalMaterials::Instance();
  const auto& cfg = fConfig;

  // --- world --------------------------------------------------------------
  auto* worldSolid = new G4Box("World", 0.75 * m, 0.75 * m, 1.5 * m);
  auto* worldLV = new G4LogicalVolume(worldSolid, M->World(), "World");
  fWorldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0, true);

  // --- module(s) ----------------------------------------------------------
  G4LogicalVolume* moduleLV = BuildModuleLogical();
  const G4double pitchX = cfg.ModulePitchX();
  const G4double pitchY = cfg.ModulePitchY();
  int modId = 0;
  for (int iy = 0; iy < cfg.arrayNy; ++iy) {
    for (int ix = 0; ix < cfg.arrayNx; ++ix) {
      G4double x = (ix - 0.5 * (cfg.arrayNx - 1)) * pitchX;
      G4double y = (iy - 0.5 * (cfg.arrayNy - 1)) * pitchY;
      if (cfg.arrayTiling == ArrayTiling::Hex) {
        // staggered rows, vertical pitch reduced to sqrt(3)/2
        y = (iy - 0.5 * (cfg.arrayNy - 1)) * pitchY * std::sqrt(3.) / 2.;
        if (iy % 2) x += 0.5 * pitchX;
      }
      new G4PVPlacement(nullptr, G4ThreeVector(x, y, cfg.module_z), moduleLV,
                        "Module_pv", worldLV, false, modId++, true);
    }
  }

  // --- beam line ----------------------------------------------------------
  if (cfg.includeBeamline) BuildBeamline(worldLV);

  DefineVisAttributes();
  return fWorldPV;
}

// ---------------------------------------------------------------------------
void RadicalDetectorConstruction::ConstructSDandField() {
  auto* sdm = G4SDManager::GetSDMpointer();

  // Charged-particle energy deposition.  Optical-photon *detection* at the
  // photocathodes (SiPM, MCP, Pb-glass readout, counters) is handled in
  // RadicalSteppingAction via the optical boundary status.

  // LYSO calorimeter tiles (module = touchable depth 2, layer = depth 1).
  auto* lysoSD = new RadicalCaloSD("LYSO_SD", "LYSO_HC", /*layerDepth*/ 1,
                                   /*moduleDepth*/ 2);
  sdm->AddNewDetector(lysoSD);
  if (fLysoLV) fLysoLV->SetSensitiveDetector(lysoSD);

  // Pb-glass leakage energy (cell = block copy number).
  if (fPbGlassLV) {
    auto* pbE = new RadicalCaloSD("PbGlassE_SD", "PbGlassE_HC", 0, -1);
    sdm->AddNewDetector(pbE);
    fPbGlassLV->SetSensitiveDetector(pbE);
  }

  // Trigger-counter energy (cell = counter id) for trigger decisions.
  if (!fCounterLV.empty()) {
    auto* cntE = new RadicalCaloSD("CounterE_SD", "CounterE_HC", 0, -1);
    sdm->AddNewDetector(cntE);
    for (auto* lv : fCounterLV) lv->SetSensitiveDetector(cntE);
  }
}

// ---------------------------------------------------------------------------
void RadicalDetectorConstruction::DefineVisAttributes() {
  auto setVis = [](const char* lvName, G4Colour col, G4bool solid = true) {
    auto* lv = G4LogicalVolumeStore::GetInstance()->GetVolume(lvName, false);
    if (!lv) return;
    auto* va = new G4VisAttributes(col);
    va->SetForceSolid(solid);
    lv->SetVisAttributes(va);
  };
  auto* worldLV = G4LogicalVolumeStore::GetInstance()->GetVolume("World", false);
  if (worldLV) worldLV->SetVisAttributes(G4VisAttributes::GetInvisible());
  auto* modLV = G4LogicalVolumeStore::GetInstance()->GetVolume("Module", false);
  if (modLV) modLV->SetVisAttributes(G4VisAttributes::GetInvisible());

  setVis("LYSO", G4Colour(0.3, 0.7, 1.0, 0.6));        // light blue
  setVis("Tungsten", G4Colour(0.4, 0.4, 0.4, 0.8));    // grey
  setVis("TyvekWrap", G4Colour(1.0, 1.0, 1.0, 0.15), false);
  setVis("Capillary", G4Colour(0.9, 0.9, 0.2, 0.5));   // quartz yellow
  setVis("WLSFilament", G4Colour(1.0, 0.5, 0.0, 0.9)); // orange
  setVis("SiPM", G4Colour(0.1, 0.8, 0.1, 1.0));        // green
  setVis("PbGlass", G4Colour(0.6, 0.4, 0.7, 0.3));     // purple
  setVis("Counter", G4Colour(0.2, 1.0, 1.0, 0.3));     // cyan
  setVis("MCPtube", G4Colour(0.5, 0.5, 0.5, 0.2), false);
  setVis("MCPwindow", G4Colour(0.9, 0.9, 0.9, 0.4));
}

// ---------------------------------------------------------------------------
void RadicalDetectorConstruction::RebuildGeometry() {
  G4GeometryManager::GetInstance()->OpenGeometry();
  G4PhysicalVolumeStore::Clean();
  G4LogicalVolumeStore::Clean();
  G4SolidStore::Clean();
  fLysoLV = fSipmLV = fMcpCathodeLV = fPbReadoutLV = fCounterReadLV = nullptr;
  fPbGlassLV = nullptr;
  fScintLV.clear();
  fConfig.RebuildCapillaries();
  auto* rm = G4RunManager::GetRunManager();
  rm->DefineWorldVolume(Construct());
  rm->GeometryHasBeenModified();
}
