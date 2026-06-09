//
// analyze.C  --  ROOT analysis of the RADiCAL energy scan.
//
// Reproduces:
//   (1) Fig. 7  : fraction of EM shower energy per LYSO:Ce tile (layer), vs
//                 beam energy (longitudinal shower profile).
//   (2) the timing-resolution fit  sigma_t = a/sqrt(E) (+) b  from the spread
//                 of  t_module - t_MCP  in each energy run.
//
// Usage:
//   root -l -b -q 'analysis/analyze.C("build/scan_out")'
//
// Input: radical_run0.root ... radical_run5.root, one per beam energy, each
// holding the `event` and `tile` ntuples written by the simulation.
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
#include "TLatex.h"
#include "TStyle.h"
#include "TMath.h"
#include "TSystem.h"

// The scintillation yield scale used during the scan (see timing_scan.mac).
// The stochastic timing term scales ~ 1/sqrt(yield), so a_physical ~ a_fit *
// sqrt(YIELD_SCALE) is a rough extrapolation to the full LYSO light yield.
static const double YIELD_SCALE = 0.002;
static const int NLAYER = 29;

struct RunResult {
  double E;        // GeV
  double sigma;    // ps
  double sigmaErr; // ps
};

// ---- locate the available run files -------------------------------------
static std::vector<TString> FindFiles(const TString& dir) {
  std::vector<TString> files;
  for (int i = 0; i < 12; ++i) {
    TString fn = Form("%s/radical_run%d.root", dir.Data(), i);
    if (!gSystem->AccessPathName(fn)) files.push_back(fn);
  }
  return files;
}

// =========================================================================
// (1) Fig. 7 : longitudinal shower profile
// =========================================================================
static void Fig7(const std::vector<TString>& files) {
  gStyle->SetOptStat(0);
  auto* c = new TCanvas("c_fig7", "RADiCAL Fig.7 longitudinal profile", 800, 600);
  auto* leg = new TLegend(0.60, 0.62, 0.88, 0.88);
  leg->SetBorderSize(0);
  leg->SetHeader("Beam energy");

  int colors[] = {kRed+1, kOrange+7, kGreen+2, kAzure+1, kBlue+1, kViolet+1};
  double ymax = 0.;
  std::vector<TH1D*> profiles;

  for (size_t f = 0; f < files.size(); ++f) {
    TFile* file = TFile::Open(files[f]);
    if (!file || file->IsZombie()) continue;
    TTree* ev = (TTree*)file->Get("event");
    TTree* ti = (TTree*)file->Get("tile");
    if (!ev || !ti) { file->Close(); continue; }

    // mean beam energy (GeV)
    double beamE; ev->SetBranchAddress("beamE", &beamE);
    double sumE = 0.; Long64_t nev = ev->GetEntries();
    for (Long64_t i = 0; i < nev; ++i) { ev->GetEntry(i); sumE += beamE; }
    double EGeV = (nev ? sumE / nev : 0.) / 1000.;

    // sum of edep per layer and total LYSO energy
    int layer; double edep;
    ti->SetBranchAddress("layer", &layer);
    ti->SetBranchAddress("edep", &edep);
    std::vector<double> layerSum(NLAYER, 0.);
    double total = 0.;
    Long64_t nrow = ti->GetEntries();
    for (Long64_t i = 0; i < nrow; ++i) {
      ti->GetEntry(i);
      if (layer >= 0 && layer < NLAYER) layerSum[layer] += edep;
      total += edep;
    }

    auto* h = new TH1D(Form("hprof%zu", f), "", NLAYER, -0.5, NLAYER - 0.5);
    for (int L = 0; L < NLAYER; ++L)
      h->SetBinContent(L + 1, total > 0 ? layerSum[L] / total : 0.);
    h->SetLineColor(colors[f % 6]);
    h->SetLineWidth(2);
    h->SetMarkerColor(colors[f % 6]);
    h->SetMarkerStyle(20 + (int)(f % 6));
    h->SetDirectory(nullptr);
    ymax = std::max(ymax, h->GetMaximum());
    profiles.push_back(h);
    leg->AddEntry(h, Form("%.0f GeV", EGeV), "lp");
    file->Close();
  }

  bool first = true;
  for (auto* h : profiles) {
    h->SetTitle("EM shower energy fraction per LYSO tile;LYSO layer (beam from layer 0);fraction of LYSO energy / layer");
    h->SetMaximum(1.25 * ymax);
    h->Draw(first ? "LP" : "LP SAME");
    first = false;
  }
  leg->Draw();
  TLatex tx; tx.SetNDC(); tx.SetTextSize(0.030); tx.SetTextColor(kGray+2);
  tx.DrawLatex(0.12, 0.91, "RADiCAL GEANT4 (this work) - cf. Fig. 7");
  c->SaveAs("fig7_longitudinal.png");
  c->SaveAs("fig7_longitudinal.pdf");
  printf("[Fig7] wrote fig7_longitudinal.png/.pdf  (%zu energies)\n", profiles.size());
}

// =========================================================================
// (2) timing resolution sigma_t = a/sqrt(E) (+) b
// =========================================================================
static RunResult FitOneTiming(TFile* file, TCanvas* c, int pad) {
  RunResult r{0, 0, 0};
  TTree* ev = (TTree*)file->Get("event");
  if (!ev) return r;

  double beamE, tDown, tUp, tMCP;
  ev->SetBranchAddress("beamE", &beamE);
  ev->SetBranchAddress("tModuleDown", &tDown);
  ev->SetBranchAddress("tModuleUp", &tUp);
  ev->SetBranchAddress("tMCP", &tMCP);

  // first pass: collect valid dt to set a sensible window.
  // Undetected channels carry a 1e9 ns sentinel from the simulation, so require
  // all three times to be physical (a few -- tens of ns).
  const double TMAX = 500.;  // ns
  std::vector<double> dts;
  double sumE = 0.; Long64_t nev = ev->GetEntries();
  for (Long64_t i = 0; i < nev; ++i) {
    ev->GetEntry(i);
    sumE += beamE;
    if (tDown > 0 && tDown < TMAX && tUp > 0 && tUp < TMAX &&
        tMCP > 0 && tMCP < TMAX) {
      double tModule = 0.5 * (tDown + tUp);
      dts.push_back(tModule - tMCP);   // ns
    }
  }
  r.E = (nev ? sumE / nev : 0.) / 1000.;
  if (dts.size() < 10) return r;

  // Robust window: use the median and the MAD (median absolute deviation) so a
  // few outlier events (poor module-time estimate, low p.e.) don't blow up the
  // Gaussian fit. robust sigma = 1.4826 * MAD.
  std::sort(dts.begin(), dts.end());
  double median = dts[dts.size() / 2];
  std::vector<double> ad;
  ad.reserve(dts.size());
  for (double d : dts) ad.push_back(std::fabs(d - median));
  std::sort(ad.begin(), ad.end());
  double mad = ad[ad.size() / 2];
  double rsig = std::max(1.4826 * mad, 0.02);   // ns, floor 20 ps

  // keep events within median +/- 4 robust-sigma
  std::vector<double> core;
  for (double d : dts)
    if (std::fabs(d - median) < 4 * rsig) core.push_back(d);
  if (core.size() < 8) core = dts;

  double win = std::max(4 * rsig, 0.10);
  auto* h = new TH1D(Form("hdt_%d", pad),
                     Form("%.0f GeV;t_{module}-t_{MCP} [ns];events", r.E),
                     50, median - win, median + win);
  for (double d : core) h->Fill(d);

  // iterative Gaussian fit within +/-2 sigma
  h->Fit("gaus", "QR");
  TF1* g = h->GetFunction("gaus");
  for (int it = 0; it < 3 && g; ++it) {
    double m = g->GetParameter(1), s = g->GetParameter(2);
    h->Fit("gaus", "Q", "", m - 2 * s, m + 2 * s);
    g = h->GetFunction("gaus");
  }
  if (g) {
    r.sigma = g->GetParameter(2) * 1000.;     // ns -> ps
    r.sigmaErr = g->GetParError(2) * 1000.;
  }
  if (c) { c->cd(pad); h->Draw(); }
  return r;
}

static void Timing(const std::vector<TString>& files) {
  gStyle->SetOptStat(0);
  gStyle->SetOptFit(0);

  auto* cdt = new TCanvas("c_dt", "dt distributions", 1200, 700);
  cdt->Divide(3, 2);

  std::vector<RunResult> res;
  int pad = 1;
  for (auto& fn : files) {
    TFile* file = TFile::Open(fn);
    if (!file || file->IsZombie()) continue;
    RunResult r = FitOneTiming(file, cdt, pad++);
    if (r.sigma > 0) res.push_back(r);
    // keep file open so histograms drawn on the canvas survive until SaveAs
  }
  cdt->SaveAs("timing_dt_distributions.png");

  if (res.size() < 2) { printf("[Timing] too few points to fit\n"); return; }

  std::sort(res.begin(), res.end(),
            [](const RunResult& a, const RunResult& b) { return a.E < b.E; });

  auto* g = new TGraphErrors();
  for (size_t i = 0; i < res.size(); ++i) {
    g->SetPoint(i, res[i].E, res[i].sigma);
    g->SetPointError(i, 0., res[i].sigmaErr);
  }

  auto* c = new TCanvas("c_timing", "RADiCAL timing resolution", 800, 600);
  g->SetTitle("RADiCAL timing resolution;Beam energy E [GeV];#sigma_{t} [ps]");
  g->SetMarkerStyle(20);
  g->SetMarkerSize(1.3);
  g->SetMarkerColor(kBlue + 1);
  g->SetLineColor(kBlue + 1);
  g->Draw("AP");

  // fit  sigma = sqrt( a^2/E + b^2 )
  auto* fit = new TF1("fit", "sqrt([0]*[0]/x + [1]*[1])", 20, 160);
  fit->SetParameters(300., 20.);
  fit->SetParNames("a", "b");
  fit->SetLineColor(kBlue + 1);
  g->Fit(fit, "Q");
  double a = fit->GetParameter(0), b = fit->GetParameter(1);
  double aErr = fit->GetParError(0), bErr = fit->GetParError(1);

  // paper reference curve a = 256 ps*sqrt(GeV), b = 17.5 ps
  auto* paper = new TF1("paper", "sqrt([0]*[0]/x + [1]*[1])", 20, 160);
  paper->SetParameters(256., 17.5);
  paper->SetLineColor(kRed + 1);
  paper->SetLineStyle(2);
  paper->Draw("SAME");

  double aPhys = a * sqrt(YIELD_SCALE);   // rough full-yield extrapolation

  auto* leg = new TLegend(0.40, 0.65, 0.88, 0.88);
  leg->SetBorderSize(0);
  leg->AddEntry(g, "GEANT4 (this work)", "p");
  leg->AddEntry(fit, Form("fit: a=%.0f#pm%.0f, b=%.1f#pm%.1f ps", a, aErr, b, bErr), "l");
  leg->AddEntry(paper, "paper: a=256, b=17.5 ps", "l");
  leg->Draw();

  TLatex tx; tx.SetNDC(); tx.SetTextSize(0.028); tx.SetTextColor(kGray + 2);
  tx.DrawLatex(0.14, 0.20,
               Form("yield-scaled run; a_{stoch}#times#sqrt{%.3f} #approx %.0f ps#sqrt{GeV}",
                    YIELD_SCALE, aPhys));

  c->SaveAs("timing_resolution.png");
  c->SaveAs("timing_resolution.pdf");

  printf("\n[Timing] sigma_t = a/sqrt(E) (+) b\n");
  printf("   fitted a   = %.1f +/- %.1f  ps*sqrt(GeV)\n", a, aErr);
  printf("   fitted b   = %.1f +/- %.1f  ps\n", b, bErr);
  printf("   paper      : a = 256, b = 17.5 ps\n");
  printf("   a extrapolated to full LYSO yield ~ %.0f ps*sqrt(GeV)\n", aPhys);
  printf("   per-energy points:\n");
  for (auto& r : res)
    printf("      E = %5.0f GeV   sigma_t = %6.1f +/- %4.1f ps\n",
           r.E, r.sigma, r.sigmaErr);
}

// ---- entry point ---------------------------------------------------------
void analyze(const char* dir = "build/scan_out") {
  std::vector<TString> files = FindFiles(dir);
  if (files.empty()) {
    printf("No radical_run*.root files found in %s\n", dir);
    return;
  }
  printf("Found %zu run files in %s\n", files.size(), dir);
  Fig7(files);
  Timing(files);
}
