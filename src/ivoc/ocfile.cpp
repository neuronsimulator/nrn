#include <../../nrnconf.h>
#if defined(__GO32__)
#define HAVE_IV 0
#endif

#include <stdio.h>
#include <stdlib.h>
#if MAC || defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#ifdef WIN32
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#endif
#include <InterViews/resource.h>
#if HAVE_IV
#include "utility.h"
#include <IV-look/dialogs.h>
#include <InterViews/session.h>
#include <InterViews/display.h>
#include <InterViews/style.h>
#endif
#include "nrnmpi.h"
#include "oc2iv.h"
#include "classreg.h"
#include "ocfile.h"
#include "nrnfilewrap.h"

extern char* ivoc_get_temp_file();
static int ivoc_unlink(const char*);
int ivoc_unlink(const char* s) {
	return unlink(s);
}

extern "C" {
#include "hocstr.h"
	FILE* hoc_obj_file_arg(int i);
}

FILE* hoc_obj_file_arg(int i) {
	Object* ob = *hoc_objgetarg(i);
	check_obj_type(ob, "File");
	OcFile* f = (OcFile*)(ob->u.this_pointer);
	if (!f->is_open()) {
		hoc_execerror("File not open:", f->get_name());
	}
	return f->file();
}

OcFile::OcFile() :filename_(""){
	file_ = NULL;
	fc_ = NULL;
#ifdef WIN32
	binary_ = false;
#endif
}
OcFile::~OcFile() {
#if HAVE_IV
	Resource::unref(fc_);
#endif
	close();
}

static double f_ropen(void* v){
	OcFile* f = (OcFile*)v;
	if (ifarg(1)) {
		f->set_name(gargstr(1));
	}
	return double(f->open(f->get_name(), "r"));
}

static double f_wopen(void* v){
	OcFile* f = (OcFile*)v;
	if (ifarg(1)) {
		f->set_name(gargstr(1));
	}
	return double(f->open(f->get_name(), "w"));
}

static double f_aopen(void* v){
	OcFile* f = (OcFile*)v;
	if (ifarg(1)) {
		f->set_name(gargstr(1));
	}
	int err = f->open(f->get_name(), "a");
#if defined(CYGWIN)
	/* ignore illegal seek */
	if (err && errno == 29) { errno = 0; }
#endif
	return double(err);
}

static double f_printf(void* v){
	OcFile* f = (OcFile*)v;
	char* buf;
	hoc_sprint1(&buf, 1);
	f->print(buf);	
	return 0.;
}

static double f_scanvar(void* v){
	OcFile* f = (OcFile*)v;
	return hoc_scan(f->file());
}

static double f_scanstr(void* v){
	OcFile* f = (OcFile*)v;
	char** pbuf = hoc_pgargstr(1);
	char* buf = hoc_tmpbuf->buf;
	int i = fscanf(f->file(), "%s", buf);
	if (i == 1) {
		hoc_assign_str(pbuf, buf);
		return double(strlen(buf));
	}else{
		return -1.;
	}
}

static double f_gets(void* v) {
	OcFile* f = (OcFile*)v;
	char** pbuf = hoc_pgargstr(1);
	char* buf;
#if USE_NRNFILEWRAP
	NrnFILEWrap nfw, *fw;
	nfw.f = f->file();
	fw = &nfw;
#else
	FILE* fw = f->file();
#endif
	if ((buf = fgets_unlimited(hoc_tmpbuf, fw)) != 0) {
		hoc_assign_str(pbuf, buf);
		return double(strlen(buf));
	}else{
		return -1.;
	}
}

static double f_mktemp(void* v) {
	OcFile* f = (OcFile*)v;
	return double(f->mktemp());
}

static double f_unlink(void* v) {
	OcFile* f = (OcFile*)v;
	return double(f->unlink());
}

static double f_eof(void* v){
	OcFile* f = (OcFile*)v;
	return double(f->eof());
}

static double f_is_open(void* v){
	OcFile* f = (OcFile*)v;
	return double(f->is_open());
}

static double f_flush(void* v){
	OcFile* f = (OcFile*)v;
	f->flush();
	return 1;
}

static const char** f_get_name(void* v){
	OcFile* f = (OcFile*)v;
	char** ps = hoc_temp_charptr();
	*ps = (char*)f->get_name();
	if (ifarg(1)) {
		hoc_assign_str(hoc_pgargstr(1), *ps);
	}
	return (const char**)ps;
}

static const char** f_dir(void* v) {
	char** ps = hoc_temp_charptr();
	OcFile* f = (OcFile*)v;
	*ps = (char*)f->dir();
	return (const char**)ps;
}

static double f_chooser(void* v){
#if HAVE_IV
IFGUI
	OcFile* f = (OcFile*)v;
	f->close();

	if (!ifarg(1)) {
		return double(f->file_chooser_popup());
	}

	char *type, *banner, *filter, *bopen, *cancel;
	banner=filter=bopen=cancel=NULL;
	const char *path=".";
	type = gargstr(1);
	if (ifarg(2)) {
		banner = gargstr(2);
	}
	if (ifarg(3)) {
		filter = gargstr(3);
	}
	if (ifarg(4)) {
		bopen = gargstr(4);
	}
	if (ifarg(5)) {
		cancel = gargstr(5);
	}
	if (ifarg(6)) {
		path = gargstr(6);
	}

	f->file_chooser_style(type, path, banner, filter, bopen, cancel);
ENDGUI
#endif
	return 1.;
}

static double f_close(void* v){
	OcFile* f = (OcFile*)v;
	f->close();
	return 0.;
}

static double f_vwrite(void* v){
	OcFile* f = (OcFile*)v;
	size_t n = 1;
	if (ifarg(2)) n = long(chkarg(1, 1., 2.e18));
	const char* x = (const char*)hoc_pgetarg(ifarg(2)+1);
	BinaryMode(f)
	return (double)fwrite(x,sizeof(double),n,f->file());
}

static double f_vread(void* v){
	OcFile* f = (OcFile*)v;
	size_t n = 1;
	if (ifarg(2)) n = int(chkarg(1, 1., 2.e18));
	char* x = (char*)hoc_pgetarg(ifarg(2)+1);
	BinaryMode(f)
	return (double)fread(x,sizeof(double),n,f->file());
}

static double f_seek(void* v){
	OcFile* f = (OcFile*)v;
	long n = 0;
	int base = 0;
	if (ifarg(1)) {
	  // no longer check since since many machines have >2GB files
	  n = long (*getarg(1));
	}
	if (ifarg(2)) {
	  base = int(chkarg(2, 0., 2.));
	}
	BinaryMode(f)
	return (double)fseek(f->file(),n,base);
}

static double f_tell(void* v){
	OcFile* f = (OcFile*)v;
	BinaryMode(f)
	return (double)ftell(f->file());
}

static void* f_cons(Object*) {
	OcFile* f =  new OcFile();
	if (ifarg(1)) {
		f->set_name(gargstr(1));
	}
	return f;
}

static void f_destruct(void* v) {
	delete (OcFile*)v;
}

Member_func f_members[] = {
	"ropen",	f_ropen,
	"wopen",	f_wopen,
	"aopen",	f_aopen,
	"printf",	f_printf,
	"scanvar",	f_scanvar,
	"scanstr",	f_scanstr,
	"gets",		f_gets,
	"eof",		f_eof,
	"isopen",	f_is_open,
	"chooser",	f_chooser,
	"close",	f_close,
	"vwrite",       f_vwrite,
	"vread",        f_vread,
	"seek",         f_seek,
	"tell",         f_tell,
	"mktemp",	f_mktemp,
	"unlink", 	f_unlink,
	"flush",	f_flush,
	0, 0
};

static Member_ret_str_func f_retstr_members[] = {
	"getname",	f_get_name,
	"dir",	f_dir,
	0,0
};

void OcFile_reg() {
	class2oc("File", f_cons, f_destruct, f_members, NULL, NULL, f_retstr_members);
}

void OcFile::close() {
	if (file_) {
		fclose(file_);
	}
	file_ = NULL;
}
void OcFile::set_name(const char* s) {
	close();
	if (s != filename_.string()) {
		filename_ = s;
	}
}

#ifdef WIN32
void OcFile::binary_mode() {
	if (file() && !binary_) {
		if (ftell(file()) != 0) {
			hoc_execerror(get_name(),
":can switch to dos binary file mode only at beginning of file.\n\
 Use File.seek(0) after opening or use a binary style read/write as first\n\
 access to file.");
 		}
#if defined(__MWERKS__)
//printf("can't switch to binary mode. No setmode\n");
		mode_[1] = 'b';  mode_[2] = '\0';
		file_ = freopen(filename_.string(), mode_, file());
#else 
		setmode(fileno(file()), O_BINARY);
#endif
		binary_ = true;
	}
}
#endif

bool OcFile::open(const char* name, const char* type) {
	set_name(name);
#ifdef WIN32
	binary_ = false;
	strcpy(mode_, type);
#endif
	file_ = fopen(expand_env_var(name), type);
#if defined(FILE_OPEN_RETRY) && FILE_OPEN_RETRY > 0
	int i;
	for (i=0; !file_ && i < FILE_OPEN_RETRY; ++i) {
		// retry occasionally needed on BlueGene
		file_ = fopen(expand_env_var(name), type);
	}
	if (i > 0) {
		if (file_) {
printf("%d opened %s after %d retries\n", nrnmpi_myid_world, name, i);
		}else{
printf("%d open %s failed after %d retries\n", nrnmpi_myid_world, name, i);
		}
	}
#endif
	return is_open();
}

FILE* OcFile::file() {
	if (!file_) {
		hoc_execerror(get_name(), ":file is not open");
	}
	return file_;
}

bool OcFile::eof() {
	int c;
	c = getc(file());
	return ungetc(c, file()) == EOF;
}

bool OcFile::mktemp() {
	char* s = ivoc_get_temp_file();
	if (s) {
		set_name(s);
		delete [] s;
		return true;
	}
	return false;
}

bool OcFile::unlink() {
	int i = ivoc_unlink(get_name());
	return i == 0;
}

void OcFile::file_chooser_style(const char* type, const char* path, const char* banner,
	const char* filter, const char* bopen, const char* cancel)
{
#if HAVE_IV
	Resource::unref(fc_);

	Style* style = new Style(Session::instance()->style());
	style->ref();
	bool nocap = true;
	if (banner) {
		if (banner[0]) {
			style->attribute("caption", banner);
			nocap = false;
		}
	}
	if (filter) {
		if (filter[0]) {
			style->attribute("filter", "true");
			style->attribute("filterPattern", filter);
		}
	}
	if (bopen) {
		if (bopen[0]) {
			style->attribute("open", bopen);
		}
	}else if (type[0] == 'w') {
		style->attribute("open", "Save");
	}
	if (cancel) {
		if (cancel[0]) {
			style->attribute("cancel", cancel);
		}
	}
	if (nocap) switch( type[0] ) {
	case 'w':
		style->attribute("caption", "File write");
		break;
	case 'a':
		style->attribute("caption", "File append");
		break;
	case 'r':
		style->attribute("caption", "File read");
		break;
	case 'd':
		style->attribute("caption", "Directory open");
		break;
	case '\0':
		style->attribute("caption", "File name only");
		break;
	}
	switch( type[0] ) {
	case 'w':
		chooser_type_ = W;
		break;
	case 'a':
		chooser_type_ = A;
		break;
	case 'r':
		chooser_type_ = R;
		break;
	case 'd':
		chooser_type_ = N;
		style->attribute("choose_directory", "on");
		break;
	case '\0':
		chooser_type_ = N;
		break;
	}
	fc_ = DialogKit::instance()->file_chooser(path, style);
	fc_->ref();
	style->unref();
#endif
}

const char* OcFile::dir() {
#if HAVE_IV
	if (fc_) {
		dirname_ = *fc_->dir();
	}else
#endif
	{
		dirname_ = "";
	}
	return dirname_.string();
}

bool OcFile::file_chooser_popup() {
#if HAVE_IV
	bool accept = false;
	if (!fc_) {
		hoc_execerror("First call to file_chooser must at least specify r or w", 0);
	}

	Display* d = Session::instance()->default_display();
	while (fc_->post_at(d->width()/2, d->height()/2)) {
		switch( chooser_type_ ) {
		case W:
			if (ok_to_write(*fc_->selected(), NULL)) {
				open(fc_->selected()->string(), "w");
				accept = true;
			}
			break;
		case A:
			if (ok_to_write(*fc_->selected(), NULL)) {
				open(fc_->selected()->string(), "a");
				accept = true;
			}
			break;
		case R:
#if 1
			if (ok_to_read(*fc_->selected(), NULL)) {
				open(fc_->selected()->string(), "r");
				accept = true;
			}
#else
accept = true;
#endif
			break;
		case N:
			set_name(fc_->selected()->string());
			accept = true;
		}
		if (accept) {
			break;
		}
	}
	return accept;
#else
	return false;
#endif
}


