//
// RadicalStackingAction.hh
//
// Optical-photon runtime control.  Optical photons are tracked after the
// charged shower (less memory).  An optional per-event cap kills surplus
// optical photons so very high-energy events stay tractable; combine with the
// scintillation yield-scale factor (RadicalMaterials::SetYieldScale) and scale
// detected p.e. back up offline.
//
#ifndef RADICAL_STACKING_ACTION_HH
#define RADICAL_STACKING_ACTION_HH

#include "G4UserStackingAction.hh"
#include "globals.hh"

class RadicalStackingAction : public G4UserStackingAction {
 public:
  RadicalStackingAction() = default;
  ~RadicalStackingAction() override = default;

  G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track*) override;
  void NewStage() override;
  void PrepareNewEvent() override;

  static void SetMaxOpticalPhotons(G4long n) { fMaxOptical = n; }

 private:
  G4long fOpticalCount = 0;
  static G4long fMaxOptical;   // 0 = no cap
};

#endif  // RADICAL_STACKING_ACTION_HH
