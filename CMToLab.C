
constexpr double M_NUCL = 0.938;

int CMToLab(double sNN = 4.5) {

    double sN = sNN/2.;
    double pN = TMath::Sqrt(sN*sN - M_NUCL*M_NUCL);

    TLorentzVector tMom4(0, 0, -pN, sN);
    TVector3 vBoost{tMom4.BoostVector()};

    TLorentzVector pMom4(0, 0, pN, sN);
    pMom4.Boost(-vBoost);

    cout << "E_lab = " << pMom4.E() << "A GeV" << endl;
    cout << "T_kin = " << pMom4.E() - pMom4.M() << "A GeV" << endl;
    cout << "p_lab = " << pMom4.Pz() << "A GeV/c" << endl;

    

    return 0;
}
