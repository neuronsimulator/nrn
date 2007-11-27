#include <objcmd.h>
#include <pool.h>
#include <netcon.h>

declarePool(HocEventPool, HocEvent)
implementPool(HocEventPool, HocEvent)
HocEventPool* HocEvent::hepool_;  

HocEvent::HocEvent() {
        stmt_ = nil;
}

HocEvent::~HocEvent() {
        if (stmt_) {
                delete stmt_;
        }
}

void HocEvent::pr(const char* s, double tt, NetCvode* ns) {
	printf("%s HocEvent %s %.15g\n", s, stmt_ ? stmt_->name() : "", tt);
}

HocEvent* HocEvent::alloc(const char* stmt) {
        if (!hepool_) {
                hepool_ = new HocEventPool(100);
        }
        HocEvent* he = hepool_->alloc();
        he->stmt_ = nil;
        if (stmt) {
                he->stmt_ = new HocCommand(stmt);
        }
        return he; 
}

void HocEvent::hefree() {
        if (stmt_) {
                delete stmt_;
                stmt_ = nil;
        }
        hepool_->hpfree(this);
}               

void HocEvent::clear() {
        if (stmt_) {
                delete stmt_;
                stmt_ = nil;
        }
}               

void HocEvent::deliver(double tt, NetCvode*) {
	if (stmt_) {
		t = tt;
#if carbon
		stmt_->execute((unsigned int)0);
#else
		stmt_->execute(false);
#endif
	}
        hefree();
}
void HocEvent::pgvts_deliver(double tt, NetCvode*) {
	deliver(tt, nil);
}

void HocEvent::reclaim() {
	if (hepool_) {
		hepool_->free_all();
	}
}

DiscreteEvent* HocEvent::savestate_save() {
//	pr("HocEvent::savestate_save", 0, net_cvode_instance);
	HocEvent* he = new HocEvent();
	if (stmt_) {
		he->stmt_ = new HocCommand(stmt_->name(), stmt_->object());
	}
	return he;
}

void HocEvent::savestate_restore(double tt, NetCvode* nc) {
//	pr("HocEvent::savestate_restore", tt, nc);
	HocEvent* he = alloc(nil);
	if (stmt_) {
		he->stmt_ = new HocCommand(stmt_->name(), stmt_->object());
	}
	nc->event(tt, he);
}

DiscreteEvent* HocEvent::savestate_read(FILE* f) {
	HocEvent* he = new HocEvent();
	int have_stmt, have_obj, index;
	char stmt[256], objname[100], buf[200];
	Object* obj = nil;
//	assert(fscanf(f, "%d %d\n", &have_stmt, &have_obj) == 2);
	fgets(buf, 200, f);
	assert(sscanf(buf, "%d %d\n", &have_stmt, &have_obj) == 2);	
	if (have_stmt) {
		fgets(stmt, 256, f);
		stmt[strlen(stmt)-1] = '\0';
		if (have_obj) {
//			assert(fscanf(f, "%s %d\n", objname, &index) == 1);
			fgets(buf, 200, f);
			assert(sscanf(buf, "%s %d\n", objname, &index) == 1);
			obj = hoc_name2obj(objname, index);
		}
		he->stmt_ = new HocCommand(stmt, obj);
	}
	return he;
}

void HocEvent::savestate_write(FILE* f) {
	fprintf(f, "%d\n", HocEventType);
	fprintf(f, "%d %d\n", stmt_ ? 1 : 0, (stmt_ && stmt_->object()) ? 1 : 0);
	if (stmt_) {
		fprintf(f, "%s\n", stmt_->name());
		if (stmt_->object()) {
			fprintf(f, "%s %d\n", stmt_->object()->ctemplate->sym->name,
				stmt_->object()->index);
		}
	}
}

