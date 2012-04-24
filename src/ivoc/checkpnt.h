#ifndef checkpoint_h
#define checkpoint_h

#define CKPT(arg1,arg2) if (!arg1.xdr(arg2)) { return false;}

class Checkpoint {
public:
	Checkpoint();
	virtual ~Checkpoint();
		
	static Checkpoint* instance();
	
	bool out(); // true means to file, false means from file
	bool xdr(long&);
	bool xdr(Object*&);
};

#endif
