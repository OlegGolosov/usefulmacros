#!/usr/bin/python

import sys

import argparse
import ROOT

parser = argparse.ArgumentParser(description='Compare root files')

parser.add_argument('-i', '--input', dest='input', help='Input root files', nargs=2, required=True)
parser.add_argument('-l', '--labels', dest='labels', help='Labels for each file', nargs=2, required=False)
parser.add_argument('-o', '--output', dest='output', help='Output file', required=True)
parser.add_argument('-d', '--directory', dest='directory', help='Directory to compare', required=False)
parser.add_argument('--rescale', dest='rescale', help='Rescale histograms', required=False, type=bool, default=False)

args = parser.parse_args()
print(args)

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetOptTitle(0)

file_1_path = args.input[0]
file_2_path = args.input[1]
output_path = args.output

dir_1 = ROOT.TFile.Open(file_1_path)
dir_2 = ROOT.TFile.Open(file_2_path)

if args.directory is not None:
    subdir_1 = dir_1.Get(args.directory)
    subdir_2 = dir_2.Get(args.directory)
else:
    subdir_1 = dir_1
    subdir_2 = dir_2


output_file = ROOT.TFile.Open(output_path,"recreate")


keys_1 = [key.GetName() for key in subdir_1.GetListOfKeys()]
keys_2 = [key.GetName() for key in subdir_2.GetListOfKeys()]
keys = list(set(keys_1).intersection(keys_2))

histograms = [(name, subdir_1.Get(name), subdir_2.Get(name)) for name in sorted(keys) ]

c = ROOT.TCanvas('c_first', 'c_first')

text = ROOT.TLatex()
text.SetNDC()
text.SetTextSize(0.035)
text.DrawLatex(0.1, 0.8, 'Comparing ')
text.DrawLatex(0.1, 0.7, file_1_path)
text.DrawLatex(0.1, 0.6, 'and')
text.DrawLatex(0.1, 0.5, file_2_path)
c.Print(output_path + '.pdf(','Title:Start')


for hist_name, hist_1, hist_2 in histograms:
    print(hist_name)

    if not(type(hist_1).__name__.startswith("TH") and type(hist_2).__name__.startswith("TH")):
        print('Skipping non-histogram object', hist_name)
        continue

    if args.rescale:
        scale_factor = 1.0*hist_1.GetEntries()/hist_2.GetEntries() if hist_2.GetEntries() != 0 else 1
        hist_2.Scale(scale_factor)
    
    title = hist_1.GetTitle()
    c = ROOT.TCanvas('c_' + hist_name, title)
    ROOT.gStyle.SetOptTitle(0)
    c.UseCurrentStyle()
    if args.labels is not None:
        hist_1.SetTitle(args.labels[0])
        hist_2.SetTitle(args.labels[1])
    c.cd()
    if type(hist_1).__name__.startswith("TH1") and type(hist_2).__name__.startswith("TH1"):
        stack = ROOT.THStack ('st_' + hist_name, title)
        hist_1.SetLineWidth(2)
        hist_1.SetLineColor(ROOT.kRed)
        hist_1.Draw()
        stat_1 = hist_1.GetListOfFunctions().FindObject('stats')
        stat_1.SetTextColor(ROOT.kRed)
        stat_1.SetX1NDC(0.8)
        stat_1.SetX2NDC(1.0)
        stat_1.SetY1NDC(0.84)
        stat_1.SetY2NDC(1.0)
        statTitle = stat_1.GetLineWith(hist_1.GetName())
        statTitle.SetText(0,0,hist_1.GetTitle())
        hist_2.SetLineWidth(2)
        hist_2.SetLineColor(ROOT.kBlack)
        hist_2.Draw()
        stat_2 = hist_2.GetListOfFunctions().FindObject('stats')
        stat_2.SetTextColor(ROOT.kBlack)
        stat_2.SetX1NDC(0.8)
        stat_2.SetX2NDC(1.0)
        stat_2.SetY1NDC(0.68)
        stat_2.SetY2NDC(0.84)
        statTitle = stat_2.GetLineWith(hist_2.GetName())
        statTitle.SetText(0,0,hist_2.GetTitle())
        ROOT.gPad.SetRightMargin(0.2)
        
        stack.Add(hist_1)
        stack.Add(hist_2)
        stack.Draw("nostack")
      
        ROOT.gPad.BuildLegend(0.8,0.0,1.0,0.68)
        
    elif type(hist_1).__name__.startswith("TH2") and type(hist_2).__name__.startswith("TH2"):
        c.Divide(2,1)
        c.cd(1)
        ROOT.gPad.SetLogz()
        hist_1.Draw("colz")
        c.cd(2)
        ROOT.gPad.SetLogz()
        hist_2.Draw("colz")
    
    c.Print(output_path + '.pdf','Title:'+ title)
    c.Write()

c = ROOT.TCanvas('c_last', 'The end!')
text.DrawLatex(0.5, 0.6, 'The end! ')
c.Print(output_path + '.pdf)','Title:Finish')

