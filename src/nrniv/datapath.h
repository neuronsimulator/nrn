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
    HocDataPaths(HocDataPaths&&) noexcept;
    HocDataPaths& operator=(HocDataPaths&&) noexcept;
    HocDataPaths(const HocDataPaths&) = delete;
    HocDataPaths& operator=(const HocDataPaths&) = delete;
    virtual ~HocDataPaths();

    void append(double*);
    void append(char**);
    void search();
    std::string retrieve(double*) const;
    std::string retrieve(char**) const;
    Symbol* retrieve_sym(double*) const;
    int style();

  private:
    // Owning pimpl. Implicit copy/move would duplicate impl_ and double-free
    // (MSVC does not NRVO create_hdp; gcc/clang NRVO hid it).
    HocDataPathImpl* impl_;
};
