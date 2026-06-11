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
  double E = 0;          // GeV
  double sigMod = 0;     // intrinsic module resolution [ps] (reference removed)
  double sigModErr = 0;
  double sigMCP = 0;     // module - MCP, as a real test beam measures it [ps]
  double sigMCPErr = 0;
};

// Robust timing width (returned in ps) of a list of times (ns): the trimmed
// standard deviation about the median (within +/-4 MAD-sigma). This is stable
// on small samples, where a Gaussian fit can diverge.
static void FitGauss(std::vector<double> v, double& sig, double& sigErr,
                     TCanvas* c, int pad, const char* title) {
  sig = sigErr = 0;
  if (v.size() < 8) return;
  std::sort(v.begin(), v.end());
  double med = v[v.size() / 2];
  std::vector<double> ad;
  for (double x : v) ad.push_back(std::fabs(x - med));
  std::sort(ad.begin(), ad.end());
  double rsig = std::max(1.4826 * ad[ad.size() / 2], 0.005);

  // trimmed mean & standard deviation of the Gaussian CORE (+/-2.5 MAD-sigma),
  // matching the paper's practice of fitting the peak (non-Gaussian low-energy
  // late tails, from channels with few p.e., are excluded).
  double s = 0, s2 = 0; int n = 0;
  std::vector<double> core;
  for (double x : v)
    if (std::fabs(x - med) < 2.5 * rsig) { s += x; s2 += x * x; ++n; core.push_back(x); }
  if (n < 5) { s = s2 = 0; n = 0; for (double x : v) { s += x; s2 += x * x; ++n; } core = v; }
  double mean = s / n, var = s2 / n - mean * mean;
  if (var < 0) var = 0;
  sig = std::sqrt(var) * 1000.;          // ps
  sigErr = sig / std::sqrt(2.0 * n);     // approximate error on a width

  // draw the distribution for the dt-distributions canvas
  if (c && pad > 0) {
    double win = std::max(4 * rsig, 0.05);
    static int uid = 0; ++uid;
    auto* h = new TH1D(Form("hg_%d", uid), title, 50, med - win, med + win);
    for (double x : core) h->Fill(x);
    c->cd(pad); h->Draw();
  }
}

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
// For each energy file, fit (a) the intrinsic module resolution = width of the
// t_module distribution (its mean is just the constant time-of-flight, so the
// width is the resolution with the reference removed -- this is the quantity the
// paper quotes after subtracting the MCP), and (b) the as-measured module - MCP.
static RunResult FitOneTiming(TFile* file, TCanvas* c, int pad) {
  RunResult r;
  TTree* ev = (TTree*)file->Get("event");
  if (!ev) return r;

  double beamE, tDown, tUp, tMCP;
  ev->SetBranchAddress("beamE", &beamE);
  ev->SetBranchAddress("tModuleDown", &tDown);
  ev->SetBranchAddress("tModuleUp", &tUp);
  ev->SetBranchAddress("tMCP", &tMCP);

  const double TMAX = 500.;  // ns; undetected channels carry a 1e9 sentinel
  std::vector<double> vMod, vMCP;
  double sumE = 0.; Long64_t nev = ev->GetEntries();
  for (Long64_t i = 0; i < nev; ++i) {
    ev->GetEntry(i);
    sumE += beamE;
    if (tDown > 0 && tDown < TMAX && tUp > 0 && tUp < TMAX) {
      double tModule = 0.5 * (tDown + tUp);
      vMod.push_back(tModule);
      if (tMCP > 0 && tMCP < TMAX) vMCP.push_back(tModule - tMCP);
    }
  }
  r.E = (nev ? sumE / nev : 0.) / 1000.;
  FitGauss(vMod, r.sigMod, r.sigModErr, c, pad,
           Form("%.0f GeV;t_{module} [ns];events", r.E));
  FitGauss(vMCP, r.sigMCP, r.sigMCPErr, nullptr, 0, "");
  return r;
}

static void Timing(const std::vector<TString>& files) {
  gStyle->SetOptStat(0);
  gStyle->SetOptFit(0);

  auto* cdt = new TCanvas("c_dt", "module time distributions", 1200, 700);
  cdt->Divide(3, 2);

  std::vector<RunResult> res;
  int pad = 1;
  for (auto& fn : files) {
    TFile* file = TFile::Open(fn);
    if (!file || file->IsZombie()) continue;
    RunResult r = FitOneTiming(file, cdt, pad++);
    if (r.sigMod > 0) res.push_back(r);
  }
  cdt->SaveAs("timing_dt_distributions.png");

  if (res.size() < 2) { printf("[Timing] too few points to fit\n"); return; }
  std::sort(res.begin(), res.end(),
            [](const RunResult& a, const RunResult& b) { return a.E < b.E; });

  auto* gMod = new TGraphErrors();   // intrinsic module resolution (headline)
  auto* gMCP = new TGraphErrors();   // as-measured module - MCP
  for (size_t i = 0; i < res.size(); ++i) {
    gMod->SetPoint(i, res[i].E, res[i].sigMod);
    gMod->SetPointError(i, 0., res[i].sigModErr);
    gMCP->SetPoint(i, res[i].E, res[i].sigMCP);
    gMCP->SetPointError(i, 0., res[i].sigMCPErr);
  }

  auto* c = new TCanvas("c_timing", "RADiCAL timing resolution", 800, 600);
  gMCP->SetTitle("RADiCAL timing resolution;Beam energy E [GeV];#sigma_{t} [ps]");
  gMCP->SetMinimum(0.);
  gMCP->SetMarkerStyle(24);
  gMCP->SetMarkerColor(kGray + 1);
  gMCP->SetLineColor(kGray + 1);
  gMCP->Draw("AP");                  // frame (larger values) drawn first
  gMod->SetMarkerStyle(20);
  gMod->SetMarkerSize(1.3);
  gMod->SetMarkerColor(kBlue + 1);
  gMod->SetLineColor(kBlue + 1);
  gMod->Draw("P SAME");

  // fit the intrinsic module resolution to  sigma = sqrt(a^2/E + b^2)
  auto* fit = new TF1("fit", "sqrt([0]*[0]/x + [1]*[1])", 20, 160);
  fit->SetParameters(150., 18.);
  fit->SetParNames("a", "b");
  fit->SetLineColor(kBlue + 1);
  gMod->Fit(fit, "Q");
  double a = fit->GetParameter(0), b = fit->GetParameter(1);
  double aErr = fit->GetParError(0), bErr = fit->GetParError(1);
  fit->Draw("SAME");

  auto* paper = new TF1("paper", "sqrt([0]*[0]/x + [1]*[1])", 20, 160);
  paper->SetParameters(256., 17.5);
  paper->SetLineColor(kRed + 1);
  paper->SetLineStyle(2);
  paper->Draw("SAME");

  auto* leg = new TLegend(0.34, 0.60, 0.88, 0.88);
  leg->SetBorderSize(0);
  leg->AddEntry(gMod, "module (this work, reference removed)", "p");
  leg->AddEntry(fit, Form("fit: a=%.0f#pm%.0f, b=%.1f#pm%.1f ps", a, aErr, b, bErr), "l");
  leg->AddEntry(paper, "paper: a=256, b=17.5 ps", "l");
  leg->AddEntry(gMCP, "module #minus MCP (as measured)", "p");
  leg->Draw();

  c->SaveAs("timing_resolution.png");
  c->SaveAs("timing_resolution.pdf");

  printf("\n[Timing] intrinsic module resolution:  sigma_t = a/sqrt(E) (+) b\n");
  printf("   fitted a = %.1f +/- %.1f ps*sqrt(GeV),  b = %.1f +/- %.1f ps\n",
         a, aErr, b, bErr);
  printf("   paper    : a = 256, b = 17.5 ps\n");
  printf("   %6s   %12s   %14s\n", "E[GeV]", "module[ps]", "module-MCP[ps]");
  for (auto& r : res)
    printf("   %6.0f   %12.1f   %14.1f\n", r.E, r.sigMod, r.sigMCP);
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
