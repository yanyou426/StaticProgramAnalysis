class A{
	public:
		A foo(A x){
			return x;
		}
};



int main(){
	A *a = new A();
	
	A *d = new A();
	A *b = d;
	A c = b->foo(*a);
	delete d;
	delete b;
	delete a;
	return 0;

}