//
// RadicalCaloHit.hh
//
// One calorimeter cell's accumulated energy for an event.  A "cell" is a LYSO
// tile (id = module*1000 + layer), a Pb-glass block (id = block) or a trigger
// counter (id = counter index).
//
#ifndef RADICAL_CALO_HIT_HH
#define RADICAL_CALO_HIT_HH

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"

class RadicalCaloHit : public G4VHit {
 public:
  explicit RadicalCaloHit(G4int id) : fId(id) {}
  ~RadicalCaloHit() override = default;

  inline void* operator new(size_t);
  inline void operator delete(void*);

  void Add(G4double edep, G4double time, const G4ThreeVector& pos) {
    fEdep += edep;
    fWeightedPos += edep * pos;
    if (time < fTime) fTime = time;   // earliest energy deposit
  }

  G4int         Id()   const { return fId; }
  G4double      Edep() const { return fEdep; }
  G4double      Time() const { return fTime; }
  G4ThreeVector Centroid() const {
    return (fEdep > 0.) ? fWeightedPos / fEdep : G4ThreeVector();
  }

 private:
  G4int         fId = 0;
  G4double      fEdep = 0.;
  G4double      fTime = 1.e9;
  G4ThreeVector fWeightedPos;
};

using RadicalCaloHitsCollection = G4THitsCollection<RadicalCaloHit>;

extern G4ThreadLocal G4Allocator<RadicalCaloHit>* RadicalCaloHitAllocator;

inline void* RadicalCaloHit::operator new(size_t) {
  if (!RadicalCaloHitAllocator)
    RadicalCaloHitAllocator = new G4Allocator<RadicalCaloHit>;
  return RadicalCaloHitAllocator->MallocSingle();
}
inline void RadicalCaloHit::operator delete(void* h) {
  RadicalCaloHitAllocator->FreeSingle(static_cast<RadicalCaloHit*>(h));
}

#endif  // RADICAL_CALO_HIT_HH
