void hex_long(){
  gStyle->SetOptStat(0);
  auto* c=new TCanvas("c","hex longitudinal",1500,460);
  c->SetLeftMargin(0.06); c->SetRightMargin(0.02); c->SetBottomMargin(0.16);
  // dimensions (mm)
  double lyso=1.5, tyvek=0.1, wrap=lyso+2*tyvek, W=2.5;
  int nL=29, nW=28;
  double stack=nL*wrap+nW*W;        // 119.3
  double z0=-stack/2.0;             // upstream face
  double hw=7.0;                    // hex flat-to-flat half (14mm)
  double capL=183.0, capHalf=capL/2;
  double smZ=z0+44.0;               // shower-max centre
  auto* fr=new TH2F("fr",";beam direction z [mm];transverse [mm]",10,-100,100,10,-11,11);
  fr->Draw();
  // layers
  double z=z0;
  for(int i=0;i<nL;i++){
    auto* b=new TBox(z,-hw,z+wrap,hw); b->SetFillColor(kAzure+1); b->SetLineColor(kAzure+2); b->Draw("l"); // LYSO(+Tyvek)
    z+=wrap;
    if(i<nW){ auto* w=new TBox(z,-hw,z+W,hw); w->SetFillColor(16); w->SetLineColor(15); w->Draw("l"); z+=W; }
  }
  // capillaries (projected x rows) running full length
  double xs[5]={-4,-2,0,2,4};
  for(double x: xs){
    auto* cap=new TBox(-capHalf,x-0.45,capHalf,x+0.45); cap->SetFillColor(kYellow); cap->SetLineColor(kOrange+3); cap->Draw("l");
    // WLS filament at shower max
    auto* fil=new TBox(smZ-7.5,x-0.45,smZ+7.5,x+0.45); fil->SetFillColor(kOrange+1); fil->Draw();
  }
  // SiPMs at both ends
  for(double x: xs){
    auto* s1=new TBox(capHalf,x-0.6,capHalf+4,x+0.6); s1->SetFillColor(kGreen+1); s1->Draw();
    auto* s2=new TBox(-capHalf-4,x-0.6,-capHalf,x+0.6); s2->SetFillColor(kGreen+1); s2->Draw();
  }
  // arrows / labels
  auto* tx=new TLatex(); tx->SetTextSize(0.045);
  tx->SetTextColor(kAzure+2); tx->DrawLatex(-95,9.0,"LYSO:Ce (1.5mm, blue) + W (2.5mm, grey) x29/28");
  tx->SetTextColor(kOrange+3); tx->DrawLatex(-95,-9.2,"quartz capillaries (yellow), DSB1/LuAG WLS at shower max (orange)");
  tx->SetTextColor(kGreen+2); tx->DrawLatex(60,-9.2,"SiPM");
  auto* arr=new TArrow(-99,0,-90,0,0.02,"|>"); arr->SetLineColor(kRed+1); arr->SetLineWidth(3); arr->SetFillColor(kRed+1); arr->Draw();
  tx->SetTextColor(kRed+1); tx->DrawLatex(-99,1.2,"e^{-} beam");
  auto* t2=new TLatex(); t2->SetTextSize(0.05); t2->SetTextColor(kBlack);
  t2->DrawLatex(-30,9.5,"Hexagonal RADiCAL module - longitudinal section");
  c->SaveAs("analysis/figures/hex/hex7_longitudinal_section.png");
}
