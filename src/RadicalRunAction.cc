//
// RadicalRunAction.cc
//
#include "RadicalRunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

RadicalRunAction::RadicalRunAction() {
  auto* ana = G4AnalysisManager::Instance();
  ana->SetDefaultFileType("root");
  ana->SetVerboseLevel(1);
  ana->SetNtupleMerging(true);

  // ---- histograms --------------------------------------------------------
  ana->CreateH1("hLong", "EM shower: energy per layer;LYSO layer;E [MeV]",
                29, -0.5, 28.5);                                   // kLong
  ana->CreateH1("hSiPMnpe", "Detected photoelectrons (all SiPM);n.p.e.;events",
                200, 0., 2000.);                                   // kSiPMnpe
  ana->CreateH1("hTiming", "t_module - t_MCP;#Deltat [ns];events",
                200, -2., 2.);                                     // kTiming
  ana->CreateH1("hELyso", "Total LYSO energy;E [MeV];events", 200, 0., 5000.);
  ana->CreateH1("hEPb", "Pb-glass leakage energy;E [MeV];events", 200, 0., 5000.);

  // shower centroid at shower max (Fig. 9)
  ana->CreateH2("hCentroid", "Shower centroid at shower max;x [mm];y [mm]",
                100, -10., 10., 100, -10., 10.);                   // kCentroid

  // ---- ntuples -----------------------------------------------------------
  // 0: per-event summary
  ana->CreateNtuple("event", "per-event summary");
  ana->CreateNtupleIColumn("eventID");
  ana->CreateNtupleDColumn("beamE");      // MeV
  ana->CreateNtupleDColumn("beamX");      // mm
  ana->CreateNtupleDColumn("beamY");
  ana->CreateNtupleDColumn("eLYSO");      // MeV total in LYSO
  ana->CreateNtupleDColumn("ePbGlass");   // MeV leakage
  ana->CreateNtupleDColumn("eCounters");  // MeV
  ana->CreateNtupleIColumn("npeSiPM");    // total detected p.e. in module
  ana->CreateNtupleIColumn("npeMCP");
  ana->CreateNtupleDColumn("tMCP");       // ns, first MCP photon
  ana->CreateNtupleDColumn("tModuleDown");// ns, avg downstream SiPM first time
  ana->CreateNtupleDColumn("tModuleUp");  // ns, avg upstream SiPM first time
  ana->CreateNtupleDColumn("xCentroid");  // mm at shower max
  ana->CreateNtupleDColumn("yCentroid");
  ana->FinishNtuple();

  // 1: per-tile energy (shower profiles, Fig. 7/9)
  ana->CreateNtuple("tile", "per-LYSO-tile energy");
  ana->CreateNtupleIColumn("eventID");
  ana->CreateNtupleIColumn("module");
  ana->CreateNtupleIColumn("layer");
  ana->CreateNtupleDColumn("edep");       // MeV
  ana->CreateNtupleDColumn("t");          // ns
  ana->CreateNtupleDColumn("x");          // mm centroid
  ana->CreateNtupleDColumn("y");
  ana->FinishNtuple();

  // 2: per-SiPM-channel photon readout
  ana->CreateNtuple("channel", "per-SiPM-channel readout");
  ana->CreateNtupleIColumn("eventID");
  ana->CreateNtupleIColumn("module");
  ana->CreateNtupleIColumn("channel");    // 0-3 corners, +50 = upstream end
  ana->CreateNtupleIColumn("npe");
  ana->CreateNtupleDColumn("tHit");       // ns, k-th p.e. threshold time
  ana->FinishNtuple();
}

RadicalRunAction::~RadicalRunAction() = default;

void RadicalRunAction::BeginOfRunAction(const G4Run* run) {
  auto* ana = G4AnalysisManager::Instance();
  G4String fn = fFileName + "_run" + std::to_string(run->GetRunID());
  ana->OpenFile(fn);
}

void RadicalRunAction::EndOfRunAction(const G4Run*) {
  auto* ana = G4AnalysisManager::Instance();
  ana->Write();
  ana->CloseFile();
}
