//
// RadicalPrimaryGeneratorAction.cc
//
#include "RadicalPrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4GenericMessenger.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"
#include "G4SystemOfUnits.hh"

RadicalPrimaryGeneratorAction::RadicalPrimaryGeneratorAction() {
  fGun = new G4ParticleGun(1);
  auto* pt = G4ParticleTable::GetParticleTable();
  fGun->SetParticleDefinition(pt->FindParticle("e-"));
  DefineCommands();
}

RadicalPrimaryGeneratorAction::~RadicalPrimaryGeneratorAction() {
  delete fGun;
  delete fMessenger;
}

void RadicalPrimaryGeneratorAction::GeneratePrimaries(G4Event* evt) {
  auto* pt = G4ParticleTable::GetParticleTable();
  if (auto* def = pt->FindParticle(fParticle)) fGun->SetParticleDefinition(def);

  // transverse spot
  const G4double x = G4RandGauss::shoot(fMeanX, fSigmaX) * mm;
  const G4double y = G4RandGauss::shoot(fMeanY, fSigmaY) * mm;
  fGun->SetParticlePosition(G4ThreeVector(x, y, fZstart * mm));

  // momentum spread
  G4double E = fEnergy * GeV;
  if (fSigmaEfrac > 0.)
    E = G4RandGauss::shoot(fEnergy, fEnergy * fSigmaEfrac) * GeV;
  if (E < 1. * MeV) E = 1. * MeV;
  fGun->SetParticleEnergy(E);

  // angular divergence
  G4double tx = 0., ty = 0.;
  if (fDivergence > 0.) {
    tx = G4RandGauss::shoot(0., fDivergence) * mrad;
    ty = G4RandGauss::shoot(0., fDivergence) * mrad;
  }
  G4ThreeVector dir(std::sin(tx), std::sin(ty), 1.0);
  fGun->SetParticleMomentumDirection(dir.unit());

  fGun->GeneratePrimaryVertex(evt);
}

void RadicalPrimaryGeneratorAction::DefineCommands() {
  fMessenger =
      new G4GenericMessenger(this, "/radical/beam/", "Beam configuration");

  fMessenger->DeclareProperty("energyGeV", fEnergy, "Beam kinetic energy [GeV]");
  fMessenger->DeclareProperty("particle", fParticle,
                              "Beam particle name (e-, e+, gamma, pi-, ...)");
  fMessenger->DeclareProperty("sigmaEfrac", fSigmaEfrac,
                              "Fractional momentum spread");
  fMessenger->DeclareProperty("meanX", fMeanX, "Beam centre x [mm]");
  fMessenger->DeclareProperty("meanY", fMeanY, "Beam centre y [mm]");
  fMessenger->DeclareProperty("sigmaX", fSigmaX, "Spot sigma x [mm]");
  fMessenger->DeclareProperty("sigmaY", fSigmaY, "Spot sigma y [mm]");
  fMessenger->DeclareProperty("divergence", fDivergence,
                              "Angular divergence sigma [mrad]");
  fMessenger->DeclareProperty("zStart", fZstart, "Beam start z [mm]");
}
