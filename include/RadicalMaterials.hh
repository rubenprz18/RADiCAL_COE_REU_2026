//
// RadicalMaterials.hh
//
// Builds every material used in the simulation together with the optical
// property tables required for full optical-photon tracking (scintillation,
// wavelength shifting, Cerenkov, reflection and photodetection).
//
// Values are representative literature numbers for the RADiCAL components
// (LYSO:Ce, DSB1, LuAG:Ce, fused silica, lead glass, Tyvek, ...).  They are
// collected here, with comments, so they are easy to refine against bench data.
//
#ifndef RADICAL_MATERIALS_HH
#define RADICAL_MATERIALS_HH

#include "globals.hh"

class G4Material;
class G4OpticalSurface;

class RadicalMaterials {
 public:
  static RadicalMaterials* Instance();
  void Build();                         // construct everything once

  // Global multiplier on scintillation light yields.  Full optical tracking of
  // EM showers at 25-150 GeV is only tractable with a reduced yield (the photon
  // count then scales the result back).  Set < 1 for production runs.
  static void   SetYieldScale(G4double s) { fYieldScale = s; }
  static G4double YieldScale() { return fYieldScale; }

  // ---- bulk materials -----------------------------------------------------
  G4Material* World()        const { return fWorld; }
  G4Material* LYSO()         const { return fLYSO; }
  G4Material* Tungsten()     const { return fTungsten; }
  G4Material* Tyvek()        const { return fTyvek; }
  G4Material* Quartz()       const { return fQuartz; }
  G4Material* DSB1()         const { return fDSB1; }
  G4Material* LuAG()         const { return fLuAG; }
  G4Material* POM()          const { return fPOM; }
  G4Material* LeadGlass()    const { return fLeadGlass; }
  G4Material* PlasticScint() const { return fPlasticScint; }
  G4Material* Silicon()      const { return fSilicon; }
  G4Material* Bialkali()     const { return fBialkali; }
  G4Material* FR4()          const { return fFR4; }
  G4Material* Vacuum()       const { return fVacuum; }

  // ---- shared optical surfaces -------------------------------------------
  G4OpticalSurface* TyvekSurface()       const { return fTyvekSurf; }   // diffuse reflector
  G4OpticalSurface* PhotocathodeSurface()const { return fPhotocathodeSurf; } // detection
  G4OpticalSurface* PolishedAbsorber()   const { return fAbsorberSurf; } // W / black

 private:
  RadicalMaterials() = default;

  void BuildBulk();
  void BuildOpticalProperties();
  void BuildSurfaces();

  G4Material *fWorld = nullptr, *fLYSO = nullptr, *fTungsten = nullptr,
             *fTyvek = nullptr, *fQuartz = nullptr, *fDSB1 = nullptr,
             *fLuAG = nullptr, *fPOM = nullptr, *fLeadGlass = nullptr,
             *fPlasticScint = nullptr, *fSilicon = nullptr,
             *fBialkali = nullptr, *fFR4 = nullptr, *fVacuum = nullptr;

  G4OpticalSurface *fTyvekSurf = nullptr, *fPhotocathodeSurf = nullptr,
                   *fAbsorberSurf = nullptr;

  static RadicalMaterials* fInstance;
  static G4double fYieldScale;
};

#endif  // RADICAL_MATERIALS_HH
