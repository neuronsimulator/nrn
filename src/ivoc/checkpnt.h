#ifndef checkpoint_h
#define checkpoint_h

#define CKPT(arg1,arg2) if (!arg1.xdr(arg2)) { return false;}

class Checkpoint {
public:
	Checkpoint();
	virtual ~Checkpoint();
		
	static Checkpoint* instance();
	
	boolean out(); // true means to file, false means from file
	boolean xdr(long&);
	boolean xdr(Object*&);
};

#endif
