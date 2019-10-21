#include <boost/program_options.hpp>
#include <string>
#include <TString.h>
#include <TDirectoryFile.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TKey.h>
#include <TList.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TPaveStats.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TGraph.h>
#include <THStack.h>
#include <TGraphErrors.h>
#include <TGraphAsymmErrors.h>
#include <TMultiGraph.h>
#include <TError.h>
#include <iostream>
#include <regex>
#include <algorithm>

using namespace std;


const vector <int> colors = 
{
  kBlack, kRed, kBlue, kGreen+2, kMagenta+2, kOrange+2, kPink+2,
  kTeal+2, kCyan+2, kCyan+4, kAzure, kGray+3, kOrange+7, kGreen+4
};
const vector <int> lineStyles = {1, 7}; 
const vector <vector <int>> markerStyles = 
{ 
  {20, 21, 22, 23, 29, 33, 34, 39, 41, 43, 45, 47, 48, 49}, // filled
  {24, 25, 26, 32, 30, 27, 28, 37, 40, 42, 44, 46, 35, 36}  // empty
};
int canvasWidth = 640; 
int canvasHeight = 480;

vector <TString> inputFileNames; 
vector <TString> excludedFolders;
vector <TString> labels;
TString folderName;
int maxDepth = 10;
bool rescale = true;
bool plot_ratio = false;
bool save_root = true;
bool save_png = false;
bool save_pdf = true;
bool saveEmpty = false;
vector <TFile*> files;
vector <TDirectory*> dirs;
vector <TString> object_names;
TString outputPath = "comp.root"; 
TString outputPathPdf = "comp.pdf";
bool xRangeSet = false;
bool yRangeSet = false;
bool zRangeSet = false;
vector <float> xRange;
vector <float> yRange;
vector <float> zRange;
bool logX, logY, logZ;
float lts;
TFile *output_file;

bool gUseIncludePattern = false;
string gIncludePattern = "";

inline istream& operator>>(istream& in, TString &data)
{
  string token;
  in >> token;
  data = token;
  return in;
}

bool parseArgs (int argc, char* argv[]);
void BuildObjectList (TDirectory *folder, int depth = 0);
void FilterObjectList();
void PlotTH1 (TString object_name);
void PlotGraph (TString object_name);
void PlotMultiGraph (TString object_name);
void Plot2MultiGraphs (TString object_name);
void PlotTHStack (TString object_name);
void Plot2THStacks (TString object_name);
void PlotTH2 (TString object_name);
bool DivideGraphs (TGraph *graph, TGraph *graph_ref);
bool DivideMultiGraphs (TMultiGraph *mg, TMultiGraph *mg_ref);
bool DivideTHStacks (THStack* hs, THStack *hs_ref);

int main (int argc, char* argv[])
{
  gErrorIgnoreLevel = 2000;
  gStyle -> SetOptStat (111111);
//  gStyle -> SetTitleAlign (33);
  gStyle -> SetLegendBorderSize (0);
  gStyle -> SetLegendTextSize (lts);
  gStyle->SetStatStyle (0);

  parseArgs (argc, argv);
  
  if (save_png)
    gSystem -> Exec("mkdir -p " + outputPath); 
		
  if (save_root) 
    output_file = new TFile (outputPath + ".root", "recreate");
  
  for (TString inputFileName : inputFileNames)
  {
    files.push_back(new TFile (inputFileName, "read"));
    dirs.push_back(files.back() -> GetDirectory (folderName));
  }
   
  BuildObjectList (dirs [0]);
  FilterObjectList();
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
  text -> DrawLatex(0.1, 0.9, "Comparing folder " + folderName + " of files:");
  
  for (int i = 0; i < labels.size(); i++)
  {
    float ypos = 0.85 - 0.05 * i;
    text -> DrawLatex(0.1, ypos, inputFileNames [i] + " (" + labels [i] + ")");
  }
  if (save_pdf) c -> Print (outputPathPdf + "(","Title:Title");
      
  for (auto object_name : object_names)
  {
    while (object_name.BeginsWith ("/"))
      object_name.Remove (0,1);
    cout << object_name << endl;
    
    TObject *object = files [0] -> Get(object_name);
    TString className = object -> ClassName();
      
    if (className.Contains ("TH2") || className.Contains ("TProfile2"))
      PlotTH2 (object_name);
    
    else if (className.Contains ("TH1") || className.Contains ("TProfile"))
      PlotTH1 (object_name);      
      
    else if (className.Contains ("TGraph"))
      PlotGraph (object_name);
      
    else if (className.Contains ("TMultiGraph"))
    { 
      if (labels.size() == 2)
        Plot2MultiGraphs (object_name);
      else
        PlotMultiGraph (object_name);
    }
    else if (className.Contains ("THStack"))
    {
      if (labels.size() == 2)
        Plot2THStacks (object_name);
      else
        PlotTHStack (object_name);
    }
    else 
      delete object;
  } 
  c -> SetName ("c_last");
  c -> SetTitle ("The end!");
  text -> DrawLatex(0.5, 0.6, "The end! ");

  if (save_pdf) c -> Print (outputPathPdf + ")","Title:The end!");
  if (save_root) output_file -> Close();
  return 0;
}

bool parseArgs (int argc, char* argv[])
{
  using namespace boost::program_options;
  options_description desc ("Allowed options");
  desc.add_options()
    ("help,h", "Print usage message")
    ("input,i", value< vector<TString> >()->required()->multitoken(), "Input ROOT files")
    ("labels,l", value< vector<TString> >()->multitoken(),"Labels for each file")
    ("exclude,e", value< vector<TString> >()->multitoken(),"Folders to exclude")
    ("output,o", value<TString>()->default_value("comp.root"),"Output file")
    ("folder,f", value<TString>()->default_value("/"), "Directory to compare")
    ("depth,d", value<int>()->default_value(10), "Maximum depth of folder search")
    ("ratio,r", value<bool>()->implicit_value(true)->default_value(false), "Plot histogram ratio")
    ("xrange,x", value< vector<float> >()->multitoken(),"X axis range") 
    ("yrange,y", value< vector<float> >()->multitoken(),"Y axis range")
    ("zrange,z", value< vector<float> >()->multitoken(),"Z axis range")
    ("logx", value<bool>()->implicit_value(true)->default_value(false), "SetLogX()")
    ("logy", value<bool>()->implicit_value(true)->default_value(false), "SetLogY()")
    ("logz", value<bool>()->implicit_value(false)->default_value(true), "SetLogZ(0)")
    ("lts", value<float>()->default_value(0.03), "Legend Text Size")
    ("no-rescale", value<bool>()->implicit_value(false)->default_value(true), "Do not rescale histograms")
    ("save-empty", value<bool>()->implicit_value(true)->default_value(false), "Save empty and zero histograms")
    ("no-pdf", value<bool>()->implicit_value(false)->default_value(true), "Do not write output to PDF file")
    ("no-root", value<bool>()->implicit_value(false)->default_value(true), "Do not write output to ROOT file")
    ("png", value<bool>()->implicit_value(true)->default_value(false), "Write output to png files")

    ("include-pattern", value<string>(&gIncludePattern), "include only objects matching pattern")
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
  saveEmpty = args ["save-empty"].as <bool> ();
  save_pdf = args ["no-pdf"].as <bool> ();
  save_root = args ["no-root"].as <bool> ();
  save_png = args ["png"].as <bool> ();
  logX = args ["logx"].as <bool> ();
  logY = args ["logy"].as <bool> ();
  logZ = args ["logz"].as <bool> ();
  lts = args ["lts"].as <float> ();
  
  if (args.count ("xrange")) xRange = args ["xrange"].as <vector <float> > ();
  if (args.count ("yrange")) yRange = args ["yrange"].as <vector <float> > ();
  if (args.count ("zrange")) zRange = args ["zrange"].as <vector <float> > ();
  
  if (xRange.size() >= 2)
  {
    if (xRange.at(1) > xRange.at(0)) xRangeSet = true;
    else cout << "WARNING: upper X axis range is smaller than the lower one.\n Switching to default.\n";
  } 
  if (yRange.size() >= 2)
  {
    if (yRange.at(1) > yRange.at(0)) yRangeSet = true;
    else cout << "WARNING: upper Y axis range is smaller than the lower one.\n Switching to default.\n";
  } 
  if (zRange.size() >= 2)
  {
    if (zRange.at(1) > zRange.at(0)) zRangeSet = true;
    else cout << "WARNING: upper Z axis range is smaller than the lower one.\n Switching to default.\n";
  } 
  
  if (args.count ("exclude")) excludedFolders = args ["exclude"].as <vector <TString> > (); 
  
  if (args.count ("labels")) labels = args ["labels"].as <vector <TString> > (); 
  if (labels.size() > 0 && labels.size() != inputFileNames.size())
  {
    cout << "Error! Number of labels provided is other than number of input files!\n";
    return false;
  }

  if (args.count("include-pattern"))
    gUseIncludePattern = true;


  for (int i = labels.size(); i < inputFileNames.size(); i++) {
    TString label = inputFileNames[i];
    if (label.Contains ("/"))
      label.Remove (0, label.Last ('/') + 1);
    label.ReplaceAll (".root", "");
    labels.push_back (label); 
  }
  
  if (outputPath.EndsWith (".root"))
    outputPath = outputPath.Remove (outputPath.Last ('.'), 5);
    
  outputPathPdf = outputPath + ".pdf";



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
    if (className.BeginsWith ("TH") || className.BeginsWith ("TProfile") || className.Contains ("Graph"))
      object_names.push_back (folder_path + '/' + object_name);
    else if (className == "TDirectoryFile" && depth < maxDepth)
    {
      bool skipFolder = false;
      for (auto exFolder : excludedFolders)
        if (object_name == exFolder) skipFolder = true;  
      if (!skipFolder) BuildObjectList (dynamic_cast <TDirectoryFile*> (object), depth + 1);
    }
    delete object;
  }
}


void PlotTH1 (TString object_name)
{
  auto ref_obj = files[0]->Get(object_name)->Clone("htemp");
  TProfile *ref_P1D = nullptr;
  TH1 *ref_hist = nullptr;
  TH1 *hist = nullptr;
  vector <TH1*> hists;
  
  if (nullptr != (ref_P1D = dynamic_cast <TProfile*> (ref_obj)))
  {
    ref_hist = ref_P1D -> ProjectionX();
    delete ref_obj;
  }
  else
    ref_hist = dynamic_cast<TH1*> (ref_obj);  

//  ref_hist -> Sumw2();
  
  int nEntries = 0;
  float sumMean = 0;
  float sumError = 0;
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  TCanvas *c = new TCanvas ("c_" + name, title);
  c -> cd();
  auto stack = new THStack ("st_" + name, title);
  
  for (int i = 0; i < files.size(); i++)
  {
    hist = (TH1*) files [i] -> Get (object_name);
    if (!hist) continue;
    hist -> SetName (name + Form ("_%d", i));
    hist -> SetTitle (labels [i]);
        
    if (!hist -> InheritsFrom ("TProfile") && rescale)
    {
      float scale_factor;
      if (hist -> GetSumOfWeights() != 0) 
//        scale_factor = 1.0 * ref_hist -> GetEntries() / hist -> GetEntries ();
        scale_factor = 1.0 * ref_hist -> Integral ("width") / hist -> Integral ("width");
      else scale_factor = 1.0;
      hist -> Scale(scale_factor);
    }
    
    nEntries += hist -> GetEntries(); 
    sumMean += hist -> GetMean(); 
    sumError += hist -> GetMeanError(); 
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
      TText *statTitle = stats -> GetLineWith (name);
      statTitle -> SetText (0, 0, labels [i]);
      stack -> Add (hist, hist->GetOption());
      hists.push_back (hist);
    }
  }
  
  if (saveEmpty || (!saveEmpty && (nEntries > 0 || sumMean > 0. || sumError > 0.)))
  {
    gPad -> SetRightMargin (0.2);
    stack -> Draw ("nostack");
//    stack -> Draw ("hist nostack"); // lines
    //gPad -> BuildLegend (0.8, 0.0, 1.0, 1.0);
    stack -> GetXaxis() -> SetTitle (ref_hist -> GetXaxis () -> GetTitle());	
    stack -> GetYaxis() -> SetTitle (ref_hist -> GetYaxis () -> GetTitle());
    if (xRangeSet) stack -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
    if (yRangeSet) stack -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
    c -> SetLogx (logX);
    c -> SetLogy (logY);
    if (save_pdf) 
      c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx"));
    if (save_png) 
      c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
    if (save_root)
    {
      output_file -> cd();
      c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
    }
    
    if (plot_ratio)
    {
      c -> SetName (Form ("%s_ratio", c -> GetName()));
      c -> SetTitle (Form ("%s: ratio to %s", c -> GetTitle(), labels [0].Data()));
      for (auto hist : hists)
      {
        //hist -> Sumw2()
        hist -> Divide (ref_hist);
        hist -> GetYaxis() -> SetRangeUser (0., 4.);
      }
      if (save_pdf) 
        c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","tx") + "_ratio");
      if (save_png) 
        c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
      if (save_root) 
        c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
    }
  }  
  delete stack;
  delete ref_hist;
  for (auto hist : hists)
    delete hist;
  delete c;
}

void PlotGraph (TString object_name)
{
  auto ref_graph = (TGraph*) files[0]->Get(object_name)->Clone("htemp");
  
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  TCanvas *c = new TCanvas ("c_" + title, title);
  c -> cd();
  auto mg = new TMultiGraph ("mg_" + name, title);
  
  for (int i = 0; i < files.size(); i++)
  {
    auto graph = (TGraph*) files [i] -> Get (object_name);
    if (!graph) continue;
    graph -> SetName (name + Form ("_%d", i));
    graph -> SetTitle (labels [i]);
    
    graph -> SetLineWidth (2);
    graph -> SetLineColor  (colors [i]);
    graph -> SetMarkerColor (colors [i]);
    graph -> SetMarkerStyle (markerStyles.at(0).at(i));
    graph -> SetFillColor (0);
    mg -> Add (graph);
  }
          
  gPad -> SetRightMargin (0.2);
  mg -> Draw ("apl");
  if (xRangeSet) mg -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
  if (yRangeSet) mg -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
  gPad -> BuildLegend (0.81, 0.0, 1.0, 1.0);
  c -> SetLogx (logX);
  c -> SetLogy (logY);
  if (save_pdf) 
    c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx"));
	if (save_png) 
		c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
  if (save_root)
	{  
    output_file -> cd();
    c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
  }
	
  if (plot_ratio)
  {
    mg -> SetName ("mg_" + name + "_ratio");
    mg -> SetTitle (title + ": ratio to " + labels [0]);
    c -> SetName (Form ("%s_ratio", c -> GetName()));
    c -> SetTitle (Form ("%s: ratio to %s", c -> GetTitle(), labels [0].Data()));
    for (auto graph : *(mg -> GetListOfGraphs()))
    {
      if (! DivideGraphs ((TGraph*) graph, ref_graph))
        break;
    }
    
    mg -> SetMinimum (-4.);
    mg -> SetMaximum (4.);
    if (save_pdf) 
      c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","tx") + "_ratio");
		if (save_png) 
			c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
    if (save_root) 
      c -> Write (((TString)c->GetName()).ReplaceAll ("/", "_"));
    delete mg;
  }
  delete c;
}

void PlotTH2 (TString object_name)
{
  TLatex *text = new TLatex();
  text -> SetNDC();
  text -> SetTextSize(0.055);
  text -> SetTextFont(42);
  vector <TH2*> hists;
  
  auto ref_obj = files [0] -> Get (object_name) -> Clone("htemp");
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
  
  int nEntries = 0;
  float sumMean = 0;
  float sumError = 0;
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  TCanvas *c = new TCanvas ("c_" + title, title);
  c -> cd();
  text -> DrawLatex (0.1, 0.95, title);
  TPad *c1 = new TPad ("c1", "c1", 0., 0., 1., 0.94);
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
    gPad -> SetTopMargin (0.06);
    gPad -> SetLogx (logX);
    gPad -> SetLogy (logY);
    gPad -> SetLogz (logZ);

    hist = (TH2*) files [i] -> Get (object_name);
    if (!hist) continue;
    hist -> SetName (name + Form ("_%d", i));
    hist -> SetTitle (labels [i]);
    
    if (!hist -> InheritsFrom ("TProfile2D") && rescale)
    {
      float scale_factor;
      if (hist -> GetSumOfWeights() != 0) 
//        scale_factor = 1.0 * ref_hist -> GetEntries() / hist -> GetEntries ();
        scale_factor = 1.0 * ref_hist -> Integral ("width") / hist -> Integral ("width");
      else scale_factor = 1.0;
      hist -> Scale (scale_factor);
    }
  
    nEntries += hist -> GetEntries(); 
    sumMean += hist -> GetMean(1) + hist -> GetMean(2) + hist -> GetMean(3); 
    sumError += hist -> GetMeanError(1) + hist -> GetMeanError(2) + hist -> GetMeanError(3); 
    
    hist -> Draw ("colz");
    if (xRangeSet) hist -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
    if (yRangeSet) hist -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
    if (zRangeSet) hist -> GetZaxis() -> SetRangeUser(zRange.at(0), zRange.at(1));
    gPad -> Modified();
    gPad -> Update();
    TPaveStats *stats = (TPaveStats*) gPad -> GetPrimitive ("stats");
    if (stats) {
      stats -> SetName (Form ("stats_%d", i));
      stats -> SetTextColor (colors [i]);
      stats -> SetX1NDC (.7);
      stats -> SetX2NDC (.9);
      stats -> SetY1NDC (.7);
      stats -> SetY2NDC (.94);
      TText *statTitle = stats -> GetLineWith (hist -> GetName());
      statTitle -> SetText (0, 0, hist -> GetTitle());
      hist -> SetStats (0);
      hists.push_back (hist);
    }
  }
  if (saveEmpty || (!saveEmpty && (nEntries > 0 || sumMean > 0. || sumError > 0.)))
  {
    if (save_pdf)
      c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","tx"));
    if (save_png) 
      c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
    if (save_root)
    { 
      output_file -> cd();
      c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
    }
    if (plot_ratio)
    {
      c -> SetName (Form("%s_ratio", c -> GetName()));
      c -> SetTitle (Form ("%s: ratio to %s", c -> GetTitle(), labels [0].Data()));
      c -> cd();
  //    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels [0]);
      for (int i = 0; i < labels.size(); i++)
      {
        c1 -> cd (i + 1);
        gPad -> SetLogz (0);
        hist = (TH2*) gPad -> GetPrimitive (name + Form ("_%d", i));
        if (!hist) continue;
  //      hist -> Sumw2()
        hist -> Divide (ref_hist);
        hist -> GetZaxis() -> SetRangeUser (0., 4.);
      }
      gPad -> Update();
      if (save_pdf) 
        c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx") + "_ratio");
      if (save_png) 
        c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
      if (save_root)
        c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
    }
  }
        
  delete ref_hist;
  for (auto hist : hists)
    delete hist;
  delete c1;
  delete c;
}

void PlotMultiGraph (TString object_name)
{
  TLatex *text = new TLatex();
  text -> SetNDC();
  text -> SetTextSize (0.055);
  text -> SetTextFont (42);
  vector <TMultiGraph*> multiGraphs;
  
  TMultiGraph *mg;
  auto mg_ref = (TMultiGraph*) files [0] -> Get (object_name) -> Clone("htemp");
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  
  TLegend *leg = new TLegend (0.,0.,1.,1.);
  leg -> SetTextSize(lts);
  TList *glist = mg_ref -> GetListOfGraphs();
  for (auto object : *glist)
    leg -> AddEntry (object, object -> GetTitle(),"pl");
  
  TCanvas *c = new TCanvas ("c_" + title, title);
  c -> cd();
  text -> DrawLatex (0.1, 0.95, title);
  
  TPad *c0 = new TPad ("c0", "c0", 0.81, 0., 1., 0.94);
  c0 -> Draw ();
  c0 -> cd ();
  leg -> Draw ();
  
  c -> cd();
  TPad *c1 = new TPad ("c1", "c1", 0., 0., .8, 0.94);
  c1 -> Draw();
  c1 -> cd();
  int npads = labels.size();
  int npadsx = int (ceil (sqrt (npads)));
  int npadsy = int (ceil (1. * npads / npadsx));
  c1 -> Divide (npadsx, npadsy);
  
  for (int i = 0; i < files.size(); i++)
  {
    c1 -> cd (i + 1);
    gPad -> SetLeftMargin (0.1);
    gPad -> SetRightMargin (0.);
    gPad -> SetTopMargin (0.1);
    gPad -> SetLogx (logX);
    gPad -> SetLogy (logY);

    mg = (TMultiGraph*) files [i] -> Get (object_name);
    if (!mg) continue;
    mg -> Draw ("apl");
    TString xAxisTitle = mg -> GetHistogram() -> GetXaxis() -> GetTitle();
    TString yAxisTitle = mg -> GetHistogram() -> GetYaxis() -> GetTitle();
    if (xRangeSet) mg -> /*GetHistogram() -> */GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
    if (yRangeSet) mg -> /*GetHistogram() -> */GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
    mg -> SetTitle (labels [i]);
    gPad -> Update();
    TPaveText *p = (TPaveText*) gPad -> FindObject ("title");
    if (p) 
    {
      p -> Clear();
      p -> InsertLine ();
      p -> InsertText (labels [i]);
      p -> SetTextSize (0.05);
    }
    mg -> SetName (name + Form ("_%d", i));
    mg -> /*GetHistogram() -> */GetXaxis() -> SetTitle(xAxisTitle);
    mg -> /*GetHistogram() -> */GetYaxis() -> SetTitle(yAxisTitle);
    multiGraphs.push_back (mg);
  }
  
  if (save_pdf)
    c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","tx"));
	if (save_png)
		c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
  if (save_root)
  { 
    output_file -> cd();
    c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
  }
  if (plot_ratio)
  {
    c -> SetName (Form("%s_ratio", c -> GetName()));
    c -> cd();
    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels [0]);
    for (int i = 0; i < labels.size(); i++)
    {
      c1 -> cd (i + 1);
      mg = (TMultiGraph*) gPad -> GetPrimitive (name + Form ("_%d", i));
      if (!mg) continue;
      if (! DivideMultiGraphs (mg, mg_ref)) 
        break;
    }
    if (save_pdf) 
      c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx") + "_ratio");
		if (save_png) 
			c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
    if (save_root)
      c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
  }
        
  delete mg_ref;
  for (auto mg : multiGraphs)
    delete mg;
  delete c0;
  delete c1;
  delete c;
}

void Plot2MultiGraphs (TString object_name)
{
  TLatex *text = new TLatex();
  text -> SetNDC();
  text -> SetTextSize (0.055);
  text -> SetTextFont (42);
  
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  TMultiGraph *mg;
  TCanvas *c = new TCanvas ("c_" + title, title);
  c -> cd();
  cout << object_name << endl;
  TMultiGraph *mg_ref = (TMultiGraph*) files [0] -> Get (object_name) -> Clone("htemp");
  mg_ref -> Draw();
  TString xAxisTitle = mg_ref->GetXaxis()->GetTitle();
  TString yAxisTitle = mg_ref->GetYaxis()->GetTitle();
  TMultiGraph *mg_common = new TMultiGraph ("mg_" + name, (TString)mg_ref->GetTitle() + ";" + xAxisTitle + ";" + yAxisTitle);
  
  TLegend *leg = new TLegend (0.81,0.,1.,1.);
  leg -> SetTextSize (lts);
  TList *glist = mg_ref -> GetListOfGraphs();
  for (auto g : *glist)
    leg -> AddEntry (g, g -> GetTitle(),"l");
  vector <TH1F> g_fake(2);
  
  text -> DrawLatex (0.1, 0.95, title);
  for (int i = 0; i < files.size(); i++)
  {
    gPad -> SetLeftMargin (0.1);
    gPad -> SetRightMargin (0.2);
    gPad -> SetTopMargin (0.1);

    g_fake.at(i).SetLineColor(kBlack);
    g_fake.at(i).SetLineStyle(lineStyles.at(i));
    leg -> AddEntry (&g_fake.at(i), labels.at(i), "l");

    mg = (TMultiGraph*) files [i] -> Get (object_name);
    if (!mg) continue;
    glist = mg -> GetListOfGraphs ();
    if (!glist) continue;
    for (int j = 0; j < glist -> GetSize(); j++)
    {
      TGraphAsymmErrors* g = (TGraphAsymmErrors*) glist -> At(j);
      g -> SetLineStyle(lineStyles.at(i));
      g -> SetMarkerStyle(markerStyles.at(i).at(j));
      mg_common -> Add(g,g->GetOption());
    }
//    TPaveText *p = (TPaveText*) gPad -> FindObject ("title");
//    if (p) 
//    {
//      p -> Clear();
//      p -> InsertLine ();
//      p -> InsertText (labels [i]);
//      p -> SetTextSize (0.05);
//    }
  }
  mg_common -> Draw ("apl");
  leg -> Draw("same");
  if (xRangeSet) mg_common -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
  if (yRangeSet) mg_common -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
  gPad -> SetLogx (logX);
  gPad -> SetLogy (logY);
  
  if (save_pdf)
    c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","tx"));
	if (save_png)
		c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
  if (save_root)
  { 
    output_file -> cd();
    c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
  }
  if (plot_ratio)
  {
    c -> SetName (Form("%s_ratio", c -> GetName()));
    c -> cd();
    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels [0]);
    mg = (TMultiGraph*) files [0] -> Get (object_name);
    if (! mg && ! DivideMultiGraphs (mg, mg_ref))
    {
      mg -> Draw ();
      leg -> Draw("same");
      if (save_pdf) 
        c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx") + "_ratio");
      if (save_png) 
        c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
      if (save_root)
        c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
    }
  }
        
  //delete mg;
  //delete mg_ref;
  //delete mg_common;
  delete c;
}

void PlotTHStack (TString object_name)
{
  TLatex *text = new TLatex();
  text -> SetNDC();
  text -> SetTextSize (0.055);
  text -> SetTextFont (42);
  vector <THStack*> stacks;
  
  THStack *hs;
  auto hs_ref = (THStack*) files [0] -> Get (object_name) -> Clone("htemp");
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  
  TLegend *leg = new TLegend (0.,0.,1.,1.);
  leg -> SetTextSize(0.1);
  TList *hslist = hs_ref -> GetHists();
  for (auto object : *hslist)
    leg -> AddEntry (object, object -> GetTitle(),"pl");
  
  TCanvas *c = new TCanvas ("c_" + title, title);
  c -> cd();
  text -> DrawLatex (0.1, 0.95, title);
  
  TPad *c0 = new TPad ("c0", "c0", 0.81, 0., 1., 0.94);
  c0 -> Draw ();
  c0 -> cd ();
  leg -> Draw ();
  
  c -> cd();
  TPad *c1 = new TPad ("c1", "c1", 0., 0., .8, 0.94);
  c1 -> Draw();
  c1 -> cd();
  int npads = labels.size();
  int npadsx = int (ceil (sqrt (npads)));
  int npadsy = int (ceil (1. * npads / npadsx));
  c1 -> Divide (npadsx, npadsy);
  
  for (int i = 0; i < files.size(); i++)
  {
    c1 -> cd (i + 1);
    gPad -> SetLeftMargin (0.1);
    gPad -> SetRightMargin (0.);
    gPad -> SetTopMargin (0.1);

    hs = (THStack*) files [i] -> Get (object_name);
    if (!hs) continue;
    hs -> Draw ("nostack");    
    TString xAxisTitle = hs -> GetHistogram() -> GetXaxis() -> GetTitle();
//    TString yAxisTitle = hs -> GetYaxis() -> GetTitle();
    hs -> SetTitle (labels [i]);
    TPaveText *p = (TPaveText*) gPad -> FindObject ("title");
    if (p) 
    {
      p -> Clear();
      p -> InsertLine ();
      p -> InsertText (labels [i]);
      p -> SetTextSize (0.05);
    }
    hs -> SetName (name + Form ("_%d", i));
    hs -> GetHistogram() /*-> GetXaxis()*/ -> SetTitle(xAxisTitle);
    if (xRangeSet) hs -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
    if (yRangeSet) hs -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
    gPad -> SetLogx (logX);
    gPad -> SetLogy (logY);
    stacks.push_back (hs);
  }
  
  if (save_pdf)
    c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","tx"));
	if (save_png)
		c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
  if (save_root)
  { 
    output_file -> cd();
    c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
  }
  if (plot_ratio)
  {
    c -> SetName (Form("%s_ratio", c -> GetName()));
    c -> cd();
    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels [0]);
    for (int i = 0; i < labels.size(); i++)
    {
      c1 -> cd (i + 1);
      hs = (THStack*) gPad -> GetPrimitive (name + Form ("_%d", i));
      if (!hs) continue;
      if (! DivideTHStacks (hs, hs_ref)) 
        break;
    }
    if (save_pdf) 
      c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx") + "_ratio");
		if (save_png) 
			c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
    if (save_root)
      c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
  }
        
  delete hs_ref;
  for (auto hs : stacks)
    delete hs;
  delete c0;
  delete c1;
  delete c;
}

void Plot2THStacks (TString object_name)
{
  TLatex *text = new TLatex();
  text -> SetNDC();
  text -> SetTextSize (0.055);
  text -> SetTextFont (42);
  
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  THStack *hs;
  TCanvas *c = new TCanvas ("c_" + title, title);
  c -> cd();
  auto hs_ref = (THStack*) files [0] -> Get (object_name) -> Clone("htemp");
  hs_ref -> Draw();
  TString xAxisTitle = hs_ref->GetXaxis()->GetTitle();
  TString yAxisTitle = hs_ref->GetYaxis()->GetTitle();
  auto *hs_common = new THStack ("hs_" + name, (TString)hs_ref->GetTitle() + ";" + xAxisTitle + ";" + yAxisTitle);
  
  TLegend *leg = new TLegend (0.81,0.,1.,1.);
  leg -> SetTextSize (lts);
  TList *hslist = hs_ref -> GetHists();
  vector <TH1F> h_fake(2);
  
  text -> DrawLatex (0.1, 0.95, title);
  for (int i = 0; i < files.size(); i++)
  {
    gPad -> SetLeftMargin (0.1);
    gPad -> SetRightMargin (0.2);
    gPad -> SetTopMargin (0.1);

    h_fake.at(i).SetLineColor(kBlack);
    h_fake.at(i).SetLineStyle(lineStyles.at(i));
    leg -> AddEntry (&h_fake.at(i), labels.at(i), "l");

    hs = (THStack*) files [i] -> Get (object_name);
    if (!hs) continue;
    hslist = hs -> GetHists ();
    for (int j = 0; j < hslist -> GetSize(); j++)
    {
      TH1* h = (TH1*) hslist -> At(j);
      h -> SetLineStyle(lineStyles.at(i));
      h -> SetMarkerStyle(markerStyles.at(i).at(j));
      hs_common -> Add(h,h->GetOption());
    }
//    TPaveText *p = (TPaveText*) gPad -> FindObject ("title");
//    if (p) 
//    {
//      p -> Clear();
//      p -> InsertLine ();
//      p -> InsertText (labels [i]);
//      p -> SetTextSize (0.05);
//    }
  }
  hs_common -> Draw ("NOSTACK");
  for (auto h : *hslist)
    leg -> AddEntry (h, h -> GetTitle(),"l");
  leg -> Draw("same");
  if (xRangeSet) hs_common -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
  if (yRangeSet) hs_common -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
  gPad -> SetLogx (logX);
  gPad -> SetLogy (logY);
  
  if (save_pdf)
    c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex","tx"));
	if (save_png)
		c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
  if (save_root)
  { 
    output_file -> cd();
    c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
  }
  if (plot_ratio)
  {
    c -> SetName (Form("%s_ratio", c -> GetName()));
    c -> cd();
    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels [0]);
    hs = (THStack*) files [0] -> Get (object_name);
    if (! hs && ! DivideTHStacks (hs, hs_ref))
    {
      hs -> Draw ("NOSTACK");
      leg -> Draw("same");
      if (save_pdf) 
        c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx") + "_ratio");
      if (save_png) 
        c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
      if (save_root)
        c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
    }
  }
        
  delete hs;
  delete hs_ref;
  delete hs_common;
  delete c;
}

bool DivideGraphs (TGraph* graph, TGraph* graph_ref) 
{
  if (graph -> GetN() != graph_ref -> GetN())
  {
    cout << "Error while dividing graphs: different number of points!";
    return false;
  }
  
  int nBins = graph -> GetN();
  double x, y, x_ref, y_ref, yErr, yErr_ref, yErrLow, yErrHigh, yErrLow_ref, yErrHigh_ref;
  
  for (int i = 0; i < nBins; i++)
  {
    graph -> GetPoint (i, x, y);
    graph_ref -> GetPoint (i, x_ref, y_ref);
    graph -> SetPoint (i, x, y / y_ref);
    if (graph -> InheritsFrom ("TGraphAsymmErrors"))
    {
      yErrLow = graph -> GetErrorYlow (i);
      yErrHigh = graph -> GetErrorYhigh (i);
      yErrLow_ref = graph_ref -> GetErrorYlow (i);
      yErrHigh_ref = graph_ref -> GetErrorYhigh (i);
      yErrLow = sqrt (pow (yErrLow / y_ref, 2.) + pow (yErrLow_ref * y / y_ref / y_ref, 2.));
      yErrHigh = sqrt (pow (yErrHigh / y_ref, 2.) + pow (yErrHigh_ref * y / y_ref / y_ref, 2.));
      ((TGraphAsymmErrors*) graph) -> SetPointError (i, graph -> GetErrorXlow (i), graph -> GetErrorXhigh (i), yErrLow, yErrHigh);
    }
    else if (graph -> InheritsFrom ("TGraphErrors"))
    {
      yErr = graph -> GetErrorY (i);
      yErr_ref = graph_ref -> GetErrorY (i);
      yErr = sqrt (pow (yErr / y_ref, 2.) + pow (yErr_ref * y / y_ref / y_ref, 2.));
      ((TGraphErrors*) graph) -> SetPointError (i, graph -> GetErrorX (i), yErr);
    }
  }
  return true;
}

bool DivideMultiGraphs (TMultiGraph* mg, TMultiGraph *mg_ref)
{
  auto glist = mg -> GetListOfGraphs ();
  auto glist_ref = mg_ref -> GetListOfGraphs ();
  
  if (glist -> GetSize() != glist_ref -> GetSize())
  {
    cout << "Error while dividing multigraphs: different number of graphs!";
    return false;
  }
  
  int nGraphs = glist -> GetSize();
  for (int i = 0; i < nGraphs; i++)
    if (! DivideGraphs ((TGraphErrors*) glist -> At (i), (TGraphErrors*) glist_ref -> At (i)))
      return false;
  
  mg -> SetMinimum (-4.);
  mg -> SetMaximum (4.);
  
  return true;
}

bool DivideTHStacks (THStack* hs, THStack *hs_ref)
{
  auto hslist = hs -> GetHists ();
  auto hslist_ref = hs_ref -> GetHists ();
  
  if (hslist -> GetSize() != hslist_ref -> GetSize())
  {
    cout << "Error while dividing THStacks: different number of histograms!";
    return false;
  }
  
  int nHists = hslist -> GetSize();
  for (int i = 0; i < nHists; i++)
    ((TH1*) hslist -> At (i)) -> Divide ((TH1*) hslist_ref -> At (i));
  
  hs -> SetMinimum (-4.);
  hs -> SetMaximum (4.);
  
  return true;
}



void FilterObjectList() {
  if (!gUseIncludePattern) {
    return;
  }

  regex re(gIncludePattern);

  decltype(object_names) objectsFiltered;

  copy_if(object_names.begin(), object_names.end(), back_inserter(objectsFiltered), [=] (const TString &name) {
    return regex_match(&name.Data()[0], &name.Data()[name.Length()], re);
  });

  swap(objectsFiltered, object_names);

  return;
}
