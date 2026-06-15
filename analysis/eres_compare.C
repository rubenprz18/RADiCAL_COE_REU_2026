//
// eres_compare.C -- overlay single-module vs 3x3-array energy resolution,
//                   with the FCC-hh goal. Shows that transverse containment
//                   improves sigma_E/E (the paper's argument for an array).
//
// Run: root -l -b -q 'analysis/eres_compare.C("results/eres_single","results/eres_array")'
//
#include <vector>
#include <algorithm>
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TF1.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TSystem.h"

static bool fitE(const char* fn, double& E, double& res, double& err) {
  res = 0;
  if (gSystem->AccessPathName(fn)) return false;
  TFile* f = TFile::Open(fn);
  if (!f || f->IsZombie()) return false;
  TTree* ev = (TTree*)f->Get("event");
  if (!ev) { f->Close(); return false; }
  double beamE, eL;
  ev->SetBranchAddress("beamE", &beamE);
  ev->SetBranchAddress("eLYSO", &eL);
  std::vector<double> e; double sE = 0; Long64_t n = ev->GetEntries();
  for (Long64_t i = 0; i < n; ++i) { ev->GetEntry(i); sE += beamE; if (eL > 0) e.push_back(eL); }
  E = (n ? sE / n : 0.) / 1000.;
  if (e.size() < 10) { f->Close(); return false; }
  std::sort(e.begin(), e.end());
  double med = e[e.size() / 2];
  std::vector<double> ad; for (double x : e) ad.push_back(std::fabs(x - med));
  std::sort(ad.begin(), ad.end());
  double rsig = std::max(1.4826 * ad[ad.size() / 2], 1.0), win = std::max(4 * rsig, 10.0);
  static int u = 0; ++u;
  auto* h = new TH1D(Form("h%d", u), "", 60, med - win, med + win);
  for (double x : e) h->Fill(x);
  h->Fit("gaus", "Q");
  TF1* g = h->GetFunction("gaus");
  for (int it = 0; it < 3 && g; ++it) {
    double m = g->GetParameter(1), s = g->GetParameter(2);
    h->Fit("gaus", "Q", "", m - 2 * s, m + 2 * s);
    g = h->GetFunction("gaus");
  }
  if (g) { double mu = g->GetParameter(1), s = g->GetParameter(2);
           if (mu > 0) { res = 100. * s / mu; err = res / std::sqrt(2.0 * e.size()); } }
  return res > 0;
}

static TGraphErrors* curve(const char* dir, int color, int marker) {
  auto* g = new TGraphErrors(); int p = 0;
  for (int i = 0; i < 8; ++i) {
    double E, r, e;
    if (fitE(Form("%s/radical_run%d.root", dir, i), E, r, e)) {
      g->SetPoint(p, E, r); g->SetPointError(p, 0., e); ++p;
    }
  }
  g->SetMarkerStyle(marker); g->SetMarkerSize(1.4);
  g->SetMarkerColor(color); g->SetLineColor(color);
  return g;
}

void eres_compare(const char* dirSingle = "results/eres_single",
                  const char* dirArray = "results/eres_array") {
  gStyle->SetOptStat(0); gStyle->SetOptFit(0);
  auto* gS = curve(dirSingle, kBlue + 1, 20);
  auto* gA = curve(dirArray, kGreen + 2, 21);

  auto* c = new TCanvas("c_cmp", "energy resolution comparison", 800, 600);
  gS->SetTitle("RADiCAL energy resolution: single module vs 3#times3 array;Beam energy E [GeV];#sigma_{E}/E [%]");
  gS->SetMinimum(0.); gS->GetXaxis()->SetLimits(15., 165.);
  gS->Draw("AP");
  gA->Draw("P SAME");

  auto fitf = [&](TGraphErrors* g, int col) {
    auto* f = new TF1(Form("f%d", col), "sqrt([0]*[0]/x + [1]*[1]/(x*x) + [2]*[2])", 20, 160);
    f->SetParameters(30., 1., 1.); f->SetLineColor(col);
    g->Fit(f, "Q"); f->SetRange(18., 162.); f->Draw("SAME"); return f;
  };
  auto* fS = fitf(gS, kBlue + 1);
  auto* fA = fitf(gA, kGreen + 2);

  auto* goal = new TF1("goal", "sqrt([0]*[0]/x + [1]*[1]/(x*x) + [2]*[2])", 20, 160);
  goal->SetParameters(10., 30., 0.7);
  goal->SetLineColor(kRed + 1); goal->SetLineStyle(2);
  goal->Draw("SAME");

  auto* leg = new TLegend(0.36, 0.60, 0.88, 0.88);
  leg->SetBorderSize(0);
  leg->AddEntry(gS, Form("single module:  %.0f%%/#sqrt{E} #oplus %.1f%%", fS->GetParameter(0), fS->GetParameter(2)), "p");
  leg->AddEntry(gA, Form("3#times3 array:  %.1f%%/#sqrt{E} #oplus %.2f%%", fA->GetParameter(0), fA->GetParameter(2)), "p");
  leg->AddEntry(goal, "FCC-hh goal: 10%/#sqrt{E} #oplus 0.3/E #oplus 0.7%", "l");
  leg->Draw();

  c->SaveAs("energy_resolution_compare.png");
  c->SaveAs("energy_resolution_compare.pdf");
  printf("single: a=%.1f%%, c=%.2f%%   |   3x3 array: a=%.1f%%, c=%.2f%%   (goal a=10, c=0.7)\n",
         fS->GetParameter(0), fS->GetParameter(2), fA->GetParameter(0), fA->GetParameter(2));
}
