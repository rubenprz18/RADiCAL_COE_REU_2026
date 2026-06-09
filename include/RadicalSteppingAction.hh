//
// RadicalSteppingAction.hh
//
// Detects optical photons absorbed at the photocathodes (SiPM / MCP / Pb-glass
// readout / counter readout) by inspecting the optical boundary process status,
// and reports them to the event action.
//
#ifndef RADICAL_STEPPING_ACTION_HH
#define RADICAL_STEPPING_ACTION_HH

#include "G4UserSteppingAction.hh"

class RadicalEventAction;
class G4OpBoundaryProcess;

class RadicalSteppingAction : public G4UserSteppingAction {
 public:
  explicit RadicalSteppingAction(RadicalEventAction* ea) : fEvent(ea) {}
  ~RadicalSteppingAction() override = default;

  void UserSteppingAction(const G4Step* step) override;

 private:
  RadicalEventAction* fEvent = nullptr;
  G4OpBoundaryProcess* fBoundary = nullptr;
};

#endif  // RADICAL_STEPPING_ACTION_HH
