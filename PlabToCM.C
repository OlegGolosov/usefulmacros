void PlabToCM(float pl)
{
  float mp=0.938;
  float El=sqrt(pl*pl+mp*mp);
  float Ekin=El-mp;
  float pc=sqrt( ( mp*El-mp*mp )/2 );
  float e_cm=sqrt( pc*pc + mp*mp );
  float ss = sqrt( 4*(pc*pc + mp*mp) );
  float yBeam = 0.25 * log ((El + pl) / (El - pl));
  cout << "Elab = " << El << endl;
  cout << "Ekin = " << Ekin << endl;
  cout << "P* = " << pc << endl;
  cout << "E* = " << e_cm << endl;
  cout << "sqrt(Snn) = " << ss << endl;
  cout << "yBeam = " << yBeam << endl;
}
