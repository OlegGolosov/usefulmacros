void PlabToCM(float pl)
{
// code to calculate T0 for a given energy
  Double_t mp=0.938;
  Double_t El=sqrt(pl*pl+mp*mp);
  Double_t Ekin=El-mp;
  Double_t pc=sqrt( ( mp*El-mp*mp )/2 );
  Double_t e_cm=sqrt( pc*pc + mp*mp );
  Double_t ss = sqrt( 4*(pc*pc + mp*mp) );
  cout << "E = " << El << endl;
  cout << "Ekin = " << Ekin << endl;
  cout << "P* = " << pc << endl;
  cout << "E* = " << e_cm << endl;
  cout << "sqrt(Snn) = " << ss << endl;
}
