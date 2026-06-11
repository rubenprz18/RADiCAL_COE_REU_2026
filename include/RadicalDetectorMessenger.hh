//
// RadicalDetectorMessenger.hh
//
// UI commands (/radical/geo/... and /radical/optical/...) that drive the
// parametrized geometry and the optical-photon runtime controls.
//
#ifndef RADICAL_DETECTOR_MESSENGER_HH
#define RADICAL_DETECTOR_MESSENGER_HH

#include "G4UImessenger.hh"
#include "globals.hh"

class RadicalDetectorConstruction;
class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithAnInteger;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithADouble;
class G4UIcmdWithABool;
class G4UIcmdWithoutParameter;

class RadicalDetectorMessenger : public G4UImessenger {
 public:
  explicit RadicalDetectorMessenger(RadicalDetectorConstruction* det);
  ~RadicalDetectorMessenger() override;

  void SetNewValue(G4UIcommand* cmd, G4String val) override;

 private:
  RadicalDetectorConstruction* fDet;

  G4UIdirectory* fGeoDir = nullptr;
  G4UIdirectory* fOptDir = nullptr;

  G4UIcmdWithAString*        fPreset = nullptr;
  G4UIcmdWithAString*        fShape = nullptr;
  G4UIcmdWithAString*        fLayout = nullptr;
  G4UIcmdWithAString*        fTiling = nullptr;
  G4UIcmdWithAnInteger*      fNx = nullptr;
  G4UIcmdWithAnInteger*      fNy = nullptr;
  G4UIcmdWithADoubleAndUnit* fSizeX = nullptr;
  G4UIcmdWithADoubleAndUnit* fSizeY = nullptr;
  G4UIcmdWithADoubleAndUnit* fHexR = nullptr;
  G4UIcmdWithADoubleAndUnit* fTyvek = nullptr;
  G4UIcmdWithABool*          fBeamline = nullptr;
  G4UIcmdWithABool*          fMCP = nullptr;
  G4UIcmdWithABool*          fPbGlass = nullptr;
  G4UIcmdWithABool*          fCounters = nullptr;
  G4UIcmdWithABool*          fCheckOverlaps = nullptr;
  G4UIcmdWithoutParameter*   fUpdate = nullptr;

  G4UIcmdWithADouble*        fYieldScale = nullptr;
  G4UIcmdWithAnInteger*      fMaxPhotons = nullptr;
  G4UIcmdWithADoubleAndUnit* fMcpRes = nullptr;
  G4UIcmdWithADoubleAndUnit* fSipmJit = nullptr;
};

#endif  // RADICAL_DETECTOR_MESSENGER_HH
