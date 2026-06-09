//
// RadicalPrimaryGeneratorAction.hh
//
// CERN H2-style electron beam: a particle gun with a finite transverse spot,
// angular divergence and momentum spread.  Configurable via /radical/beam/...
//
#ifndef RADICAL_PRIMARY_GENERATOR_ACTION_HH
#define RADICAL_PRIMARY_GENERATOR_ACTION_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4GenericMessenger;

class RadicalPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
 public:
  RadicalPrimaryGeneratorAction();
  ~RadicalPrimaryGeneratorAction() override;

  void GeneratePrimaries(G4Event*) override;

 private:
  void DefineCommands();

  G4ParticleGun* fGun = nullptr;
  G4GenericMessenger* fMessenger = nullptr;

  G4double fEnergy   = 50.;           // beam kinetic energy [GeV]
  G4double fSigmaEfrac = 0.01;        // fractional momentum spread
  G4double fMeanX = 0., fMeanY = 0.;  // mm
  G4double fSigmaX = 3., fSigmaY = 3.;// mm spot size
  G4double fDivergence = 0.;          // mrad
  G4double fZstart = -700.;           // mm (upstream of all detectors)
  G4String fParticle = "e-";
};

#endif  // RADICAL_PRIMARY_GENERATOR_ACTION_HH
