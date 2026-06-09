//
// RadicalActionInitialization.cc
//
#include "RadicalActionInitialization.hh"
#include "RadicalPrimaryGeneratorAction.hh"
#include "RadicalRunAction.hh"
#include "RadicalEventAction.hh"
#include "RadicalSteppingAction.hh"
#include "RadicalStackingAction.hh"

void RadicalActionInitialization::BuildForMaster() const {
  SetUserAction(new RadicalRunAction());
}

void RadicalActionInitialization::Build() const {
  SetUserAction(new RadicalPrimaryGeneratorAction());
  SetUserAction(new RadicalRunAction());

  auto* eventAction = new RadicalEventAction();
  SetUserAction(eventAction);
  SetUserAction(new RadicalSteppingAction(eventAction));
  SetUserAction(new RadicalStackingAction());
}
