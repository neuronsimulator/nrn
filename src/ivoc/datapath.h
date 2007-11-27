/*
  given a set of double*, find the executable hoc varnames
 */

struct Symbol;
class String;
class HocDataPathImpl;

class HocDataPaths {
public:
	// 0 objref style, 1 object id style, 2 symbol style
 	HocDataPaths(int = 1000, int pathstyle = 0);
	virtual ~HocDataPaths();

	void append(double*);
	void append(char**);
	void search();
	String* retrieve(double*);
	String* retrieve(char**);
	Symbol* retrieve_sym(double*);
	int style();
private:
	HocDataPathImpl* impl_;
};
