//
// RadicalSteppingAction.cc
//
#include "RadicalSteppingAction.hh"
#include "RadicalEventAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"

void RadicalSteppingAction::UserSteppingAction(const G4Step* step) {
  auto* track = step->GetTrack();
  if (track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition())
    return;

  auto* post = step->GetPostStepPoint();

  // Loop breaker: kill optical photons that bounce excessively (endless total
  // internal reflection off the 95% Tyvek, or navigation traps at the drilled
  // capillary holes). These late/trapped photons cannot affect the leading-edge
  // (first few p.e.) timing, so removing them is safe and keeps runs tractable.
  if (track->GetCurrentStepNumber() > 1000 ||
      post->GetGlobalTime() > 120. * ns) {
    track->SetTrackStatus(fStopAndKill);
    return;
  }

  if (post->GetStepStatus() != fGeomBoundary) return;

  // Lazily locate the optical boundary process.
  if (!fBoundary) {
    auto* pm = G4OpticalPhoton::OpticalPhotonDefinition()->GetProcessManager();
    auto* pv = pm->GetProcessList();
    for (size_t i = 0; i < pv->size(); ++i) {
      if ((*pv)[i]->GetProcessName() == "OpBoundary") {
        fBoundary = dynamic_cast<G4OpBoundaryProcess*>((*pv)[i]);
        break;
      }
    }
  }
  if (!fBoundary || fBoundary->GetStatus() != Detection) return;

  auto* touch = post->GetTouchableHandle()();
  auto* vol = touch->GetVolume();
  if (!vol) return;
  const G4String& name = vol->GetLogicalVolume()->GetName();
  const G4double t = post->GetGlobalTime();

  if (name == "SiPM") {
    const G4int channel = touch->GetCopyNumber(0);
    const G4int module = touch->GetCopyNumber(1);  // module envelope copy
    fEvent->AddSiPMPhoton(module, channel, t);
  } else if (name == "MCPcathode") {
    fEvent->AddMCPPhoton(t);
  } else if (name == "PbReadout") {
    fEvent->AddPbPhoton(touch->GetCopyNumber(0), t);
  } else if (name == "CounterReadout") {
    fEvent->AddCounterPhoton(touch->GetCopyNumber(0), t);
  }
}
