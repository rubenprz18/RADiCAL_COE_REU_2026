//
// myplots.C  --  starter template for making your OWN graphs from RADiCAL
//                simulation data.  Copy/edit the examples; that's the point.
//
// Run it:
//   root -l -b -q 'analysis/myplots.C("results/run_150GeV/radical_run0.root")'
//   (or simply:  ./run.sh plot results/run_150GeV/radical_run0.root )
//
// ---------------------------------------------------------------------------
// WHAT'S IN THE FILE (three ntuples = three tables you can plot from):
//
//   event  -- one row per beam electron. Columns (units):
//       eventID, beamE [MeV], beamX,beamY [mm], eLYSO [MeV], ePbGlass [MeV],
//       eCounters [MeV], npeSiPM, npeMCP, tMCP [ns], tModuleDown,tModuleUp [ns],
//       xCentroid,yCentroid [mm]
//
//   tile   -- one row per LYSO tile that was hit. Columns:
//       eventID, module, layer (0..28), edep [MeV], t [ns], x,y [mm]
//
//   channel-- one row per SiPM channel that fired. Columns:
//       eventID, module, channel (0-3 downstream, 50-53 upstream), npe, tHit [ns]
//
// The basic command is:   tree->Draw("Y:X", "cut", "option");
//   - "Y"        a 1D histogram of Y
//   - "Y:X"      a scatter/2D plot of Y vs X
//   - "cut"      a selection, e.g. "beamE>100000 && tMCP>0"
//   - ">>name(nbins,lo,hi)"  sends the result into a named histogram you control
// ---------------------------------------------------------------------------

void myplots(const char* filename = "results/run_150GeV/radical_run0.root") {
  TFile* f = TFile::Open(filename);
  if (!f || f->IsZombie()) { printf("Cannot open %s\n", filename); return; }

  auto* event   = (TTree*)f->Get("event");
  auto* tile    = (TTree*)f->Get("tile");
  auto* channel = (TTree*)f->Get("channel");
  if (!event) { printf("No 'event' tree in %s\n", filename); return; }

  gStyle->SetOptStat(1110);   // show entries/mean/RMS on histograms

  auto* c = new TCanvas("c", "RADiCAL plots", 1200, 900);
  c->Divide(2, 2);

  // (1) ENERGY in the LYSO -- a simple 1D histogram -----------------------
  c->cd(1);
  event->Draw("eLYSO>>hE(60,0,0)");          // (0,0) = auto-range
  if (auto* h = (TH1*)gPad->GetPrimitive("hE"))
    h->SetTitle("Energy sampled in LYSO;E_{LYSO} [MeV];events");

  // (2) LINEARITY: LYSO energy vs beam energy -- a 2D plot ----------------
  // (most useful on a multi-energy scan file; on a single-energy run it is a
  //  vertical strip showing the event-to-event spread)
  c->cd(2);
  event->Draw("eLYSO:beamE/1000.>>hLin(60,0,0,60,0,0)", "", "COLZ");  // y:x
  if (auto* h = (TH2*)gPad->GetPrimitive("hLin"))
    h->SetTitle("Sampling linearity;E_{beam} [GeV];E_{LYSO} [MeV]");

  // (3) LONGITUDINAL SHOWER PROFILE -- energy per layer from the tile tree -
  c->cd(3);
  tile->Draw("layer>>hprof(29,-0.5,28.5)", "edep");   // weight by edep
  if (auto* h = (TH1*)gPad->GetPrimitive("hprof"))
    h->SetTitle("Longitudinal profile;LYSO layer;total E [MeV]");

  // (4) TIMING: t_module - t_MCP -- with a validity cut -------------------
  c->cd(4);
  event->Draw("0.5*(tModuleDown+tModuleUp)-tMCP>>hdt(80,-2,4)",
              "tMCP>0 && tMCP<500 && tModuleDown>0 && tModuleUp>0");
  if (auto* h = (TH1*)gPad->GetPrimitive("hdt"))
    h->SetTitle("Shower timing;t_{module}-t_{MCP} [ns];events");

  c->SaveAs("myplots.png");
  c->SaveAs("myplots.pdf");
  printf("Wrote myplots.png / myplots.pdf\n");

  // ----- IDEAS to try next (uncomment / edit) ---------------------------
  // event->Draw("beamY:beamX", "", "COLZ");          // the beam spot
  // event->Draw("npeSiPM:beamE/1000.", "", "COLZ");  // light yield vs energy
  // channel->Draw("npe", "channel<50");              // p.e. in downstream SiPMs
  // event->Draw("xCentroid:yCentroid","","COLZ");    // shower position (Fig.9)
}
