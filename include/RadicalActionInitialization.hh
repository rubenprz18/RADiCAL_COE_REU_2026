//
// RadicalActionInitialization.hh
//
#ifndef RADICAL_ACTION_INITIALIZATION_HH
#define RADICAL_ACTION_INITIALIZATION_HH

#include "G4VUserActionInitialization.hh"

class RadicalActionInitialization : public G4VUserActionInitialization {
 public:
  RadicalActionInitialization() = default;
  ~RadicalActionInitialization() override = default;

  void BuildForMaster() const override;
  void Build() const override;
};

#endif  // RADICAL_ACTION_INITIALIZATION_HH
