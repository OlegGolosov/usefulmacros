#!/usr/bin/python

import sys

import argparse
import math
import ROOT
from ROOT import gStyle
from ROOT import gDirectory
from ROOT import gPad

parser = argparse.ArgumentParser(description='Compare root files')

parser.add_argument('-i', '--input', dest='input', help='Input root files', nargs='+', required=True)
parser.add_argument('-l', '--labels', dest='labels', help='Labels for each file', nargs='+', required=False)
parser.add_argument('-o', '--output', dest='output', help='Output file', default='comp.root')
parser.add_argument('-d', '--directory', dest='directory', help='Directory to compare', required=False)
parser.add_argument('--rescale', dest='rescale', help='Rescale histograms', required=False, type=bool, default=True)

args = parser.parse_args()
print(args)

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(111111)
ROOT.gStyle.SetTitleAlign(33)
colors = [ROOT.kBlack, ROOT.kRed, ROOT.kBlue, ROOT.kGreen+2, ROOT.kMagenta+2, ROOT.kOrange+2, ROOT.kPink+2, ROOT.kTeal+2, ROOT.kCyan+2, ROOT.kCyan+4, ROOT.kAzure, ROOT.kGray+3, ROOT.kOrange+7, ROOT.kGreen+4]

output_path = args.output
if args.directory is not None:
  directory = args.directory
else: 
  directory = ''

files = []
for file_path in args.input:
  file = ROOT.TFile.Open (file_path)
  files.append (file)

i = 0
labels = []
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
  
output_file = ROOT.TFile.Open(output_path,"recreate")
output_path_pdf=output_path.replace(".root",".pdf") 

object_names = []
files[0].cd(directory)
for key in gDirectory.GetListOfKeys():
  object_name=key.GetName()
  hist=gDirectory.Get(object_name)
  if hist.InheritsFrom("TH1") or hist.InheritsFrom("TGraph") or hist.InheritsFrom("TMultiGraph"):
    object_names.append(object_name)
  else:
    print 'Skipping non-histogram object', object_name
for file in files:
  file.cd(directory)
  object_names_temp = []
  for key in gDirectory.GetListOfKeys():
    object_names_temp.append(key.GetName())
  object_names = list(set(object_names).intersection(object_names_temp))

c = ROOT.TCanvas('c_first', 'c_first')

text = ROOT.TLatex()
text.SetNDC()
text.SetTextSize(0.055)
text.SetTextFont(42)
text.DrawLatex(0.1, 0.9, 'Comparing ')
i = 0
for label in labels:
  ypos = 0.85 - 0.05 * i
  text.DrawLatex(0.1, ypos, args.input [i] + ' (' + label + ')')
  i += 1
c.Print(output_path_pdf + '(','Title:Title')
    
for object_name in object_names:
  print(object_name)
  
  files[0].cd(directory)
  ref_hist=gDirectory.Get(object_name).Clone()
  #ref_hist.Sumw2()
  title = ref_hist.GetName()
  c = ROOT.TCanvas('c_' + title, title)
  c.cd()
  
  if (ref_hist.InheritsFrom("TH2")):
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
      file.cd(directory)
      c1.cd(i+1)
      ROOT.gPad.SetLogz()
      ROOT.gPad.SetLeftMargin(0.15)
      ROOT.gPad.SetTopMargin(0.01)
      
      hist=gDirectory.Get(object_name)
      hist.SetName(object_name + '_(%d)' % i)
      hist.SetTitle(labels[i])
      
      if args.rescale:
        scale_factor = 1.0*ref_hist.GetEntries()/hist.GetEntries() if hist.GetEntries() != 0 else 1
        hist.Scale(scale_factor)
        
      hist.Draw("colz")
      ROOT.gPad.Modified()
      ROOT.gPad.Update()
      stats = ROOT.gPad.GetPrimitive('stats')
      stats.SetName('stats_(%d)' % i)
      stats.SetTextColor(colors [i])
      stats.SetX1NDC(.7)
      stats.SetX2NDC(.9)
      stats.SetY1NDC(.7)
      stats.SetY2NDC(.99)
      statTitle = stats.GetLineWith(hist.GetName())
      statTitle.SetText(0,0,hist.GetTitle())
      hist.SetStats(0)
      i += 1
    
    output_file.cd()
    c.Print(output_path_pdf,'Title:'+ title.replace('tex','te'))
    c.Write()

    c.SetName(c.GetName() + "_ratio")
    c.cd()
    text.DrawLatex(0.1, 0.95, title + ": ratio to " + labels [0])
    i = 0
    for label in labels:
      c1.cd(i + 1)
      ROOT.gPad.SetLogz(0)
      hist=ROOT.gPad.GetPrimitive(object_name + '_(%d)' % i)
      #hist.Sumw2()
      hist.Divide(ref_hist)
      i += 1
    c.Print(output_path_pdf,'Title:'+ title.replace('tex','te') + "_ratio")
    c.Write()

  elif (ref_hist.InheritsFrom("TH1")):
    ROOT.gStyle.SetOptTitle(1)
    stack = ROOT.THStack ('st_' + object_name, title)
    i = 0
    hists = []
    for file in files:
      file.cd(directory)
      hist=gDirectory.Get(object_name)
      hist.SetName(object_name + '_(%d)' % i)
      hist.SetTitle(labels[i])
          
      if args.rescale:
        scale_factor = 1.0*ref_hist.GetEntries()/hist.GetEntries() if hist.GetEntries() != 0 else 1
        print ref_hist.GetEntries(), hist.GetEntries(), scale_factor
        hist.Scale(scale_factor)
      hist.SetLineWidth(2)
      hist.SetLineColor(colors[i])
      hist.Draw()
      ROOT.gPad.Modified()
      ROOT.gPad.Update()
      stats=hist.GetListOfFunctions().FindObject('stats')
      stats.SetName('stats_(%d)' % i)
      stats.SetTextColor(colors[i])
      stats.SetX1NDC(0.8)
      stats.SetX2NDC(1.0)
      y_offset=0.1*i
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
    output_file.cd()
    c.Print(output_path_pdf,'Title:'+ title.replace('tex','te'))
    c.Write()
    
    c.SetName(c.GetName() + "_ratio")
    stack = ROOT.THStack(stack.GetName() + "_ratio", stack.GetTitle() + ": ratio to " + labels [0]);
    for hist in hists:
      #hist.Sumw2()
      hist.Divide(ref_hist)
      stack.Add(hist)
    stack.Draw("nostack")
    #ROOT.gPad.BuildLegend(0.8,0.0,1.0,0.9-y_offset)
    c.Print(output_path_pdf,'Title:'+ title.replace('tex','te') + "_ratio")
    c.Write()
        
c = ROOT.TCanvas('c_last', 'The end!')
text.DrawLatex(0.5, 0.6, 'The end! ')

c.Print(output_path_pdf + ')','Title:The end!')
