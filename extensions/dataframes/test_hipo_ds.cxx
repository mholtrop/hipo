//
// Created by Maurik Holtrop on 7/19/22.
//
//pre-fix = 16.6s for 10 hipo files.
//
#include <string>
#include <iostream>
#include <chrono>
#include "RHipoDS.hxx"
#include "TCanvas.h"

using namespace ROOT;
using namespace ROOT::RDF;

std::vector<float> v_abs(std::vector<float>  &x, std::vector<float> &y, std::vector<float> &z){ std::vector<float> out;
   for(int i=0; i< x.size(); ++i){
      out.push_back(sqrt(x[i]*x[i]+y[i]*y[i]+z[i]*z[i]));
   };
   return out;
};

int main(int argc, char **argv) {
   // Very simple test of the Hipo DataFrame.
   // ROOT::EnableImplicitMT();
   int N_open = 1000000;
   std::chrono::nanoseconds delta_t;

   if(argc < 2){
      std::cout << "Please specify a HIPO data file on the command line. (Only one file.) \n";
      return 1;
   }else{
      std::cout << "Opening file " << argv[1] << std::endl;
   }

   auto start = std::chrono::high_resolution_clock::now();
   auto ds = std::make_unique<RHipoDS>(argv[1], N_open);
   auto cols_ds = ds->GetColumnNames();
   bool translated = ds->fColumnNameTranslation;
   auto stop = std::chrono::high_resolution_clock::now();
   delta_t = std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
   printf("Open file in  %6.5f ms  for %6d events = %6.5f ns/event\n",
          delta_t.count()*1.e-6, N_open, double(delta_t.count())/N_open );

   //("/data/CLAS12/data/hipo/rec_clas_016321.evio.00001.hipo");
//   auto all_columns = ds->GetColumnNames();
//   for(int i=0; i< all_columns.size(); ++i){
//      printf("%40s  bank id: %4d  %s \n", all_columns[i].c_str(), i, ds->fColumnTypeIsVector[i] ? "vector":"scaler" );
//   }
   auto total_events = ds->GetEntries();
   auto df = RDataFrame(std::move(ds));
   auto cols_df = df.GetColumnNames();
   RInterface<Detail::RDF::RLoopManager, void> df2 = df;
   std::string run_config_event = "RUN::config.event";
   if(translated){
      df2 = df.Alias("px", "REC_Particle_px").Alias("py", "REC_Particle_py").Alias("pz", "REC_Particle_pz").Alias("pid", "REC_Particle_pid").Alias("status","REC_Particle_status");
      run_config_event = "RUN_config_event";
   }else{
      df2 = df.Alias("px", "REC::Particle.px").Alias("py", "REC::Particle.py").Alias("pz", "REC::Particle.pz").Alias("pid", "REC::Particle.pid").Alias("status","REC::Particle.status");
   }

   auto h_pid=df2.Histo1D({"h_pid","Particle ID",4601,-2300,2300},"pid");
   auto h_evt = df2.Histo1D({"h_evt", "Event number", 1000001, 0, 1000000}, run_config_event);
   auto h_px = df2.Histo1D({"h_px", "P_x", 1000, 0., 12.},"px");
   auto h_py = df2.Histo1D({"h_py", "P_y", 1000, 0., 12.},"py");
   auto h_pz = df2.Histo1D({"h_pz", "P_z", 1000, 0., 12.},"pz");

   // Lambda function for the absolute of a vector component set.
   auto v_abs_l = [](
         std::vector<float> &x, std::vector<float> &y, std::vector<float> &z)
   { RVec<double> out;
      for(int i=0; i< x.size(); ++i){
         out.push_back(sqrt(x[i]*x[i]+y[i]*y[i]+z[i]*z[i]));
      };
      return out;
   };

   auto h_p = df2.Define("p",v_abs,{"px","py","pz"}).Histo1D({"h_p","P (Momentum)", 1000, 0., 12.}, "p");
   //
   // Note that for the following style of DataFrame definitions, you *must* use aliasses. The original names
   // of columns in HIPO are incompatible with C++ (or Python or anything really) code direct access to these variables.
   //
   //   auto h_p = df2.Define("p","vector<float> out;for(int i=0; i< px.size(); ++i){out.push_back(sqrt(px[i]*px[i]+py[i]*py[i]+pz[i]*pz[i]));}; return out;").Histo1D({"h_p","P (Momentum)", 1000, 0., 12.}, "p");

   TCanvas* c = new TCanvas("c", "Test RHipoDS", 0, 0, 2000, 1000);
   c->Divide(2, 1);
   c->cd(1);
   // First pass through the data
   start = std::chrono::high_resolution_clock::now();
   h_pid->DrawClone();
   stop = std::chrono::high_resolution_clock::now();
   delta_t = std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
   double time_ns = double(delta_t.count());
   printf("processed events = %7lu  in  %6.5f s, or %10.3f ns/event. \n", total_events, time_ns*1.e-9,
          time_ns/total_events);

   c->cd(2);
   start = std::chrono::high_resolution_clock::now();
   h_evt->DrawClone();
   stop = std::chrono::high_resolution_clock::now();
   delta_t = std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
   time_ns = double(delta_t.count());
//   printf("processed events = %7lu  in  %6.5f s, or %10.3f ns/event. \n", total_events, time_ns*1.e-9, time_ns/total_events);

   c->Print("demo1.pdf");

   c->Clear();
   c->Divide(2, 2);

   start = std::chrono::high_resolution_clock::now();
   auto p1 = c->cd(1);
   p1->SetLogy();
   h_px->DrawClone();

   auto p2 = c->cd(2);
   p2->SetLogy();
   h_py->DrawClone();

   auto p3 = c->cd(3);
   p3->SetLogy();
   h_pz->DrawClone();

   auto p4 = c->cd(4);
   p4->SetLogy();
   h_p->DrawClone();

   stop = std::chrono::high_resolution_clock::now();
   delta_t = std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
   time_ns = double(delta_t.count());
//   printf("processed events = %7lu  in  %6.5f s, or %10.3f ns/event. \n", total_events, time_ns*1.e-9, time_ns/total_events);
   c->Print("demo2.pdf");
}