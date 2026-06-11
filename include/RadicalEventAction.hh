//
// RadicalEventAction.hh
//
// Per-event bookkeeping: collects the calorimeter hit collections and the
// optical photons detected (reported by the stepping action), then fills the
// ROOT ntuples / histograms.
//
#ifndef RADICAL_EVENT_ACTION_HH
#define RADICAL_EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <map>
#include <vector>

class RadicalEventAction : public G4UserEventAction {
 public:
  RadicalEventAction() = default;
  ~RadicalEventAction() override = default;

  void BeginOfEventAction(const G4Event*) override;
  void EndOfEventAction(const G4Event*) override;

  // --- called by RadicalSteppingAction on photon detection ---------------
  void AddSiPMPhoton(G4int module, G4int channel, G4double t);
  void AddMCPPhoton(G4double t);
  void AddPbPhoton(G4int block, G4double t);
  void AddCounterPhoton(G4int id, G4double t);

  // photo-electron thresholds for leading-edge timing (like a fixed-threshold
  // discriminator on the rising edge of the SiPM / MCP pulse).
  static constexpr G4int kSiPMThreshold = 3;  // 3rd detected p.e.
  static constexpr G4int kMCPThreshold = 3;   // 3rd detected p.e. (robust leading edge)

 private:
  struct PD { G4int n = 0; G4double tFirst = 1.e9; };
  void Tally(std::map<G4long, PD>& m, G4long key, G4double t);
  // time of the k-th earliest photon (fixed-threshold leading edge)
  static G4double ThresholdTime(std::vector<G4double>& times, G4int k);

  std::map<G4long, std::vector<G4double>> fSiPM;  // channel -> photon times
  std::vector<G4double> fMCP;                      // MCP photon times
  std::map<G4long, PD> fPb;       // key = block (count + first)
  std::map<G4long, PD> fCounter;  // key = counter id (count + first)

  G4int fLysoHCID = -1;
  G4int fPbHCID = -1;
  G4int fCounterHCID = -1;
};

#endif  // RADICAL_EVENT_ACTION_HH
