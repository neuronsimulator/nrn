#ifndef bbsavestate_h
#define bbsavestate_h

struct Object;
struct Section;
struct Node;
struct Prop;
struct Point_process;

// base class to allow either counting, reading, writing files, buffers, etc.
// All the BBSaveState methods take a BBSS_IO subclass and are merely iterators
class BBSS_IO {
  public:
    enum Type { IN, OUT, CNT };
    BBSS_IO();
    virtual ~BBSS_IO() {}
    virtual void i(int& j, int chk = 0) = 0;
    virtual void d(int n, double& p) = 0;
    virtual void d(int n, double* p) = 0;
    virtual void s(char* cp, int chk = 0) = 0;
    virtual Type type() = 0;
    virtual void skip(int) {}  // only when reading
};

class BBSaveState {
  public:
    BBSaveState();
    virtual ~BBSaveState();
    virtual void apply(BBSS_IO* io);
    BBSS_IO* f;

    int counts(int** gids, int** counts);
    void core();
    void init();
    void finish();
    void gids();
    void gid2buffer(int gid, char* buffer, int size);
    void buffer2gid(int gid, char* buffer, int size);
    void gidobj(int basegid);
    void gidobj(int spgid, Object*);
    void cell(Object*);
    int cellsize(Object*);
    void section_exist_info(Section*);
    void section(Section*);
    int sectionsize(Section*);
    void seccontents(Section*);
    void node(Node*);
    void node01(Section*, Node*);
    void mech(Prop*);
    void netrecv_pp(Point_process*);
    void possible_presyn(int gid);

    void mk_base2spgid();
    void mk_pp2de();
    void mk_presyn_info();
    void del_pp2de();
};


#endif
