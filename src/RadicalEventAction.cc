//
// RadicalEventAction.cc
//
#include "RadicalEventAction.hh"
#include "RadicalRunAction.hh"
#include "RadicalCaloHit.hh"

#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4AnalysisManager.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <algorithm>
#include <limits>

// Realistic readout resolutions (defaults: realistic MCP reference, ideal SiPM).
G4double RadicalEventAction::fMCPres = 0.018 * CLHEP::ns;  // ~18 ps reference
G4double RadicalEventAction::fSiPMjit = 0.0;               // per-channel jitter

void RadicalEventAction::BeginOfEventAction(const G4Event*) {
  fSiPM.clear();
  fMCP.clear();
  fPb.clear();
  fCounter.clear();
}

void RadicalEventAction::Tally(std::map<G4long, PD>& m, G4long key, G4double t) {
  auto& d = m[key];
  d.n += 1;
  if (t < d.tFirst) d.tFirst = t;
}

G4double RadicalEventAction::ThresholdTime(std::vector<G4double>& times,
                                           G4int k) {
  if (times.empty()) return 1.e9;
  const G4int idx = std::min<G4int>(k, (G4int)times.size()) - 1;
  std::nth_element(times.begin(), times.begin() + idx, times.end());
  return times[idx];
}

void RadicalEventAction::AddSiPMPhoton(G4int module, G4int channel, G4double t) {
  fSiPM[(G4long)module * 1000 + channel].push_back(t);
}
void RadicalEventAction::AddMCPPhoton(G4double t) { fMCP.push_back(t); }
void RadicalEventAction::AddPbPhoton(G4int block, G4double t) {
  Tally(fPb, block, t);
}
void RadicalEventAction::AddCounterPhoton(G4int id, G4double t) {
  Tally(fCounter, id, t);
}

namespace {
RadicalCaloHitsCollection* GetHC(const G4Event* evt, G4int& id,
                                 const char* hcName) {
  if (id < 0) id = G4SDManager::GetSDMpointer()->GetCollectionID(hcName);
  auto* hce = evt->GetHCofThisEvent();
  if (!hce || id < 0) return nullptr;
  return static_cast<RadicalCaloHitsCollection*>(hce->GetHC(id));
}
}  // namespace

void RadicalEventAction::EndOfEventAction(const G4Event* evt) {
  auto* ana = G4AnalysisManager::Instance();
  const G4int eid = evt->GetEventID();

  // ---- beam (primary) ----------------------------------------------------
  G4double beamE = 0., beamX = 0., beamY = 0.;
  if (evt->GetNumberOfPrimaryVertex() > 0) {
    auto* v = evt->GetPrimaryVertex(0);
    beamX = v->GetX0();
    beamY = v->GetY0();
    if (v->GetNumberOfParticle() > 0)
      beamE = v->GetPrimary(0)->GetKineticEnergy();
  }

  // ---- LYSO tiles --------------------------------------------------------
  G4double eLyso = 0.;
  G4int maxLayer = -1;
  G4double maxLayerE = -1.;
  G4double xC = 0., yC = 0.;
  if (auto* hc = GetHC(evt, fLysoHCID, "LYSO_SD/LYSO_HC")) {
    for (size_t i = 0; i < hc->entries(); ++i) {
      auto* h = (*hc)[i];
      const G4int module = h->Id() / 1000;
      const G4int layer = h->Id() % 1000;
      eLyso += h->Edep();
      auto c = h->Centroid();
      ana->FillNtupleIColumn(radana::kTile, 0, eid);
      ana->FillNtupleIColumn(radana::kTile, 1, module);
      ana->FillNtupleIColumn(radana::kTile, 2, layer);
      ana->FillNtupleDColumn(radana::kTile, 3, h->Edep() / MeV);
      ana->FillNtupleDColumn(radana::kTile, 4, h->Time() / ns);
      ana->FillNtupleDColumn(radana::kTile, 5, c.x() / mm);
      ana->FillNtupleDColumn(radana::kTile, 6, c.y() / mm);
      ana->AddNtupleRow(radana::kTile);
      ana->FillH1(radana::kLong, layer, h->Edep() / MeV);
      if (h->Edep() > maxLayerE) {
        maxLayerE = h->Edep();
        maxLayer = layer;
        xC = c.x();
        yC = c.y();
      }
    }
  }
  if (maxLayer >= 0) {
    ana->FillH2(radana::kCentroid, xC / mm, yC / mm);
  }

  // ---- Pb glass leakage --------------------------------------------------
  G4double ePb = 0.;
  if (auto* hc = GetHC(evt, fPbHCID, "PbGlassE_SD/PbGlassE_HC"))
    for (size_t i = 0; i < hc->entries(); ++i) ePb += (*hc)[i]->Edep();

  // ---- trigger counters --------------------------------------------------
  G4double eCnt = 0.;
  if (auto* hc = GetHC(evt, fCounterHCID, "CounterE_SD/CounterE_HC"))
    for (size_t i = 0; i < hc->entries(); ++i) eCnt += (*hc)[i]->Edep();

  // ---- SiPM channels (fixed p.e.-threshold leading-edge timing) ----------
  G4int npeSiPM = 0;
  G4double tDownSum = 0., tUpSum = 0.;
  G4int nDown = 0, nUp = 0;
  for (auto& kv : fSiPM) {
    const G4int module = (G4int)(kv.first / 1000);
    const G4int channel = (G4int)(kv.first % 1000);
    const G4int npe = (G4int)kv.second.size();
    G4double tHitNs = ThresholdTime(kv.second, kSiPMThreshold);
    if (tHitNs < 1.e8) tHitNs += G4RandGauss::shoot(0., fSiPMjit);  // electronics
    const G4double tHit = tHitNs / ns;
    npeSiPM += npe;
    ana->FillNtupleIColumn(radana::kChannel, 0, eid);
    ana->FillNtupleIColumn(radana::kChannel, 1, module);
    ana->FillNtupleIColumn(radana::kChannel, 2, channel);
    ana->FillNtupleIColumn(radana::kChannel, 3, npe);
    ana->FillNtupleDColumn(radana::kChannel, 4, tHit);
    ana->AddNtupleRow(radana::kChannel);
    // require enough p.e. to form a stable threshold time
    if (npe >= kSiPMThreshold) {
      if (channel >= 50) { tUpSum += tHit; ++nUp; }   // upstream end
      else               { tDownSum += tHit; ++nDown; }
    }
  }
  const G4double tDown = nDown ? tDownSum / nDown : 0.;
  const G4double tUp = nUp ? tUpSum / nUp : 0.;
  ana->FillH1(radana::kSiPMnpe, npeSiPM);

  // ---- timing ------------------------------------------------------------
  const G4int npeMCP = (G4int)fMCP.size();
  G4double tMCPns = ThresholdTime(fMCP, kMCPThreshold);
  if (tMCPns < 1.e8) tMCPns += G4RandGauss::shoot(0., fMCPres);  // reference res.
  const G4double tMCP = tMCPns / ns;  // ns
  if (npeMCP >= kMCPThreshold && (nDown || nUp)) {
    G4double tModule = 0.;
    if (nDown && nUp) tModule = 0.5 * (tDown + tUp);
    else tModule = nDown ? tDown : tUp;
    ana->FillH1(radana::kTiming, tModule - tMCP);
  }

  ana->FillH1(radana::kELyso, eLyso / MeV);
  ana->FillH1(radana::kEPb, ePb / MeV);

  // ---- event summary ntuple ---------------------------------------------
  ana->FillNtupleIColumn(radana::kEvent, 0, eid);
  ana->FillNtupleDColumn(radana::kEvent, 1, beamE / MeV);
  ana->FillNtupleDColumn(radana::kEvent, 2, beamX / mm);
  ana->FillNtupleDColumn(radana::kEvent, 3, beamY / mm);
  ana->FillNtupleDColumn(radana::kEvent, 4, eLyso / MeV);
  ana->FillNtupleDColumn(radana::kEvent, 5, ePb / MeV);
  ana->FillNtupleDColumn(radana::kEvent, 6, eCnt / MeV);
  ana->FillNtupleIColumn(radana::kEvent, 7, npeSiPM);
  ana->FillNtupleIColumn(radana::kEvent, 8, npeMCP);
  ana->FillNtupleDColumn(radana::kEvent, 9, tMCP);
  ana->FillNtupleDColumn(radana::kEvent, 10, tDown);
  ana->FillNtupleDColumn(radana::kEvent, 11, tUp);
  ana->FillNtupleDColumn(radana::kEvent, 12, xC / mm);
  ana->FillNtupleDColumn(radana::kEvent, 13, yC / mm);
  ana->AddNtupleRow(radana::kEvent);
}
