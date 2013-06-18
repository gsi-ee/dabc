// this file needed only to create stream infos
// later it should be done directly in DABC process

void new_file() {
    TFile* f = TFile::Open("ff.root", "RECREATE");
    h1 = new TH1F("myhisto","Tilte of myhisto", 100, -10., 10.);
    h1->FillRandom("gaus", 10000);
    h1->Write();
    delete f;
}
