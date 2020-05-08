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
#include <limits>

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
float ratioPadSize = 0.3;

vector <TString> inputFileNames; 
vector <TString> excludedFolders;
vector <TString> labels;
TString folderName;
TString th1option = "e";
TString th2option = "colz";
TString thStackOption = "nostack";
TString graphOption = "pl";
TString multiGraphOption = "apl";
int refLineStyle = 3;
int maxDepth = 100;
bool rescale = true;
bool plot_ratio = false;
bool save_root = true;
bool save_png = false;
bool save_pdf = true;
bool saveEmpty = false;
bool plotLegend = false;
bool plotTitle = true;
vector <TFile*> files;
vector <TDirectory*> dirs;
vector <TString> object_names;
TString outputPath = "comp"; 
TString outputPathPdf = "comp.pdf";
bool xRangeSet = false;
bool yRangeSet = false;
bool zRangeSet = false;
bool ratioRangeSet = false;
vector <float> xRange;
vector <float> yRange;
vector <float> zRange;
vector <float> ratioRange;
bool logX, logY,logX2d, logY2d, logZ;
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
void GetRangeY (vector <TH1*> hists, vector <float> &range, bool logY = false);
void GetRangeY (vector <TGraph*> graphs, vector <float> &range, bool logY = false);

int main (int argc, char* argv[])
{
  gErrorIgnoreLevel = 2000;
  gStyle  -> SetOptStat (111111);
  gStyle -> SetOptFit (1);
//  gStyle -> SetTitleAlign (33);
  gStyle -> SetLegendBorderSize (0);
  gStyle -> SetLegendTextSize (lts);
  gStyle->SetStatStyle (0);
  gStyle->SetOptTitle(0);

  if(!parseArgs (argc, argv))
    return -1;
  
  if (save_png)
    gSystem -> Exec("mkdir -p " + outputPath); 
		
  if (save_root) 
    output_file = new TFile (outputPath + ".root", "recreate");
  
  for (TString inputFileName : inputFileNames)
  {
    files.push_back(new TFile (inputFileName, "read"));
    dirs.push_back(files.back() -> GetDirectory (folderName));
  }
   
  BuildObjectList (dirs.at(0));
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
    text -> DrawLatex(0.1, ypos, inputFileNames.at(i) + " (" + labels.at(i) + ")");
  }
  if (save_pdf) c -> Print (outputPathPdf + "(","Title:Title");
      
  for (auto object_name : object_names)
  {
    while (object_name.BeginsWith ("/"))
      object_name.Remove (0,1);
    cout << object_name << endl;
    
    TObject *object = files.at(0) -> Get(object_name);
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
    ("pattern,p", value<string>(&gIncludePattern),
        "include only objects matching pattern e.g. \".*psd[1-3]_psd[1-3]_(X|Y){2}\"")
    ("output,o", value<TString>()->default_value("comp.root"),"Output file")
    ("folder,f", value<TString>()->default_value("/"), "Directory to compare")
    ("depth,d", value<int>()->default_value(10), "Maximum depth of folder search")
    ("ratio,r", value<bool>()->implicit_value(true)->default_value(false), "Plot histogram ratio")
    ("xrange,x", value< vector<float> >()->multitoken(),"X axis range") 
    ("yrange,y", value< vector<float> >()->multitoken(),"Y axis range")
    ("zrange,z", value< vector<float> >()->multitoken(),"Z axis range")
    ("ratio-range", value< vector<float> >()->multitoken(),"Ratio range")
    ("logx", value<bool>()->implicit_value(true)->default_value(false), "SetLogX()")
    ("logx2d", value<bool>()->implicit_value(true)->default_value(false), "SetLogX() for 2D plots")
    ("logy", value<bool>()->implicit_value(true)->default_value(false), "SetLogY()")
    ("logy2d", value<bool>()->implicit_value(true)->default_value(false), "SetLogY() for 2D plots")
    ("logz", value<bool>()->implicit_value(false)->default_value(true), "SetLogZ(0)")
    ("legend", value<bool>()->implicit_value(true)->default_value(false), "Plot legend instead of stats for TH1")
    ("lts", value<float>()->default_value(0.03), "Legend Text Size")
    ("opt1", value<TString>()->default_value("e"), "TH1 drawing option")
    ("opt2", value<TString>()->default_value("colz"), "TH2 drawing option")
    ("optG", value<TString>()->default_value("a"), "TGraph drawing option")
    ("optMG", value<TString>()->default_value("apl"), "TMultiGraph drawing option")
    ("optHS", value<TString>()->default_value("nostack"), "THStack drawing option")
    ("no-rescale", value<bool>()->implicit_value(false)->default_value(true), "Do not rescale histograms")
    ("save-empty", value<bool>()->implicit_value(true)->default_value(false), "Save empty and zero histograms")
    ("no-pdf", value<bool>()->implicit_value(false)->default_value(true), "Do not write output to PDF file")
    ("no-root", value<bool>()->implicit_value(false)->default_value(true), "Do not write output to ROOT file")
    ("png", value<bool>()->implicit_value(true)->default_value(false), "Write output to png files")
  ;
  
  variables_map args;
  store(parse_command_line(argc, argv, desc), args);

  if (args.count("help")) {  
      cout << desc << "\n";
      return false;
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
  logX2d = args ["logx2d"].as <bool> ();
  logY = args ["logy"].as <bool> ();
  logY2d = args ["logy2d"].as <bool> ();
  logZ = args ["logz"].as <bool> ();
  plotLegend = args ["legend"].as <bool> ();
  lts = args ["lts"].as <float> ();
  th1option = args ["opt1"].as <TString> ();
  th2option = args ["opt2"].as <TString> ();
  thStackOption = args ["optHS"].as <TString> ();
  graphOption = args ["optG"].as <TString> ();
  multiGraphOption = args ["optMG"].as <TString> ();
  
  if (args.count ("xrange")) xRange = args ["xrange"].as <vector <float> > ();
  if (args.count ("yrange")) yRange = args ["yrange"].as <vector <float> > ();
  if (args.count ("zrange")) zRange = args ["zrange"].as <vector <float> > ();
  if (args.count ("ratio-range")) ratioRange = args ["ratio-range"].as <vector <float> > ();
  
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
  if (ratioRange.size() >= 2)
  {
    if (ratioRange.at(1) > ratioRange.at(0)) ratioRangeSet = true;
    else cout << "WARNING: upper ratio range is smaller than the lower one.\n Switching to default.\n";
  } 
  
  if (args.count ("exclude")) excludedFolders = args ["exclude"].as <vector <TString> > (); 
  
  if (args.count ("labels")) labels = args ["labels"].as <vector <TString> > (); 
  if (labels.size() > 0 && labels.size() != inputFileNames.size())
  {
    cout << "Error! Number of labels provided is other than number of input files!\n";
    return false;
  }

  if (args.count("pattern"))
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

  if (labels.size() < 2) 
    plot_ratio = false;

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
    if (!className.Contains ("3") && 
      (className.BeginsWith ("TH") || className.BeginsWith ("TProfile") || className.Contains ("Graph")))
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
  vector <TH1*> hists (files.size()), rhists;
  for (uint i = 0; i < hists.size(); i++)
    hists.at(i) = (TH1*) files.at(i) -> Get (object_name);
  auto ref_obj = hists.at(0)->Clone("htemp");
  TProfile *ref_P1D = nullptr;
  TH1 *ref_hist = nullptr;
  TH1 *hist = nullptr;
  
  if (nullptr != (ref_P1D = dynamic_cast <TProfile*> (ref_obj)))
  {
    ref_hist = ref_P1D -> ProjectionX();
    delete ref_obj;
  }
  else
    ref_hist = dynamic_cast<TH1*> (ref_obj);  

  ref_hist -> Sumw2();
  
  if (!yRangeSet) 
    GetRangeY(hists, yRange, logY);

  int nEntries = 0;
  float sumMean = 0;
  float sumError = 0;
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  TCanvas *c = new TCanvas ("c_" + name, title);
  c -> SetRightMargin (0.2);
  c -> cd ();
  TPad *c1; 
  TLine *line;
  
  for (int i = 0; i < hists.size(); i++)
  {
    hist = hists.at(i);
    if (!hist) continue;
    hist -> SetName (name + Form ("_%d", i));
    hist -> SetTitle (labels.at(i));
        
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
    hist -> SetLineColor  (colors.at(i));
    hist -> SetMarkerColor (colors.at(i));
    TString option;
    if (i == 0) option = th1option;
    else option = "sames " + th1option;
    hist -> Draw (option);
    gPad -> Modified ();
    gPad -> Update ();
    auto stats = (TPaveStats*) hist -> GetListOfFunctions() -> FindObject("stats");
    if (stats)
    {
      stats -> SetName (Form ("stats_%d", i));
      stats -> SetTextColor (colors.at(i));
      stats -> SetX1NDC (0.8);
      stats -> SetX2NDC (1.0);
      float y_offset = 0.1 * i;
      stats -> SetY1NDC (0.9 - y_offset);
      stats -> SetY2NDC (1.0 - y_offset);
      TText *statTitle = stats -> GetLineWith (hist->GetName());
      statTitle -> SetText (0, 0, labels.at(i));
      hist -> SetStats(0);
    }
  }
  
  if (saveEmpty || (!saveEmpty && (nEntries > 0 || sumMean > 0. || sumError > 0.)))
  {
    if (plotLegend)
      gPad -> BuildLegend (0.8011, ratioPadSize, 1.0, 1.0);
    if (xRangeSet) hists.at(0) -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
    hists.at(0) -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
    c -> SetLogx (logX);
    c -> SetLogy (logY);
    
    if (plotTitle)
    {
      TLatex *text = new TLatex();
      text -> SetNDC();
      text -> SetTextSize (0.055);
      text -> SetTextFont (42); 
      text -> DrawLatex (0.1, 0.95, title);
    }

    if (plot_ratio)
    {
      c->SetBottomMargin(ratioPadSize);
      c1 = new TPad (Form ("%s_ratio", c -> GetName()),"",0.,0.,1.,ratioPadSize);
      c1 -> SetTopMargin(0.);
      c1 -> SetRightMargin(0.2);
      c1 -> SetBottomMargin(0.1 / ratioPadSize);
      c1 -> Draw();
      TString yAxisTitle = "ratio";
      c1->cd();
      for (uint i = 1; i < hists.size(); i++)
      {
	auto hist = (TH1*)hists.at(i) -> Clone(Form("%s_ratio", hists.at(i) -> GetName())); 
	hist -> SetTitle (Form("%s_ratio", hists.at(i) -> GetTitle()));
        hist -> Divide (ref_hist);
	hist -> SetStats (0);
	hist -> Draw("same " + th1option);
	TObject *stats = hist -> GetListOfFunctions() -> FindObject(Form ("stats_%d", i));
        if (stats) stats -> Delete();
        rhists.push_back(hist);
      }
      if (!ratioRangeSet) 
	GetRangeY(rhists, ratioRange);
      
      TAxis *xAxis = rhists.at(0) -> GetXaxis();
      xAxis -> SetTitleSize(xAxis -> GetTitleSize() / ratioPadSize);
      xAxis -> SetLabelSize(xAxis -> GetLabelSize() / ratioPadSize);
      xAxis -> SetTickLength(xAxis -> GetTickLength() / ratioPadSize);
      if (xRangeSet) xAxis -> SetRangeUser(xRange.at(0), xRange.at(1));
      
      TAxis *yAxis = rhists.at(0) -> GetYaxis();
      yAxis -> SetTitle (yAxisTitle);
      yAxis -> SetTitleSize(yAxis -> GetTitleSize() / ratioPadSize);
      yAxis -> SetTitleOffset(0.1 / ratioPadSize);
      yAxis -> SetLabelSize(yAxis -> GetLabelSize() / ratioPadSize);
      yAxis -> SetRangeUser (ratioRange.at(0), ratioRange.at(1));
      yAxis -> SetNdivisions (505);

      line = new TLine (xAxis->GetXmin(),1,xAxis->GetXmax(),1);
      line->SetLineStyle(refLineStyle);
      line->Draw();
    }
    if (save_pdf) 
      c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx"));
    if (save_png) 
      c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
    if (save_root)
    {
      output_file -> cd();
      c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
    }
  }  
  delete ref_hist;
  for (auto hist : hists)
    delete hist;
  if (plot_ratio) 
  {
    for (auto hist : rhists)
      delete hist;
    delete line;
  }
}


void PlotGraph (TString object_name)
{
  vector <TGraph*> graphs(files.size()), rgraphs;
  for (uint i = 0; i < files.size(); i++)
    graphs.at(i) = (TGraph*) files.at(i)->Get(object_name); 
  auto ref_graph = graphs.at(0);
  
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
 
  TCanvas *c = new TCanvas ("c_" + title, title);
  TPad *c1;
  TLine *line;
  c -> SetRightMargin (0.2);
  c -> cd();

  if (!yRangeSet) 
    GetRangeY (graphs, yRange, logY);
  
  for (int i = 0; i < graphs.size(); i++)
  {
    auto graph = graphs.at(i);
    if (!graph) continue;
    graph -> SetName (name + Form ("_%d", i));
    graph -> SetTitle (labels.at(i));
    
    graph -> SetLineWidth (2);
    graph -> SetLineColor  (colors.at(i));
    graph -> SetMarkerColor (colors.at(i));
    graph -> SetMarkerStyle (markerStyles.at(0).at(i));
    graph -> SetFillColor (0);
    TString option;
    if (i == 0) option = "a" + graphOption;
    else option = "same " + graphOption;
    graph -> Draw (option);
  }
          
  if (xRangeSet) graphs.at(0) -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
  graphs.at(0) -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
  gPad -> BuildLegend (0.8011, ratioPadSize, 1.0, 1.0);
  c -> SetLogx (logX);
  c -> SetLogy (logY);
  
  if (plotTitle)
  {
    TLatex *text = new TLatex();
    text -> SetNDC();
    text -> SetTextSize (0.055);
    text -> SetTextFont (42); 
    text -> DrawLatex (0.1, 0.95, title);
  }
	
  if (plot_ratio)
  {
    c->SetBottomMargin(ratioPadSize);
    c1 = new TPad (Form ("%s_ratio", c -> GetName()),"",0.,0.,1.,ratioPadSize);
    c1 -> SetTopMargin(0.);
    c1 -> SetRightMargin(0.2);
    c1 -> SetBottomMargin(0.1 / ratioPadSize);
    c1 -> Draw();
    TString yAxisTitle = "ratio";
    c1->cd();
    for (uint i = 1; i < graphs.size(); i++)
    {
      TGraph *graph;
      if(graphs.at(i))
	graph = (TGraph*) graphs.at(i) -> Clone(Form("%s_ratio", graphs.at(i) -> GetName()));
      if (! DivideGraphs (graph, ref_graph))
        continue;
      graph -> SetTitle (Form("%s_ratio", graphs.at(i) -> GetTitle()));
      rgraphs.push_back(graph);
      TString option;
      if (i == 1) option = "a" + graphOption;
      else option = "same" + graphOption;
      graph -> Draw (option);
    }
    
    if (!ratioRangeSet) 
      GetRangeY(rgraphs, ratioRange);
    
    TAxis *xAxis = rgraphs.at(0) -> GetXaxis();
    xAxis -> SetTitleSize(xAxis -> GetTitleSize() / ratioPadSize);
    xAxis -> SetLabelSize(xAxis -> GetLabelSize() / ratioPadSize);
    xAxis -> SetTickLength(xAxis -> GetTickLength() / ratioPadSize);
    if (xRangeSet) xAxis -> SetRangeUser(xRange.at(0), xRange.at(1));
    
    TAxis *yAxis = rgraphs.at(0) -> GetYaxis();
    yAxis -> SetTitle (yAxisTitle);
    yAxis -> SetTitleSize(yAxis -> GetTitleSize() / ratioPadSize);
    yAxis -> SetTitleOffset(0.1 / ratioPadSize);
    yAxis -> SetLabelSize(yAxis -> GetLabelSize() / ratioPadSize);
    yAxis -> SetRangeUser (ratioRange.at(0), ratioRange.at(1));
    yAxis -> SetNdivisions (505);

    line = new TLine (xAxis->GetXmin(),1,xAxis->GetXmax(),1);
    line->SetLineStyle(refLineStyle);
    line->Draw();
  }
  
  if (save_pdf) c -> Print (outputPathPdf, "Title:" + title.ReplaceAll ("tex", "tx"));
  if (save_png) c -> Print (outputPath + "/" + title.ReplaceAll ("/", "_") + ".png");
  if (save_root)
  {  
    output_file -> cd();
    c -> Write(((TString)c->GetName()).ReplaceAll ("/", "_"));
  }
  
  delete c;
  for (auto graph:graphs)
    delete graph;
  if (plot_ratio)
  {
    for (auto graph:rgraphs)
      delete graph;
    delete line;
  }
}


void PlotTH2 (TString object_name)
{
  TLatex *text = new TLatex();
  text -> SetNDC();
  text -> SetTextSize(0.055);
  text -> SetTextFont(42);
  vector <TH2*> hists;
 
  for (auto file:files)
    hists.push_back((TH2*)file -> Get (object_name));
  auto ref_obj = hists.at(0) -> Clone("htemp");
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
    ref_hist -> Sumw2();
  
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
  if (npads == 3) 
  {
    npadsx = 3;
    npadsy = 1;
  }
  c1 -> Divide (npadsx, npadsy);
  
  for (int i = 0; i < files.size(); i++)
  {
    c1 -> cd (i + 1);
    gPad -> SetLogz ();
    gPad -> SetLeftMargin (0.15);
    gPad -> SetTopMargin (0.06);
    gPad -> SetLogx (logX2d);
    gPad -> SetLogy (logY2d);
    gPad -> SetLogz (logZ);

    hist = hists.at(i);
    if (!hist) continue;
    hist -> SetName (name + Form ("_%d", i));
    hist -> SetTitle (labels.at(i));
    
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
    
    hist -> Draw (th2option);
    if (xRangeSet) hist -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
    if (yRangeSet) hist -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
    if (zRangeSet) hist -> GetZaxis() -> SetRangeUser(zRange.at(0), zRange.at(1));
    gPad -> Modified();
    gPad -> Update();
    TPaveStats *stats = (TPaveStats*) gPad -> GetPrimitive ("stats");
    if (stats) {
      stats -> SetName (Form ("stats_%d", i));
      stats -> SetTextColor (colors.at(i));
      stats -> SetX1NDC (.7);
      stats -> SetX2NDC (.9);
      stats -> SetY1NDC (.7);
      stats -> SetY2NDC (.94);
      TText *statTitle = stats -> GetLineWith (hist -> GetName());
      statTitle -> SetText (0, 0, hist -> GetTitle());
      hist -> SetStats (0);
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
      c -> SetTitle (Form ("%s: ratio to %s", c -> GetTitle(), labels.at(0).Data()));
      c -> cd();
      text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels.at(0));
      for (int i = 0; i < labels.size(); i++)
      {
        c1 -> cd (i + 1);
        gPad -> SetLogz (0);
        hist = (TH2*) gPad -> GetPrimitive (name + Form ("_%d", i));
        if (!hist) continue;
  //      hist -> Sumw2()
        hist -> Divide (ref_hist);
        if (ratioRangeSet)
	  hist -> GetZaxis() -> SetRangeUser (ratioRange.at(0), ratioRange.at(1));
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
  auto mg_ref = (TMultiGraph*) files.at(0) -> Get (object_name) -> Clone("htemp");
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
  if (npads == 3) 
  {
    npadsx = 3;
    npadsy = 1;
  }
  c1 -> Divide (npadsx, npadsy);
  
  for (int i = 0; i < files.size(); i++)
  {
    c1 -> cd (i + 1);
    gPad -> SetLeftMargin (0.1);
    gPad -> SetRightMargin (0.);
    gPad -> SetTopMargin (0.1);
    gPad -> SetLogx (logX);
    gPad -> SetLogy (logY);

    mg = (TMultiGraph*) files.at(i) -> Get (object_name);
    if (!mg) continue;
    mg -> Draw (multiGraphOption);
    gPad->Modified(); gPad->Update();
    TString xAxisTitle = mg/* -> GetHistogram()*/ -> GetXaxis() -> GetTitle();
    TString yAxisTitle = mg/* -> GetHistogram()*/ -> GetYaxis() -> GetTitle();
    if (xRangeSet) mg -> /*GetHistogram() -> */GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
    if (yRangeSet) mg -> /*GetHistogram() -> */GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
    //if (xRangeSet) mg -> GetXaxis() -> SetLimits(xRange.at(0), xRange.at(1));
    //mg->SetMinimum(yRange.at(0));
    //mg->SetMaximum(yRange.at(1))
    mg -> SetTitle (labels.at(i));
    mg -> /*GetHistogram() -> */GetXaxis() -> SetTitle(xAxisTitle);
    mg -> /*GetHistogram() -> */GetYaxis() -> SetTitle(yAxisTitle);
    gPad->Modified(); gPad->Update();
    TPaveText *p = (TPaveText*) gPad -> FindObject ("title");
    if (p) 
    {
      p -> Clear();
      p -> InsertLine ();
      p -> InsertText (labels.at(i));
      p -> SetTextSize (0.05);
    }
    mg -> SetName (name + Form ("_%d", i));
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
    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels.at(0));
    for (int i = 0; i < labels.size(); i++)
    {
      c1 -> cd (i + 1);
      mg = (TMultiGraph*) gPad -> GetPrimitive (name + Form ("_%d", i));
      if (!mg) continue;
      if (! DivideMultiGraphs (mg, mg_ref)) 
        break;
      gPad->SetLogy(0);
      TLine line (mg->GetXaxis()->GetXmin(),1,mg->GetXaxis()->GetXmax(),1);
      line.SetLineStyle(refLineStyle);
      line.Draw();
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
  TString title = object_name;
  TString name = object_name;
  name.ReplaceAll ("/", "_");
  cout << object_name << endl;

  gStyle->SetOptTitle(0);
  vector <TMultiGraph*> mgs(2);
  vector <TList*> glists(2);
  vector <TGraph*> graphs, rgraphs;
  for (int i = 0; i < 2; i++)
  {
    mgs.at(i) = (TMultiGraph*) files.at(i) -> Get (object_name);
    if (mgs.at(i)) glists.at(i) = mgs.at(i) -> GetListOfGraphs();
  }
  
  if (glists.at(1) && glists.at(0)->GetSize() != glists.at(1)->GetSize())
  {
    cout << "Warning! Different number of graphs in multigraphs!/n";
    return;
  }

  TCanvas *c = new TCanvas ("c_" + title, title);
  TPad *c1;
  TLine *line;
  c -> cd();
  
  TLegend *leg = new TLegend (0.81,ratioPadSize,1.,1.);
  leg -> SetTextSize (lts);
  for (auto g : *glists.at(0))
    leg -> AddEntry (g, g -> GetTitle(),"l");
  vector <TH1F> g_fake(2);
  
  gPad -> SetRightMargin (0.2);
  for (int i = 0; i < 2; i++)
  {
    g_fake.at(i).SetLineColor(kBlack);
    g_fake.at(i).SetLineStyle(lineStyles.at(i));
    leg -> AddEntry (&g_fake.at(i), labels.at(i), "l");
    
    TList *glist = glists.at(i);
    if (!glist) continue;
    for (int j = 0; j < glist -> GetSize(); j++)
    {
      TGraph* g = (TGraph*) glist -> At(j);
      g -> SetLineStyle(lineStyles.at(i));
      g -> SetMarkerStyle(markerStyles.at(i).at(j));
      TString option;
      if (i == 0 && j == 0) option = "a" + graphOption;
      else option = "same " + graphOption;
      g -> Draw (option);
      graphs.push_back(g);
    }
  }
  leg -> Draw("same");
  if (!yRangeSet) 
    GetRangeY (graphs, yRange, logY); 
  if (xRangeSet) graphs.at(0) -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
  graphs.at(0) -> GetYaxis() -> SetRangeUser(yRange.at(0), yRange.at(1));
  gPad -> SetLogx (logX);
  gPad -> SetLogy (logY);
  
  if (plotTitle)
  {
    TLatex *text = new TLatex();
    text -> SetNDC();
    text -> SetTextSize (0.055);
    text -> SetTextFont (42);
    text -> DrawLatex (0.1, 0.95, title);
  }

  if (plot_ratio)
  {
    c->SetBottomMargin(ratioPadSize);
    c1 = new TPad (Form ("%s_ratio", c -> GetName()),"",0.,0.,1.,ratioPadSize);
    c1 -> SetTopMargin(0.);
    c1 -> SetRightMargin(0.2);
    c1 -> SetBottomMargin(0.1 / ratioPadSize);
    c1 -> Draw();
    TString yAxisTitle = "ratio";
    c1->cd();
    for (int i = 0; i < glists.at(1)->GetSize(); i++)
    {
      TString option;
      TGraph *g = (TGraph*)(glists.at(1)->At(i))->Clone(Form("%s_ratio", glists.at(1)->At(i)->GetName()));
      TGraph *g_ref = (TGraph*)glists.at(0)->At(i);
      DivideGraphs (g, g_ref);
      if (i == 0) option = "a" + graphOption;
      else option = "same " + graphOption;
      g -> Draw (option);
      rgraphs.push_back(g);
    }
    if (!ratioRangeSet) 
      GetRangeY (rgraphs, ratioRange, logY);
    
    line = new TLine (rgraphs.at(0)->GetXaxis()->GetXmin(),1,rgraphs.at(0)->GetXaxis()->GetXmax(),1);
    line->SetLineStyle(refLineStyle);
    line->Draw();
    
    TAxis *xAxis = rgraphs.at(0) -> GetXaxis();
    xAxis -> SetTitleSize(xAxis -> GetTitleSize() / ratioPadSize);
    xAxis -> SetLabelSize(xAxis -> GetLabelSize() / ratioPadSize);
    xAxis -> SetTickLength(xAxis -> GetTickLength() / ratioPadSize);
    
    TAxis *yAxis = rgraphs.at(0) -> GetYaxis();
    yAxis -> SetTitle (yAxisTitle);
    yAxis -> SetTitleSize(yAxis -> GetTitleSize() / ratioPadSize);
    yAxis -> SetTitleOffset(0.1 / ratioPadSize);
    yAxis -> SetLabelSize(yAxis -> GetLabelSize() / ratioPadSize);
    yAxis -> SetRangeUser (ratioRange.at(0), ratioRange.at(1));

    if (xRangeSet) 
    { 
      xAxis -> SetRangeUser(xRange.at(0), xRange.at(1));
      graphs.at(0) -> GetXaxis() -> SetRangeUser(xRange.at(0), xRange.at(1));
    }
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
       
  for (auto g:graphs)
    delete g;
  for (auto g:rgraphs)
    delete g;
  delete line;
  delete leg;
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
  auto hs_ref = (THStack*) files.at(0) -> Get (object_name) -> Clone("htemp");
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
  if (npads == 3) 
  {
    npadsx = 3;
    npadsy = 1;
  }
  c1 -> Divide (npadsx, npadsy);
  
  for (int i = 0; i < files.size(); i++)
  {
    c1 -> cd (i + 1);
    gPad -> SetLeftMargin (0.1);
    gPad -> SetRightMargin (0.);
    gPad -> SetTopMargin (0.1);

    hs = (THStack*) files.at(i) -> Get (object_name);
    if (!hs) continue;
    hs -> Draw (thStackOption);    
    TString xAxisTitle = hs -> GetHistogram() -> GetXaxis() -> GetTitle();
//    TString yAxisTitle = hs -> GetYaxis() -> GetTitle();
    hs -> SetTitle (labels.at(i));
    TPaveText *p = (TPaveText*) gPad -> FindObject ("title");
    if (p) 
    {
      p -> Clear();
      p -> InsertLine ();
      p -> InsertText (labels.at(i));
      p -> SetTextSize (0.05);
    }
    hs -> SetName (name + Form ("_%d", i));
    hs -> GetHistogram() -> GetXaxis() -> SetTitle(xAxisTitle);
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
    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels.at(0));
    for (int i = 0; i < labels.size(); i++)
    {
      c1 -> cd (i + 1);
      hs = (THStack*) gPad -> GetPrimitive (name + Form ("_%d", i));
      if (!hs) continue;
      if (! DivideTHStacks (hs, hs_ref)) 
        break;
      gPad->SetLogy(0);
      TLine line (hs->GetXaxis()->GetXmin(),1,hs->GetXaxis()->GetXmax(),1);
      line.SetLineStyle(refLineStyle);
      line.Draw();
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
  auto hs_ref = (THStack*) files.at(0) -> Get (object_name) -> Clone("htemp");
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

    hs = (THStack*) files.at(i) -> Get (object_name);
    if (!hs) continue;
    hslist = hs -> GetHists ();
    for (int j = 0; j < hslist -> GetSize(); j++)
    {
      TH1* h = (TH1*) hslist -> At(j);
      h -> SetLineStyle(lineStyles.at(i));
      h -> SetMarkerStyle(markerStyles.at(i).at(j));
      hs_common -> Add(h,h->GetOption());
    }
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
    text -> DrawLatex(0.1, 0.95, title + ": ratio to " + labels.at(0));
    hs = (THStack*) files.at(1) -> Get (object_name);
    if (! hs && ! DivideTHStacks (hs, hs_ref))
    {
      hs -> Draw ("NOSTACK");
      leg -> Draw("same");
      gPad->SetLogy(0);
      TLine line (hs->GetXaxis()->GetXmin(),1,hs->GetXaxis()->GetXmax(),1);
      line.SetLineStyle(refLineStyle);
      line.Draw();
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
 
  if (!glist || !glist_ref)  
  {
    cout << "Error while dividing multigraphs: empty list of graphs!";
    return false;
  }

  if (glist -> GetSize() != glist_ref -> GetSize())
  {
    cout << "Warning in DivideMultiGraphs: different number of graphs!";
  }
  
  int nGraphs = glist -> GetSize();
  for (int i = 0; i < nGraphs; i++)
    if (! DivideGraphs ((TGraphErrors*) glist -> At (i), (TGraphErrors*) glist_ref -> At (i)))
      glist->At(i)->Delete();
  
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


void GetRangeY (vector <TH1*> hists, vector <float> &range, bool logY)
{
  range.resize(2);
  range.at(0) = numeric_limits<float>::max();
  range.at(1) = numeric_limits<float>::min();

  for (auto hist:hists) 
  {
    if (!hist) continue;
    int minBin = hist->GetMinimumBin(); 
    int maxBin = hist->GetMaximumBin();
    float min = hist->GetBinContent(minBin) - hist->GetBinError(minBin);
    float max = hist->GetBinContent(maxBin) + hist->GetBinError(maxBin);
    if (min < range.at(0))
      range.at(0) = min;
    if (max > range.at(1))
      range.at(1) = max;
  }
  float sup = 0.05 * (range.at(1) - range.at(0));
  if (range.at(0) != 0.) range.at(0) -= sup;
  range.at(1) += sup;
  if (logY) range.at(0) = 1.;
}


void GetRangeY (vector <TGraph*> graphs, vector <float> &range, bool logY)
{
  range.resize(2);
  range.at(0) = numeric_limits<float>::max();
  range.at(1) = numeric_limits<float>::min();

  for (auto graph:graphs) 
  {
    if (!graph) continue;
    int n = graph->GetN();
    double *y = graph->GetY();
    float errHigh, errLow;
    double *minY = min_element(y, y+n);
    double *maxY = max_element(y, y+n);
    int minBin = distance (y, minY); 
    int maxBin = distance (y, maxY);
    TString className = graph->ClassName(); 
    if ( className.Contains ("Errors"))
    {
      errLow = graph->GetErrorYlow(minBin);
      errHigh = graph->GetErrorYhigh(maxBin);
    }
    float min = *minY - errLow;
    float max = *maxY + errHigh;
    if (min < range.at(0))
      range.at(0) = min;
    if (max > range.at(1))
      range.at(1) = max;
  }
  float sup = 0.05 * (range.at(1) - range.at(0));
  if (range.at(0) != 0.) range.at(0) -= sup;
  range.at(1) += sup;
  if (logY) range.at(0) = 1.;
}
