//
// RadicalRunAction.hh
//
// Sets up the ROOT output (histograms + ntuples) via G4's analysis manager.
//
#ifndef RADICAL_RUN_ACTION_HH
#define RADICAL_RUN_ACTION_HH

#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;

// Histogram / ntuple ids shared with RadicalEventAction.
namespace radana {
enum H1 { kLong = 0, kSiPMnpe, kTiming, kELyso, kEPb };
enum H2 { kCentroid = 0 };
enum NT { kEvent = 0, kTile, kChannel };
}  // namespace radana

class RadicalRunAction : public G4UserRunAction {
 public:
  RadicalRunAction();
  ~RadicalRunAction() override;

  void BeginOfRunAction(const G4Run*) override;
  void EndOfRunAction(const G4Run*) override;

  void SetFileName(const G4String& n) { fFileName = n; }

 private:
  G4String fFileName = "radical";
};

#endif  // RADICAL_RUN_ACTION_HH
