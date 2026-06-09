//
// radical.cc  --  main program for the RADiCAL GEANT4 simulation
//
// Usage:
//   ./radical                 # interactive session with visualization
//   ./radical macros/run.mac  # batch mode, executes the macro
//
#include "RadicalDetectorConstruction.hh"
#include "RadicalActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "G4PhysListFactory.hh"
#include "G4OpticalPhysics.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

int main(int argc, char** argv) {
  // Interactive if no macro argument is given.
  G4UIExecutive* ui = nullptr;
  if (argc == 1) ui = new G4UIExecutive(argc, argv);

  CLHEP::HepRandom::setTheSeed(time(nullptr));

  auto* runManager =
      G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  // --- geometry ---
  runManager->SetUserInitialization(new RadicalDetectorConstruction());

  // --- physics: FTFP_BERT for EM showers + full optical physics ---
  G4PhysListFactory factory;
  G4VModularPhysicsList* physics = factory.GetReferencePhysList("FTFP_BERT");
  auto* optical = new G4OpticalPhysics();
  physics->RegisterPhysics(optical);
  runManager->SetUserInitialization(physics);

  // --- user actions ---
  runManager->SetUserInitialization(new RadicalActionInitialization());

  // --- visualization ("quiet" suppresses the long driver-registration dump) ---
  auto* visManager = new G4VisExecutive("quiet");
  visManager->Initialize();

  auto* uiManager = G4UImanager::GetUIpointer();

  if (ui) {  // interactive
    uiManager->ApplyCommand("/control/execute macros/init_vis.mac");
    ui->SessionStart();
    delete ui;
  } else {  // batch
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    uiManager->ApplyCommand(command + fileName);
  }

  delete visManager;
  delete runManager;
  return 0;
}
