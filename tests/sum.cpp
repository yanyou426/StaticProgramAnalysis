int sum_up(int p, int q, int k){
  int x = p + 1;
  int y = q + 2;
  int z, m;

  do{
    m = k;
    y = q - 1;
    if(x > 5){
      x = 5;
      z = 5;
    }
    else{
      x = m -3;
      break;
    }
  } while(x > 1);
  z = 2 * p;
  return z;
}