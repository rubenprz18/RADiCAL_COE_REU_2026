//
// RadicalCaloSD.hh
//
// Generic calorimeter sensitive detector.  The cell id is built from copy
// numbers in the touchable history: layer at `layerDepth`, and (optionally)
// module at `moduleDepth` (use moduleDepth < 0 for single-cell detectors).
//
#ifndef RADICAL_CALO_SD_HH
#define RADICAL_CALO_SD_HH

#include "G4VSensitiveDetector.hh"
#include "RadicalCaloHit.hh"
#include <map>

class G4Step;
class G4HCofThisEvent;

class RadicalCaloSD : public G4VSensitiveDetector {
 public:
  RadicalCaloSD(const G4String& name, const G4String& hcName, G4int layerDepth,
                G4int moduleDepth);
  ~RadicalCaloSD() override = default;

  void Initialize(G4HCofThisEvent* hce) override;
  G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;

 private:
  G4String fHCName;
  G4int    fLayerDepth;
  G4int    fModuleDepth;
  G4int    fHCID = -1;
  RadicalCaloHitsCollection* fHits = nullptr;
  std::map<G4int, RadicalCaloHit*> fMap;   // id -> hit (per event)
};

#endif  // RADICAL_CALO_SD_HH
