#ifndef ocfile_h
#define ocfile_h

#include <string>
class File;
class FileChooser;

class OcFile {
  public:
    OcFile();
    virtual ~OcFile();
    bool open(const char* filename, const char* type);
    void set_name(const char* s);
    const char* get_name() {
        return filename_.c_str();
    }
    const char* dir();
    void close();
    void print(const char* s) {
        fprintf(file(), "%s", s);
    }
    FILE* file();
    bool is_open() {
        return file_ ? true : false;
    }
    bool eof();
    void flush() {
        if (file_) {
            fflush(file_);
        }
    }
    bool mktemp();
    bool unlink();
    void file_chooser_style(const char* type,
                            const char* path,
                            const char* banner = NULL,
                            const char* filter = NULL,
                            const char* accept = NULL,
                            const char* cancel = NULL);
    bool file_chooser_popup();
#ifdef WIN32
    void binary_mode();  // sets open file to binary mode once only
#endif
  private:
    FileChooser* fc_;
    enum { N, R, W, A };
#if HAVE_IV
    int chooser_type_;
#endif
    std::string filename_;
    std::string dirname_;
    FILE* file_;
#ifdef WIN32
    bool binary_;
    char mode_[3];
#endif
};

#ifdef WIN32
#define BinaryMode(ocfile) ocfile->binary_mode();
#else
#define BinaryMode(ocfile) /**/
#endif

bool isDirExist(const std::string& path);
bool makePath(const std::string& path);

#endif
