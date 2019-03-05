#!/usr/bin/python

import sys

import argparse
import math
import ROOT
from ROOT import gStyle
from ROOT import gPad


parser = argparse.ArgumentParser(description='Compare root files')

parser.add_argument('-i', '--input', dest='input', help='Input root files', nargs='+', required=True)
parser.add_argument('-l', '--labels', dest='labels', help='Labels for each file', nargs='+', required=False)
parser.add_argument('-o', '--output', dest='output', help='Output file', default='comp.root')
parser.add_argument('-d', '--directory', dest='directory', help='Directory to compare', default='')
parser.add_argument('--depth', dest='max_depth', help='Maximum depth of folder search', default=10)
parser.add_argument('--rescale', dest='rescale', help='Rescale histograms', required=False, type=bool, default=True)
parser.add_argument('-r', '--ratio', dest='plot_ratio', help='Plot histogram ratio', required=False, type=bool, default=False)
parser.add_argument('--root', dest='write_to_root', help='Write output to ROOT file', required=False, type=bool, default=False)
parser.add_argument('--pdf', dest='write_to_pdf', help='Write output to PDF file', required=False, type=bool, default=True)

args = parser.parse_args()

colors = [ROOT.kBlack, ROOT.kRed, ROOT.kBlue, ROOT.kGreen+2, ROOT.kMagenta+2, ROOT.kOrange+2, ROOT.kPink+2, ROOT.kTeal+2, ROOT.kCyan+2, ROOT.kCyan+4, ROOT.kAzure, ROOT.kGray+3, ROOT.kOrange+7, ROOT.kGreen+4]
files = []
labels = []
max_depth = int (args.max_depth)

def BuildObjectList (directory, depth = 0):
  global max_depth
  directory_path=directory.GetPath().rsplit(':', 1)[-1]
  object_names = []
  for key in directory.GetListOfKeys():
    object_name=key.GetName()
    object=directory.Get(object_name)
    if type(object).__name__.startswith("TH") or type(object).__name__.startswith("TProfile") or type(object).__name__.endswith("Graph"):
      object_names.append(directory_path + '/' + object_name)
    elif ('TDirectoryFile' in type(object).__name__) and (depth < max_depth):
      object_names.extend (BuildObjectList (object, depth + 1))
  return object_names


def PlotTH1 (object_name):
  global colors
  global files
  global titles
  
  ref_hist=files[0].Get(object_name).Clone()  
  if ref_hist.InheritsFrom("TProfile"):
    ref_hist=ref_hist.ProjectionX()
  #ref_hist.Sumw2()
  
  title = object_name
  c = ROOT.TCanvas('c_' + title, title)
  c.cd()
  ROOT.gStyle.SetOptTitle(1)
  stack = ROOT.THStack ('st_' + object_name, title)
  i = 0
  hists = []
  for file in files:
    hist = file.Get(object_name)
    hist.SetName(object_name + '_%d' % i)
    hist.SetTitle(labels[i])
        
    if args.rescale:
      #scale_factor = 1.0*ref_hist.GetEntries()/hist.GetEntries() if hist.GetEntries() != 0 else 1
      scale_factor = 1.0*ref_hist.Integral("width")/hist.Integral("width") if hist.Integral("width") != 0 else 1
      hist.Scale(scale_factor)
    hist.SetLineWidth(2)
    hist.SetLineColor(colors[i])
    hist.SetMarkerColor(colors[i])
    hist.Draw()
    ROOT.gPad.Modified()
    ROOT.gPad.Update()
    stats=hist.GetListOfFunctions().FindObject('stats')
    stats.SetName('stats_%d' % i)
    stats.SetTextColor(colors[i])
    stats.SetX1NDC(0.8)
    stats.SetX2NDC(1.0)
    y_offset = 0.1 * i
    stats.SetY1NDC(0.9-y_offset)
    stats.SetY2NDC(1.0-y_offset)
    statTitle = stats.GetLineWith(object_name)
    statTitle.SetText(0,0,labels [i])
    stack.Add(hist)
    hists.append(hist)
    i+=1
          
  ROOT.gPad.SetRightMargin(0.2)
  stack.Draw("nostack")
  #ROOT.gPad.BuildLegend(0.8,0.0,1.0,0.9-y_offset)
  if args.write_to_pdf: 
      c.Print(output_path_pdf,'Title:'+ title.replace('tex','te'))
  if args.write_to_root: 
    output_file.cd()
    c.Write()
  
  if args.plot_ratio:
    c.SetName(c.GetName() + "_ratio")
    stack = ROOT.THStack(stack.GetName() + "_ratio", stack.GetTitle() + ": ratio to " + labels [0]);
    for hist in hists:
      if hist.InheritsFrom("TProfile"):
        hist=hist.ProjectionX()
      #hist.Sumw2()
      hist.Divide(ref_hist)
      hist.GetYaxis().SetRangeUser(0.,4.)
      stack.Add(hist)        
    stack.Draw("nostack")
    stack.GetHistogram().GetYaxis().SetRangeUser(0.,4.)
    gPad.Modified()
    gPad.Update()
    #ROOT.gPad.BuildLegend(0.8,0.0,1.0,0.9-y_offset)
    if args.write_to_pdf: 
      c.Print(output_path_pdf,'Title:'+ title.replace('tex','te') + "_ratio")
    if args.write_to_root: 
      c.Write()

def PlotTH2 (object_name):
  global colors
  global files
  global titles
    
  text = ROOT.TLatex()
  text.SetNDC()
  text.SetTextSize(0.055)
  text.SetTextFont(42)
  
  ref_hist=files[0].Get(object_name).Clone()
  if ref_hist.InheritsFrom("TProfile2D"):
    ref_hist=ref_hist.ProjectionXY()
  #ref_hist.Sumw2()
  
  title = object_name
  c = ROOT.TCanvas('c_' + title, title)
  c.cd()
  ROOT.gStyle.SetOptTitle(0)
  text.DrawLatex(0.1, 0.95, title)
  c1 = ROOT.TPad("c1", "c1", 0., 0., 1., 0.95);
  c1.Draw();
  c1.cd();
  npads = len(labels)
  npadsx = int(math.ceil (math.sqrt (npads)))
  npadsy = int(math.ceil (1. * npads / npadsx))
  c1.Divide(npadsx, npadsy)
  i = 0
  for file in files:
    c1.cd(i+1)
    ROOT.gPad.SetLogz()
    ROOT.gPad.SetLeftMargin(0.15)
    ROOT.gPad.SetTopMargin(0.01)
    
    hist=file.Get(object_name)
    hist.SetName(object_name + '_%d' % i)
    hist.SetTitle(labels[i])
    
    if args.rescale:
      #scale_factor = 1.0*ref_hist.GetEntries()/hist.GetEntries() if hist.GetEntries() != 0 else 1
      scale_factor = 1.0*ref_hist.Integral("width")/hist.Integral("width") if hist.Integral("width") != 0 else 1
      hist.Scale(scale_factor)
      
    hist.Draw("colz")
    ROOT.gPad.Modified()
    ROOT.gPad.Update()
    stats = ROOT.gPad.GetPrimitive('stats')
    stats.SetName('stats_%d' % i)
    stats.SetTextColor(colors [i])
    stats.SetX1NDC(.7)
    stats.SetX2NDC(.9)
    stats.SetY1NDC(.7)
    stats.SetY2NDC(.99)
    statTitle = stats.GetLineWith(hist.GetName())
    statTitle.SetText(0,0,hist.GetTitle())
    hist.SetStats(0)
    i += 1
  
  if args.write_to_pdf: 
      c.Print(output_path_pdf,'Title:'+ title.replace('tex','te'))
  if args.write_to_root: 
    output_file.cd()
    c.Write()
  if args.plot_ratio:
    c.SetName(c.GetName() + "_ratio")
    c.cd()
    text.DrawLatex(0.1, 0.95, title + ": ratio to " + labels [0])
    i = 0
    for label in labels:
      c1.cd(i + 1)
      ROOT.gPad.SetLogz(0)
      hist=ROOT.gPad.GetPrimitive(object_name + '_%d' % i)
      if hist.InheritsFrom("TProfile2D"):
        hist=hist.ProjectionXY()
      #hist.Sumw2()
      hist.Divide(ref_hist)
      hist.GetZaxis().SetRangeUser(0,4)
      i += 1
    if args.write_to_pdf: 
      c.Print(output_path_pdf,'Title:'+ title.replace('tex','te') + "_ratio")
    if args.write_to_root: c.Write()


print(args)

ROOT.gROOT.SetBatch(True)
ROOT.gErrorIgnoreLevel=2000
gStyle.SetOptStat(111111)
gStyle.SetTitleAlign(33)

output_path = args.output

dirs = []
for file_path in args.input:
  files.append(ROOT.TFile.Open(file_path))
  dirs.append(files[-1].GetDirectory (args.directory))

if args.labels is not None:
  for label in args.labels:
    if len (args.labels) > len (args.input): 
      print 'Warning! Number of labels provided is larger than number of input files!'
      break
    labels.append(label)
  if len(args.labels) < len(args.input):
    print 'Warning! Number of labels provided is less than number of input files!'

i = len(labels)    
while i < len (args.input):
  label = args.input[i]
  if '/' in label: 
    label = label.rsplit('/', 1)[-1]
  if label.endswith('.root'):
    label = label.rsplit('.root', 1)[0]
  labels.append(label)
  i+=1  
  
output_path_pdf=output_path.replace(".root",".pdf") 
if args.write_to_root: 
  output_file = ROOT.TFile.Open(output_path,"recreate") 
 
object_names = BuildObjectList (dirs[0])
for dir in dirs:
  object_names = sorted (list(set(object_names).intersection(BuildObjectList (dir))), key = object_names.index)
#print object_names

c = ROOT.TCanvas('c_first', 'c_first')

text = ROOT.TLatex()
text.SetNDC()
text.SetTextSize(0.055)
text.SetTextFont(42)
text.DrawLatex(0.1, 0.9, 'Comparing folder /' + args.directory + ' of files:')
i = 0
for label in labels:
  ypos = 0.85 - 0.05 * i
  text.DrawLatex(0.1, ypos, args.input [i] + ' (' + label + ')')
  i += 1
c.Print(output_path_pdf + '(','Title:Title')
    
for object_name in object_names:
  while object_name.startswith ('/'):
    object_name = object_name.split('/', 1)[-1]
  print object_name
  
  object=files[0].Get(object_name).Clone()
  
  if (object.InheritsFrom("TH2")):
    PlotTH2 (object_name)
    
  elif (object.InheritsFrom("TH1")):
    PlotTH1 (object_name)
    
        
c = ROOT.TCanvas('c_last', 'The end!')
text.DrawLatex(0.5, 0.6, 'The end! ')

c.Print(output_path_pdf + ')','Title:The end!')
