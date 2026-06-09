//
// RadicalCaloSD.cc
//
#include "RadicalCaloSD.hh"

#include "G4Step.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4TouchableHistory.hh"
#include "G4OpticalPhoton.hh"

G4ThreadLocal G4Allocator<RadicalCaloHit>* RadicalCaloHitAllocator = nullptr;

RadicalCaloSD::RadicalCaloSD(const G4String& name, const G4String& hcName,
                             G4int layerDepth, G4int moduleDepth)
    : G4VSensitiveDetector(name),
      fHCName(hcName),
      fLayerDepth(layerDepth),
      fModuleDepth(moduleDepth) {
  collectionName.insert(hcName);
}

void RadicalCaloSD::Initialize(G4HCofThisEvent* hce) {
  fHits = new RadicalCaloHitsCollection(SensitiveDetectorName, fHCName);
  if (fHCID < 0)
    fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHits);
  hce->AddHitsCollection(fHCID, fHits);
  fMap.clear();
}

G4bool RadicalCaloSD::ProcessHits(G4Step* step, G4TouchableHistory*) {
  // Ignore optical photons here (handled in the stepping action).
  if (step->GetTrack()->GetDefinition() == G4OpticalPhoton::Definition())
    return false;

  const G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.) return false;

  const auto* touch = step->GetPreStepPoint()->GetTouchable();
  const G4int layer = touch->GetCopyNumber(fLayerDepth);
  const G4int module =
      (fModuleDepth >= 0) ? touch->GetCopyNumber(fModuleDepth) : 0;
  const G4int id = module * 1000 + layer;

  auto it = fMap.find(id);
  RadicalCaloHit* hit = nullptr;
  if (it == fMap.end()) {
    hit = new RadicalCaloHit(id);
    fMap[id] = hit;
    fHits->insert(hit);
  } else {
    hit = it->second;
  }
  hit->Add(edep, step->GetPreStepPoint()->GetGlobalTime(),
           step->GetPreStepPoint()->GetPosition());
  return true;
}
