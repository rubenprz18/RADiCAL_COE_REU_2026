void draw_hex7(){
  gStyle->SetOptStat(0);
  auto* c=new TCanvas("c","hex7",560,560);
  auto* fr=new TH2F("fr",";x [mm];y [mm]",10,-9,9,10,-9,9); fr->Draw();
  double R=8.08; // circumradius (14mm flat-to-flat)
  TPolyLine* hex=new TPolyLine(7);
  for(int k=0;k<6;k++){double a=(60*k+30)*TMath::DegToRad(); hex->SetPoint(k,R*cos(a),R*sin(a));}
  double a0=30*TMath::DegToRad(); hex->SetPoint(6,R*cos(a0),R*sin(a0));
  hex->SetLineColor(kBlue+2); hex->SetLineWidth(3); hex->Draw();
  auto cap=[&](double x,double y,int col){auto*e=new TEllipse(x,y,0.6);e->SetFillColor(col);e->SetLineColor(kBlack);e->Draw();};
  // centre (T-type, read out) + 6 ring alternating T(red)/E(blue)
  cap(0,0,kYellow);                       // centre (T-type)
  for(int k=0;k<6;k++){double a=60*k*TMath::DegToRad();
    cap(4.0*cos(a),4.0*sin(a), (k%2==0)?kRed:kBlue);}
  auto* t=new TLatex(); t->SetTextSize(0.04);
  t->DrawLatex(-8.5,8.1,"Hex7: red=T-type(timing), blue=E-type(energy), yellow=centre");
  c->SaveAs("analysis/figures/hex7_layout.png");
}
