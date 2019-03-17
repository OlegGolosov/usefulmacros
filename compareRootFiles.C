#include <boost/program_options.hpp>
#include <string>
#include <TString.h>
#include <TDirectoryFile.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TROOT.h>
#include <TKey.h>
#include <TList.h>
#include <TLatex.h>
#include <TPaveStats.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TGraph.h>
#include <THStack.h>
#include <TMultiGraph.h>
#include <TError.h>
#include <iostream>

using namespace std;


int colors [] = {kBlack, kRed, kBlue, kGreen+2, kMagenta+2, kOrange+2, kPink+2, kTeal+2, kCyan+2, kCyan+4, kAzure, kGray+3, kOrange+7, kGreen+4};
vector <TString> inputFileNames; 
vector <TString> labels;
TString folderName;
int maxDepth = 10;
bool rescale = true;
bool plot_ratio = false;
bool save_root = true;
bool save_pdf = true;
vector <TFile*> files;
vector <TDirectory*> dirs;
vector <TString> object_names;
TString outputPath = "comp.root"; 
TString outputPathPdf = "comp.pdf"; 
TFile *output_file;

inline istream& operator>>(istream& in, TString &data)
{
  string token;
  in >> token;
  data = token;
  return in;
}

bool parseArgs (int argc, char* argv[]);
void BuildObjectList (TDirectory *folder, int depth = 0);
void PlotTH1 (TString object_name);
void PlotTH2 (TString object_name);

int main (int argc, char* argv[])
{
  gROOT -> SetBatch (true);
  gErrorIgnoreLevel = 2000;
  gStyle -> SetOptStat (111111);
  gStyle -> SetTitleAlign (33);

  parseArgs (argc, argv);
  
  if (save_root) 
    output_file = new TFile (outputPath, "recreate");
  
  for (TString inputFileName : inputFileNames)
  {
    files.push_back(new TFile (inputFileName, "read"));
    dirs.push_back(files.back() -> GetDirectory (folderName));
  }
   
  BuildObjectList (dirs [0]);
  for (auto dir : dirs) 
  {
//    object_names = sorted (list(set(object_names).intersection(BuildObjectList (dir))), key = object_names.index)
//    delete dir;
  }

  TCanvas *c = new TCanvas ("c_first", "c_first");

  TLatex *text = new TLatex();
  text -> SetNDC();
  text -> SetTextSize(0.055);
  text -> SetTextFont(42);
  text -> DrawLatex(0.1, 0.9, "Comparing folder /" + folderName + " of files:");
  
  for (int i = 0; i < labels.size(); i++)
  {
    float ypos = 0.85 - 0.05 * i;
    text -> DrawLatex(0.1, ypos, inputFileNames [i] + " (" + labels [i] + ")");
  }
  c -> Print (outputPathPdf + "(","Title:Title");
      
  for (auto object_name : object_names)
  {
    while (object_name.BeginsWith ("/"))
      object_name.Remove (0,1);
    cout << object_name << endl;
    
    if (! files[0] -> Get(object_name)) cout << "BOO!!!";
    TObject *object = files [0] -> Get(object_name) -> Clone();
    
    if (object -> InheritsFrom("TH2"))
      PlotTH2 (object_name);
      
    else if (object -> InheritsFrom("TH1"))
      PlotTH1 (object_name);
      
    delete object;
  } 
  c -> SetName ("c_last");
  c -> SetTitle ("The end!");
  text -> DrawLatex(0.5, 0.6, "The end! ");

  c -> Print (outputPathPdf + ")","Title:The end!");
  output_file->Close();
  return 0;
}

bool parseArgs (int argc, char* argv[])
{
using namespace boost::program_options;
  options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "Print usage message")
    ("input,i", value< vector<TString> >()->required()->multitoken(), "Input ROOT files")
    ("labels,l", value< vector<TString> >()->multitoken(),"Labels for each file")
    ("output,o", value<TString>()->default_value("comp.root"),"Output file")
    ("folder,f", value<TString>()->default_value("/"), "Directory to compare")
    ("depth,d", value<int>()->default_value(10), "Maximum depth of folder search")
    ("ratio,r", value<bool>()->implicit_value(false)->default_value(true), "Plot histogram ratio")
    ("no-rescale", value<bool>()->implicit_value(false)->default_value(true), "Rescale histograms")
    ("no-root", value<bool>()->implicit_value(false)->default_value(true), "Write output to ROOT file")
    ("no-pdf,p", value<bool>()->implicit_value(false)->default_value(true), "Write output to PDF file")
  ;
  
  variables_map args;
  store(parse_command_line(argc, argv, desc), args);

  if (args.count("help")) {  
      cout << desc << "\n";
      return 0;
  }
  notify (args); 

  inputFileNames = args ["input"].as <vector <TString> > ();
  outputPath = args ["output"].as <TString> (); 
  folderName = args ["folder"].as <TString> ();
  maxDepth = args ["depth"].as <int> ();
  rescale = args ["no-rescale"].as <bool> ();
  plot_ratio = args ["ratio"].as <bool> ();
  save_root = args ["no-root"].as <bool> ();
  save_pdf = args ["no-pdf"].as <bool> ();
  
  if (args.count ("labels")) labels = args ["labels"].as <vector <TString> > (); 
  if (labels.size () > 0 && labels.size () != inputFileNames.size ())
  {
    cout << "Error! Number of labels provided is other than number of input files!\n";
    return false;
  }
  
  for (int i = labels.size(); i < inputFileNames.size(); i++) {
    TString label = inputFileNames[i];
    if (label.Contains ("/"))
      label.Remove (0, label.Last ('/') + 1);
    label.ReplaceAll (".root", "");
    labels.push_back (label); 
  }
  
  if (outputPath.EndsWith (".root"))
    outputPath = outputPath.Remove (outputPath.Last ('.'), 5);
    
  return true;
}


void BuildObjectList (TDirectory *folder, int depth)
{
  TString folder_path = folder -> GetPath();
  folder_path.Remove (0, folder_path.Last (':') + 1);
  TList *keys = folder -> GetListOfKeys ();
  for (auto key : *keys)
  {
    TString object_name = key -> GetName();
    auto object = dynamic_cast <TKey*> (key) -> ReadObj ();
    TString className = object -> ClassName();
    if (className.BeginsWith ("TH") || className.BeginsWith ("TProfile") || className.EndsWith ("Graph"))
      object_names.push_back (folder_path + '/' + object_name);
    else if (className == "TDirectoryFile" && depth < maxDepth)
    {
      BuildObjectList (dynamic_cast <TDirectoryFile*> (object), depth + 1);
    }
//    delete object;
  }
//  delete keys;
}


void PlotTH1 (TString object_name)
{
  auto ref_obj = files[0]->Get(object_name)->Clone();
  TProfile *ref_P1D = nullptr;
  TH1 *ref_hist = nullptr;
  TH1 *hist = nullptr;
  vector <TH1*> hists;
  
  if (nullptr != (ref_P1D = dynamic_cast<TProfile*>(ref_obj)))
  {
    ref_hist = ref_P1D -> ProjectionX();
    delete ref_obj;
  }
  else
    ref_hist = dynamic_cast<TH1*> (ref_obj);  

//  ref_hist -> Sumw2();
  
  TString title = object_name;
  TCanvas *c = new TCanvas ("c_" + title, title);
  c -> cd();
  gStyle -> SetOptTitle(1);
  auto stack = new THStack ("st_" + object_name, title);
  
  for (int i = 0; i < files.size(); i++)
  {
    hist = (TH1*) files [i] -> Get (object_name);
    hist -> SetName (object_name + Form ("_%d", i));
    hist -> SetTitle (labels [i]);
        
    if (rescale)
    {
      float scale_factor;
      if (hist -> GetEntries() != 0) 
//        scale_factor = 1.0 * ref_hist -> GetEntries() / hist -> GetEntries ();
        scale_factor = 1.0 * ref_hist -> Integral ("width") / hist -> Integral ("width");
      else scale_factor = 1.0;
      hist -> Scale(scale_factor);
    }
    
    hist -> SetLineWidth (2);
    hist -> SetLineColor  (colors [i]);
    hist -> SetMarkerColor (colors [i]);
    hist -> Draw ();
    gPad -> Modified ();
    gPad -> Update ();
    auto stats = (TPaveStats*) hist -> GetListOfFunctions() -> FindObject("stats");
    if (stats)
    {
      stats -> SetName (Form ("stats_%d", i));
      stats -> SetTextColor (colors [i]);
      stats -> SetX1NDC (0.8);
      stats -> SetX2NDC (1.0);
      float y_offset = 0.1 * i;
      stats -> SetY1NDC (0.9 - y_offset);
      stats -> SetY2NDC (1.0 - y_offset);
      TText *statTitle = stats -> GetLineWith (object_name);
      statTitle -> SetText (0, 0, labels [i]);
      stack -> Add (hist);
      hists.push_back (hist);
    }
  }
          
  gPad -> SetRightMargin (0.2);
  stack -> Draw ("nostack");
  //gPad -> BuildLegend (0.8,0.0,1.0,0.9 - y_offset);
  if (save_pdf) 
    c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "te"));
  if (save_root)  
    output_file -> cd();
    c -> Write();
  
  if (plot_ratio)
  {
    c -> SetName (Form ("%s_ratio", c -> GetName()));
    for (auto hist : hists)
    {
      //hist -> Sumw2()
      hist -> Divide (ref_hist);
      hist -> GetYaxis () -> SetRangeUser (0. ,4.);
    }
    if (save_pdf) 
      c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","te") + "_ratio");
    if (save_root) 
      c -> Write ();
    delete stack;
  }  
  delete ref_hist;
  for (auto hist : hists)
    delete hist;
}

void PlotTH2 (TString object_name)
{
  TLatex *text = new TLatex();
  text -> SetNDC();
  text -> SetTextSize(0.055);
  text -> SetTextFont(42);
  vector <TH2*> hists;
  
  
  auto ref_obj = files [0] -> Get (object_name) -> Clone();
  TProfile2D *ref_P2D = nullptr;
  TH2 *ref_hist = nullptr;
  TH2 *hist = nullptr;
  
  if (nullptr != (ref_P2D = dynamic_cast <TProfile2D*> (ref_obj)))
  {
    ref_hist = ref_P2D -> ProjectionXY();
    delete ref_obj;
  }
  else
    ref_hist = dynamic_cast <TH2*> (ref_obj);  
//  ref_hist -> Sumw2();
  
  TString title = object_name;
  TCanvas *c = new TCanvas ("c_" + title, title);
  c -> cd();
  gStyle -> SetOptTitle (0);
  text -> DrawLatex (0.1, 0.95, title);
  TPad *c1 = new TPad ("c1", "c1", 0., 0., 1., 0.95);
  c1 -> Draw();
  c1 -> cd();
  int npads = labels.size();
  int npadsx = int (ceil (sqrt (npads)));
  int npadsy = int (ceil (1. * npads / npadsx));
  c1 -> Divide (npadsx, npadsy);
  
  for (int i = 0; i < files.size(); i++)
  {
    c1 -> cd (i + 1);
    gPad -> SetLogz ();
    gPad -> SetLeftMargin (0.15);
    gPad -> SetTopMargin (0.01);

    hist = (TH2*) files [i] -> Get (object_name);
    hist -> SetName (object_name + Form ("_%d", i));
    hist -> SetTitle (labels [i]);
    
    if (rescale)
    {
      float scale_factor;
      if (hist -> GetEntries() != 0) 
//        scale_factor = 1.0 * ref_hist -> GetEntries() / hist -> GetEntries ();
        scale_factor = 1.0 * ref_hist -> Integral ("width") / hist -> Integral ("width");
      else scale_factor = 1.0;
      hist -> Scale(scale_factor);
    }
  
    hist -> Draw ("colz");
    gPad -> Modified();
    gPad -> Update();
    TPaveStats *stats = (TPaveStats*) gPad -> GetPrimitive ("stats");
    if (stats) {
      stats -> SetName (Form ("stats_%d", i));
      stats -> SetTextColor (colors [i]);
      stats -> SetX1NDC (.7);
      stats -> SetX2NDC (.9);
      stats -> SetY1NDC (.7);
      stats -> SetY2NDC (.99);
      TText *statTitle = stats -> GetLineWith (hist -> GetName());
      statTitle -> SetText (0, 0, hist -> GetTitle());
      hist -> SetStats (0);
      hists.push_back (hist);
    }
  }
  
  if (save_pdf)
    c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","te"));
  if (save_root)
  { 
    output_file -> cd();
    c -> Write();
  }
  if (plot_ratio)
  {
    c -> SetName (Form("%s_ratio", c -> GetName()));
    c -> cd();
    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels [0]);
    for (int i = 0; i < labels.size(); i++)
    {
      c1 -> cd (i + 1);
      gPad -> SetLogz (0);
      hist = (TH2*) gPad -> GetPrimitive (object_name + Form ("_%d", i));
//      hist -> Sumw2()
      hist -> Divide (ref_hist);
      hist -> GetZaxis() -> SetRangeUser(0, 4);
    }
    if (save_pdf) 
      c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "te") + "_ratio");
    if (save_root)
      c -> Write();
  }
        
  delete ref_hist;
  for (auto hist : hists)
    delete hist;
}