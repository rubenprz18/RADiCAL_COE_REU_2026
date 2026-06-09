//
// RadicalStackingAction.cc
//
#include "RadicalStackingAction.hh"

#include "G4Track.hh"
#include "G4OpticalPhoton.hh"

G4long RadicalStackingAction::fMaxOptical = 0;

G4ClassificationOfNewTrack RadicalStackingAction::ClassifyNewTrack(
    const G4Track* track) {
  if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
    if (track->GetParentID() > 0) {  // a secondary optical photon
      ++fOpticalCount;
      if (fMaxOptical > 0 && fOpticalCount > fMaxOptical) return fKill;
      return fWaiting;  // track after the charged shower
    }
  }
  return fUrgent;
}

void RadicalStackingAction::NewStage() {}

void RadicalStackingAction::PrepareNewEvent() { fOpticalCount = 0; }
