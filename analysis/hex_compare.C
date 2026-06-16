//
// hex_compare.C -- compare the FCC-MIT hexagonal cell (hex tile + 7 capillaries)
// to the square module: detected light yield and intrinsic module timing vs E.
//
// Run: root -l -b -q 'analysis/hex_compare.C("results/hex7_timing","results/scan_realistic")'
//
#include <vector>
#include <algorithm>
#include "TFile.h"
#include "TTree.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TSystem.h"

static double rSig(std::vector<double>& v) {        // robust core width (ps)
  if (v.size() < 5) return -1;
  std::sort(v.begin(), v.end());
  double med = v[v.size() / 2];
  std::vector<double> ad; for (double x : v) ad.push_back(std::fabs(x - med));
  std::sort(ad.begin(), ad.end());
  double rs = 1.4826 * ad[ad.size() / 2]; if (rs < 1e-4) rs = 0.05;
  double s = 0, s2 = 0; int n = 0;
  for (double x : v) if (std::fabs(x - med) < 2.5 * rs) { s += x; s2 += x * x; ++n; }
  return n > 1 ? std::sqrt(s2 / n - (s / n) * (s / n)) * 1000. : -1;
}

static void readDir(const char* dir, TGraph* gNpe, TGraph* gSig) {
  int p = 0;
  for (int i = 0; i < 6; ++i) {
    TString fn = Form("%s/radical_run%d.root", dir, i);
    if (gSystem->AccessPathName(fn)) continue;
    TFile* f = TFile::Open(fn); if (!f || f->IsZombie()) continue;
    TTree* e = (TTree*)f->Get("event"); if (!e) { f->Close(); continue; }
    double tD, tU, bE; int npe;
    e->SetBranchAddress("tModuleDown", &tD); e->SetBranchAddress("tModuleUp", &tU);
    e->SetBranchAddress("beamE", &bE); e->SetBranchAddress("npeSiPM", &npe);
    std::vector<double> vMod; double sNpe = 0, sE = 0; int nN = 0;
    for (Long64_t j = 0; j < e->GetEntries(); ++j) {
      e->GetEntry(j); sE += bE; sNpe += npe; ++nN;
      if (tD > 0 && tD < 500 && tU > 0 && tU < 500) vMod.push_back(0.5 * (tD + tU));
    }
    double E = sE / nN / 1000., sig = rSig(vMod);
    if (sig > 0) { gNpe->SetPoint(p, E, sNpe / nN); gSig->SetPoint(p, E, sig); ++p; }
    f->Close();
  }
}

void hex_compare(const char* hexDir = "results/hex7_timing",
                 const char* sqDir = "results/scan_realistic") {
  gStyle->SetOptStat(0);
  auto* gNpeH = new TGraph(); auto* gSigH = new TGraph();
  auto* gNpeS = new TGraph(); auto* gSigS = new TGraph();
  readDir(hexDir, gNpeH, gSigH);
  readDir(sqDir,  gNpeS, gSigS);

  auto* c = new TCanvas("c_hex", "hex vs square", 1100, 500);
  c->Divide(2, 1);

  c->cd(1); gPad->SetLogy();
  gNpeH->SetTitle("Detected light yield;Beam energy E [GeV];n.p.e. (all SiPM)");
  gNpeH->SetMarkerStyle(21); gNpeH->SetMarkerColor(kRed + 1); gNpeH->SetLineColor(kRed + 1);
  gNpeH->SetMarkerSize(1.4); gNpeH->SetMinimum(300);
  gNpeH->Draw("APL");
  gNpeS->SetMarkerStyle(20); gNpeS->SetMarkerColor(kBlue + 1); gNpeS->SetLineColor(kBlue + 1);
  gNpeS->SetMarkerSize(1.4); gNpeS->Draw("PL SAME");
  auto* l1 = new TLegend(0.4, 0.2, 0.88, 0.36); l1->SetBorderSize(0);
  l1->AddEntry(gNpeH, "Hexagon (7 capillaries)", "pl");
  l1->AddEntry(gNpeS, "Square (quincunx-5)", "pl"); l1->Draw();

  c->cd(2);
  gSigH->SetTitle("Intrinsic module timing;Beam energy E [GeV];#sigma(t_{module}) [ps]");
  gSigH->SetMarkerStyle(21); gSigH->SetMarkerColor(kRed + 1); gSigH->SetLineColor(kRed + 1);
  gSigH->SetMarkerSize(1.4); gSigH->SetMinimum(0); gSigH->SetMaximum(50);
  gSigH->Draw("APL");
  gSigS->SetMarkerStyle(20); gSigS->SetMarkerColor(kBlue + 1); gSigS->SetLineColor(kBlue + 1);
  gSigS->SetMarkerSize(1.4); gSigS->Draw("PL SAME");
  auto* l2 = new TLegend(0.4, 0.74, 0.88, 0.9); l2->SetBorderSize(0);
  l2->AddEntry(gSigH, "Hexagon (7 capillaries)", "pl");
  l2->AddEntry(gSigS, "Square (quincunx-5)", "pl"); l2->Draw();

  c->SaveAs("hex_vs_square.png");
  c->SaveAs("hex_vs_square.pdf");
  printf("wrote hex_vs_square.png  (hex points=%d, square points=%d)\n",
         gNpeH->GetN(), gNpeS->GetN());
}
