void easy(int v){
    v = 2;
    int p = v + 1e7;
    p = ~p;
    p = p | v;
    v = p + v;
    v = v + 1e7;
}