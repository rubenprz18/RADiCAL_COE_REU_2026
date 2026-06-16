//
// RadicalGeometryConfig.cc
//
#include "RadicalGeometryConfig.hh"
#include <cmath>

void RadicalGeometryConfig::ApplyPreset(ModulePreset preset) {
  switch (preset) {
    case ModulePreset::Current14:
      // Current prototype under test (Section 2, Fig. 1/2).
      nLysoPlates   = 29;
      lysoThickness = 1.50 * CLHEP::mm;
      nWPlates      = 28;
      wThickness    = 2.50 * CLHEP::mm;
      tileSizeX = tileSizeY = 14.0 * CLHEP::mm;
      hexCircumradius = 14.0 * CLHEP::mm / std::sqrt(3.0);  // 14 mm flat-to-flat
      layout = CapillaryLayout::Quincunx5;
      break;

    case ModulePreset::Enhanced18:
      // Enhanced design (Section 7, Fig. 28): larger 18x18 cross section,
      // 9 capillaries (5 T-type + 4 E-type) in a 3x3 grid.
      nLysoPlates   = 29;
      lysoThickness = 1.50 * CLHEP::mm;
      nWPlates      = 28;
      wThickness    = 2.50 * CLHEP::mm;
      tileSizeX = tileSizeY = 18.0 * CLHEP::mm;
      hexCircumradius = 18.0 * CLHEP::mm / std::sqrt(3.0);  // 18 mm flat-to-flat
      layout = CapillaryLayout::Grid9;
      break;
  }
  RebuildCapillaries();
}

void RadicalGeometryConfig::RebuildCapillaries() {
  capillaries.clear();

  // Corner offset for the 5-hole quincunx (Fig. 2): holes sit 3.50 mm from the
  // tile centre in x and y.  Scale this with the tile size for other presets.
  const G4double cornerOffset = (tileSizeX <= 15.0 * CLHEP::mm)
                                    ? 3.50 * CLHEP::mm
                                    : 0.25 * tileSizeX;   // ~4.5 mm at 18 mm

  auto addCap = [&](G4double x, G4double y, CapType type,
                    G4double holeD, G4double odD) {
    CapillarySpec c;
    c.x = x;
    c.y = y;
    c.type = type;
    c.holeDiameter = holeD;
    c.outerDiameter = odD;
    c.coreDiameter = 0.95 * CLHEP::mm;
    c.filamentDiameter = 0.90 * CLHEP::mm;
    // The central hole in the current module is the smaller 0.9 mm bore.
    if (std::abs(x) < 1e-6 && std::abs(y) < 1e-6 &&
        layout == CapillaryLayout::Quincunx5) {
      c.outerDiameter = 0.85 * CLHEP::mm;
      c.coreDiameter = 0.70 * CLHEP::mm;
      c.filamentDiameter = 0.65 * CLHEP::mm;
    }
    capillaries.push_back(c);
  };

  const G4double corHole = 1.30 * CLHEP::mm;   // 4 x phi 1.3 mm corner holes
  const G4double corOD   = 1.15 * CLHEP::mm;   // 1150 um capillary OD
  const G4double cenHole = 0.90 * CLHEP::mm;   // 1 x phi 0.9 mm centre hole

  if (layout == CapillaryLayout::Hex7) {
    // FCC-MIT-2024 proposed hexagonal cell: a centre capillary + 6 in a
    // hexagonal ring, alternating T-type (timing) and E-type (energy).
    // Ring radius is kept inside the ~4.5 mm shower-max region.
    const G4double ringR = (tileSizeX <= 15.0 * CLHEP::mm)
                               ? 4.0 * CLHEP::mm
                               : 0.28 * tileSizeX;
    addCap(0., 0., CapType::TType, corHole, corOD);  // centre (read out)
    for (int k = 0; k < 6; ++k) {
      const G4double a = (60.0 * k) * CLHEP::deg;     // 0,60,...,300 deg
      const CapType t = (k % 2 == 0) ? CapType::TType : CapType::EType;
      addCap(ringR * std::cos(a), ringR * std::sin(a), t, corHole, corOD);
    }
  } else if (layout == CapillaryLayout::Quincunx5) {
    addCap(-cornerOffset, +cornerOffset, CapType::TType, corHole, corOD);
    addCap(+cornerOffset, +cornerOffset, CapType::TType, corHole, corOD);
    addCap(-cornerOffset, -cornerOffset, CapType::TType, corHole, corOD);
    addCap(+cornerOffset, -cornerOffset, CapType::TType, corHole, corOD);
    addCap(0., 0., CapType::Calibration, cenHole, cenHole);  // centre (calib)
  } else {  // Grid9 (Fig. 28)
    // 3x3 grid.  Default assignment = Fig. 28 left variant:
    //   corners  -> T-type, edge midpoints -> E-type, centre -> T-type.
    const G4double s = cornerOffset;  // grid spacing from centre
    const G4double gx[3] = {-s, 0., +s};
    const G4double gy[3] = {-s, 0., +s};
    for (int iy = 0; iy < 3; ++iy) {
      for (int ix = 0; ix < 3; ++ix) {
        const bool isCentre = (ix == 1 && iy == 1);
        const bool isEdge = ((ix == 1) ^ (iy == 1));  // edge midpoint
        CapType t = CapType::TType;
        if (isEdge) t = CapType::EType;
        if (isCentre) t = CapType::Calibration;
        addCap(gx[ix], gy[iy], t, corHole, corOD);
      }
    }
  }
}
