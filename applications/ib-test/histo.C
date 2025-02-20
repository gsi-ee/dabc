void draw_histo(const char *name, const char *title, const char *fname)
{
   std::ifstream f(fname);

   TH1* h = new TH1I(name, title, 100, 0., 7.5);

   int cnt = 0;

   while (!f.eof()) {
      double v;
      f >> v;

      // printf("value = %5.3f\n", v);

      h->Fill(v);

      cnt++;

      if (cnt == 150000) printf("25 percent boundary = %5.3f\n", v);
      if (cnt == 180000) printf("10 percent boundary = %5.3f\n", v);
      if (cnt == 190000) printf("5 percent boundary = %5.3f\n", v);
      if (cnt == 198000) printf("1 percent boundary = %5.3f\n", v);
   }

   TCanvas* c = new TCanvas(Form("can_%s", name), name, 3);

   c->SetLogy(1);

   h->GetXaxis()->SetTitle("#mus");

   h->Draw();

   c->SaveAs(Form("%s.png", name));
}

void histo()
{
   draw_histo("master_to_slave", "", "/misc/linev/m_to_s1.txt");
   draw_histo("slave_to_master", "", "/misc/linev/s1_to_m.txt");
}
