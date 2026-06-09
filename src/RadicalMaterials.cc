//
// RadicalMaterials.cc
//
#include "RadicalMaterials.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Element.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4OpticalSurface.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include <vector>
#include <algorithm>

RadicalMaterials* RadicalMaterials::fInstance = nullptr;
G4double RadicalMaterials::fYieldScale = 1.0;

RadicalMaterials* RadicalMaterials::Instance() {
  if (!fInstance) fInstance = new RadicalMaterials();
  return fInstance;
}

// Convert a list of wavelengths (nm, any order) into photon energies (ascending).
namespace {
std::vector<G4double> EnergiesFromNm(std::vector<G4double> nm) {
  std::sort(nm.begin(), nm.end(), std::greater<G4double>());  // long lambda first
  std::vector<G4double> e;
  e.reserve(nm.size());
  for (G4double l : nm) e.push_back((1239.841984 / l) * eV);  // -> ascending energy
  return e;
}
// Build a Gaussian emission spectrum sampled at the given energies.
std::vector<G4double> Gaussian(const std::vector<G4double>& energy,
                               G4double peakNm, G4double sigmaNm) {
  std::vector<G4double> v;
  v.reserve(energy.size());
  const G4double peakE = (1239.841984 / peakNm) * eV;
  const G4double sE = std::abs((1239.841984 / (peakNm + sigmaNm)) * eV -
                               (1239.841984 / peakNm) * eV);
  for (G4double en : energy) {
    const G4double x = (en - peakE) / sE;
    v.push_back(std::exp(-0.5 * x * x) + 1e-4);
  }
  return v;
}
}  // namespace

void RadicalMaterials::Build() {
  static G4bool built = false;
  if (built) return;   // materials are defined once; geometry may rebuild freely
  BuildBulk();
  BuildOpticalProperties();
  BuildSurfaces();
  built = true;
}

// ---------------------------------------------------------------------------
void RadicalMaterials::BuildBulk() {
  auto* nist = G4NistManager::Instance();

  fWorld   = nist->FindOrBuildMaterial("G4_AIR");
  fTungsten= nist->FindOrBuildMaterial("G4_W");
  fSilicon = nist->FindOrBuildMaterial("G4_Si");
  fPOM     = nist->FindOrBuildMaterial("G4_POLYOXYMETHYLENE");
  fVacuum  = nist->FindOrBuildMaterial("G4_Galactic");
  fPlasticScint = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
  fFR4     = nist->FindOrBuildMaterial("G4_GLASS_PLATE");  // FR4 stand-in

  // Elements
  G4Element* Lu = nist->FindOrBuildElement("Lu");
  G4Element* Y  = nist->FindOrBuildElement("Y");
  G4Element* Si = nist->FindOrBuildElement("Si");
  G4Element* O  = nist->FindOrBuildElement("O");
  G4Element* Al = nist->FindOrBuildElement("Al");
  G4Element* C  = nist->FindOrBuildElement("C");
  G4Element* H  = nist->FindOrBuildElement("H");
  G4Element* Pb = nist->FindOrBuildElement("Pb");
  G4Element* K  = nist->FindOrBuildElement("K");
  G4Element* Na = nist->FindOrBuildElement("Na");
  G4Element* Sb = nist->FindOrBuildElement("Sb");
  G4Element* Cs = nist->FindOrBuildElement("Cs");

  // LYSO:Ce  ~ Lu(1.8) Y(0.2) Si O5, rho = 7.1 g/cm3.
  fLYSO = new G4Material("LYSO_Ce", 7.10 * g / cm3, 4);
  fLYSO->AddElement(Lu, 9);   // approximate stoichiometry (Lu1.8 -> 9/10 scaled)
  fLYSO->AddElement(Y, 1);
  fLYSO->AddElement(Si, 5);
  fLYSO->AddElement(O, 25);

  // Tyvek = high-density polyethylene nonwoven, rho ~ 0.38 g/cm3.
  fTyvek = new G4Material("Tyvek", 0.38 * g / cm3, 2);
  fTyvek->AddElement(C, 1);
  fTyvek->AddElement(H, 2);

  // Fused silica (capillary tube/rod, MCP window).
  fQuartz = new G4Material("FusedSilica", 2.20 * g / cm3, 2);
  fQuartz->AddElement(Si, 1);
  fQuartz->AddElement(O, 2);

  // DSB1 wavelength shifter: polystyrene-based plastic, rho ~ 1.05 g/cm3.
  fDSB1 = new G4Material("DSB1", 1.05 * g / cm3, 2);
  fDSB1->AddElement(C, 8);
  fDSB1->AddElement(H, 8);

  // LuAG:Ce ceramic (E-type filaments, alt. WLS).  Lu3 Al5 O12, rho 6.73.
  fLuAG = new G4Material("LuAG_Ce", 6.73 * g / cm3, 3);
  fLuAG->AddElement(Lu, 3);
  fLuAG->AddElement(Al, 5);
  fLuAG->AddElement(O, 12);

  // Lead glass (SF5-like) for the Cerenkov backing calorimeter, rho 4.08.
  fLeadGlass = new G4Material("LeadGlassSF5", 4.08 * g / cm3, 5);
  fLeadGlass->AddElement(Pb, 0.512);
  fLeadGlass->AddElement(O, 0.270);
  fLeadGlass->AddElement(Si, 0.193);
  fLeadGlass->AddElement(K, 0.018);
  fLeadGlass->AddElement(Na, 0.007);

  // Bialkali photocathode (Sb-K-Cs) thin layer for the MCP / SiPM windows.
  fBialkali = new G4Material("Bialkali", 4.28 * g / cm3, 3);
  fBialkali->AddElement(Sb, 1);
  fBialkali->AddElement(K, 2);
  fBialkali->AddElement(Cs, 1);
}

// ---------------------------------------------------------------------------
void RadicalMaterials::BuildOpticalProperties() {
  // ---- world air: transparent, n = 1 -------------------------------------
  {
    std::vector<G4double> e = EnergiesFromNm({700, 300});
    std::vector<G4double> n(e.size(), 1.000);
    std::vector<G4double> abs(e.size(), 100. * m);
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", e, n);
    mpt->AddProperty("ABSLENGTH", e, abs);
    fWorld->SetMaterialPropertiesTable(mpt);
    fVacuum->SetMaterialPropertiesTable(mpt);
  }

  // ---- LYSO:Ce scintillator ----------------------------------------------
  {
    std::vector<G4double> e = EnergiesFromNm({600, 540, 500, 460, 420, 400, 380, 360});
    std::vector<G4double> rindex(e.size(), 1.82);
    std::vector<G4double> abs(e.size(), 40. * cm);
    std::vector<G4double> emis = Gaussian(e, 420., 25.);  // peak ~420 nm
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", e, rindex);
    mpt->AddProperty("ABSLENGTH", e, abs);
    mpt->AddProperty("SCINTILLATIONCOMPONENT1", e, emis);
    mpt->AddConstProperty("SCINTILLATIONYIELD", fYieldScale * 30000. / MeV);  // ~30 ph/keV
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 40. * ns);
    mpt->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
    fLYSO->SetMaterialPropertiesTable(mpt);
    fLYSO->GetIonisation()->SetBirksConstant(0.008 * mm / MeV);
  }

  // ---- fused silica (transparent, dispersive) ----------------------------
  {
    std::vector<G4double> e = EnergiesFromNm({700, 600, 500, 420, 350, 300});
    std::vector<G4double> n = {1.455, 1.458, 1.462, 1.468, 1.476, 1.487};
    std::vector<G4double> abs(e.size(), 50. * m);
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", e, n);
    mpt->AddProperty("ABSLENGTH", e, abs);
    fQuartz->SetMaterialPropertiesTable(mpt);
  }

  // ---- DSB1 wavelength shifter -------------------------------------------
  // Absorbs ~370-430 nm (blue/UV), emits ~480-560 nm (green), tau = 3.5 ns.
  // WLS absorption must be negligible at wavelengths LONGER than the emission
  // band, otherwise G4OpWLS can try to up-convert red Cerenkov light (fatal).
  // nm are passed descending so the paired arrays line up with ascending energy.
  {
    std::vector<G4double> e = EnergiesFromNm({700, 580, 520, 480, 430, 415, 400, 370});
    std::vector<G4double> rindex(e.size(), 1.59);
    std::vector<G4double> abs(e.size(), 2.0 * m);
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", e, rindex);
    mpt->AddProperty("ABSLENGTH", e, abs);
    // huge (no WLS) in green/red; short only inside the blue absorption band
    std::vector<G4double> wlsAbs = {1.e5 * m, 1.e5 * m, 1.e5 * m, 1.e5 * m,
                                    0.4 * mm, 0.3 * mm, 0.4 * mm, 0.6 * mm};
    mpt->AddProperty("WLSABSLENGTH", e, wlsAbs);
    // emission only at energies below the absorption band (Stokes shift)
    std::vector<G4double> wlsEmit = {0.02, 0.7, 1.0, 0.5, 0.0, 0.0, 0.0, 0.0};
    mpt->AddProperty("WLSCOMPONENT", e, wlsEmit);
    mpt->AddConstProperty("WLSTIMECONSTANT", 3.5 * ns);
    fDSB1->SetMaterialPropertiesTable(mpt);
  }

  // ---- LuAG:Ce (E-type filament / alternative WLS) -----------------------
  // Absorbs ~410-450 nm (blue), emits ~500-540 nm. Same Stokes-shift care as DSB1.
  {
    std::vector<G4double> e = EnergiesFromNm({700, 600, 540, 500, 450, 430, 410});
    std::vector<G4double> rindex(e.size(), 1.84);
    std::vector<G4double> abs(e.size(), 1.5 * m);
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", e, rindex);
    mpt->AddProperty("ABSLENGTH", e, abs);
    std::vector<G4double> wlsAbs = {1.e5 * m, 1.e5 * m, 1.e5 * m, 1.e5 * m,
                                    0.5 * mm, 0.4 * mm, 0.4 * mm};
    mpt->AddProperty("WLSABSLENGTH", e, wlsAbs);
    std::vector<G4double> wlsEmit = {0.05, 0.5, 1.0, 0.6, 0.0, 0.0, 0.0};
    mpt->AddProperty("WLSCOMPONENT", e, wlsEmit);
    mpt->AddConstProperty("WLSTIMECONSTANT", 50. * ns);
    fLuAG->SetMaterialPropertiesTable(mpt);
  }

  // ---- lead glass: Cerenkov radiator -------------------------------------
  {
    std::vector<G4double> e = EnergiesFromNm({700, 600, 500, 420, 350});
    std::vector<G4double> n = {1.667, 1.673, 1.682, 1.700, 1.728};
    std::vector<G4double> abs = {3.*m, 3.*m, 2.5*m, 1.5*m, 0.3*m};
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", e, n);
    mpt->AddProperty("ABSLENGTH", e, abs);
    fLeadGlass->SetMaterialPropertiesTable(mpt);
  }

  // ---- plastic scintillator (A1/A2 trigger counters) ---------------------
  {
    std::vector<G4double> e = EnergiesFromNm({550, 500, 460, 425, 400, 380});
    std::vector<G4double> rindex(e.size(), 1.58);
    std::vector<G4double> abs(e.size(), 1.5 * m);
    std::vector<G4double> emis = Gaussian(e, 425., 20.);
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX", e, rindex);
    mpt->AddProperty("ABSLENGTH", e, abs);
    mpt->AddProperty("SCINTILLATIONCOMPONENT1", e, emis);
    mpt->AddConstProperty("SCINTILLATIONYIELD", fYieldScale * 10000. / MeV);
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.4 * ns);
    mpt->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
    fPlasticScint->SetMaterialPropertiesTable(mpt);
    fPlasticScint->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);
  }
}

// ---------------------------------------------------------------------------
void RadicalMaterials::BuildSurfaces() {
  // Tyvek: diffuse (Lambertian) reflector, ~95% reflectivity.  Modeled as a
  // dielectric_metal skin so photons never enter the Tyvek bulk.
  fTyvekSurf = new G4OpticalSurface("TyvekSurface");
  fTyvekSurf->SetType(dielectric_metal);
  fTyvekSurf->SetFinish(groundfrontpainted);  // Lambertian reflection
  fTyvekSurf->SetModel(unified);
  fTyvekSurf->SetSigmaAlpha(0.20);
  {
    std::vector<G4double> e = EnergiesFromNm({650, 500, 420, 350});
    std::vector<G4double> refl(e.size(), 0.95);
    std::vector<G4double> eff(e.size(), 0.0);
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("REFLECTIVITY", e, refl);
    mpt->AddProperty("EFFICIENCY", e, eff);
    fTyvekSurf->SetMaterialPropertiesTable(mpt);
  }

  // Generic photocathode (SiPM HDR2 / MCP bialkali): detection with a
  // wavelength-dependent quantum / photon-detection efficiency.
  fPhotocathodeSurf = new G4OpticalSurface("Photocathode");
  fPhotocathodeSurf->SetType(dielectric_metal);
  fPhotocathodeSurf->SetFinish(polished);
  fPhotocathodeSurf->SetModel(unified);
  {
    // PDE/QE curve peaking ~40% around 450-470 nm (SiPM) / ~25% for MCP UV.
    std::vector<G4double> e = EnergiesFromNm({650, 600, 550, 500, 470, 450, 420, 400, 350});
    std::vector<G4double> eff = {0.05, 0.12, 0.25, 0.35, 0.40, 0.40, 0.35, 0.28, 0.15};
    std::vector<G4double> refl(e.size(), 0.0);  // absorb (detect or lose)
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("EFFICIENCY", e, eff);
    mpt->AddProperty("REFLECTIVITY", e, refl);
    fPhotocathodeSurf->SetMaterialPropertiesTable(mpt);
  }

  // Tungsten / black absorber: low reflectivity polished metal.
  fAbsorberSurf = new G4OpticalSurface("Absorber");
  fAbsorberSurf->SetType(dielectric_metal);
  fAbsorberSurf->SetFinish(polished);
  fAbsorberSurf->SetModel(unified);
  {
    std::vector<G4double> e = EnergiesFromNm({650, 420, 350});
    std::vector<G4double> refl(e.size(), 0.20);
    std::vector<G4double> eff(e.size(), 0.0);
    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("REFLECTIVITY", e, refl);
    mpt->AddProperty("EFFICIENCY", e, eff);
    fAbsorberSurf->SetMaterialPropertiesTable(mpt);
  }
}
