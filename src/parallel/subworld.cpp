// included by ocbbs.cpp

void BBSImpl::subworld_worker_execute() {
    // execute the same thing that execute_worker is executing. This
    // is done for all the nrnmpi_myid_bbs == -1 workers associated with
    // the specific nrnmpi_myid == 0 with nrnmpi_myid_bbs >= 0.
    // All the nrnmpi/mpispike.cpp functions can be used since the
    // proper communicators for a subworld are used by those functions.
    // The broadcast functions are particularly useful and those are
    // how execute_worker passes messages into here.

    // printf("%d enter subworld_worker_execute\n", nrnmpi_myid_world);
    int info[2];
    // wait for something to do
    nrnmpi_int_broadcast(info, 2, 0);
    // info[0] = -1 means it was from a pc.context. Also -2 means
    // DONE.
    // printf("%d subworld_worker_execute info %d %d\n", nrnmpi_myid_world, info[0], info[1]);
    int id = info[0];
    if (id == -2) {  // DONE, so quit.
        done();
    }
    hoc_ac_ = double(id);
    int style = info[1];
    if (style == 0) {  // hoc statement form
        int size;
        nrnmpi_int_broadcast(&size, 1, 0);  // includes terminator
        char* s = new char[size];
        nrnmpi_char_broadcast(s, size, 0);
        hoc_obj_run(s, nullptr);
        delete[] s;
        // printf("%d leave subworld_worker_execute\n", nrnmpi_myid_world);
        return;
    }
    int i, j;
    int npickle;
    char* s;
    Symbol* fname = 0;
    Object* ob = nullptr;
    char* sarg[20];  // up to 20 arguments may be strings
    int ns = 0;      // number of args that are strings
    int narg = 0;    // total number of args

    if (style == 3) {  // python callable
        nrnmpi_int_broadcast(&npickle, 1, 0);
        s = new char[npickle];
        nrnmpi_char_broadcast(s, npickle, 0);
    } else if (style == 1) {  // hoc function
        int size;
        nrnmpi_int_broadcast(&size, 1, 0);  // includes terminator
        // printf("%d subworld hoc function string size = %d\n", nrnmpi_myid_world, size);
        s = new char[size];
        nrnmpi_char_broadcast(s, size, 0);
        fname = hoc_lookup(s);
        if (!fname) {
            return;
        }  // error raised by sender
    } else {
        return;  // no others implemented, error raised by sender
    }

    // now get the args
    int argtypes;
    nrnmpi_int_broadcast(&argtypes, 1, 0);
    // printf("%d subworld argtypes = %d\n", nrnmpi_myid_world, argtypes);
    for (j = argtypes; (i = j % 5) != 0; j /= 5) {
        ++narg;
        if (i == 1) {  // double
            double x;
            nrnmpi_dbl_broadcast(&x, 1, 0);
            // printf("%d subworld scalar = %g\n", nrnmpi_myid_world, x);
            hoc_pushx(x);
        } else if (i == 2) {  // string
            int size;
            nrnmpi_int_broadcast(&size, 1, 0);
            sarg[ns] = new char[size];
            nrnmpi_char_broadcast(sarg[ns], size, 0);
            hoc_pushstr(sarg + ns);
            ns++;
        } else if (i == 3) {  // Vector
            int n;
            nrnmpi_int_broadcast(&n, 1, 0);
            Vect* vec = new Vect(n);
            nrnmpi_dbl_broadcast(vec->data(), n, 0);
            hoc_pushobj(vec->temp_objvar());
        } else {  // PythonObject
            int n;
            nrnmpi_int_broadcast(&n, 1, 0);
            char* s;
            s = new char[n];
            nrnmpi_char_broadcast(s, n, 0);
            Object* po = neuron::python::methods.pickle2po(s, size_t(n));
            delete[] s;
            hoc_pushobj(hoc_temp_objptr(po));
        }
    }

    if (style == 3) {
        size_t size;
        char* rs = neuron::python::methods.call_picklef(s, size_t(npickle), narg, &size);
        assert(rs);
        delete[] rs;
    } else {
        // printf("%d subworld hoc call %s narg=%d\n", nrnmpi_myid_world, fname->name, narg);
        hoc_call_objfunc(fname, narg, ob);
        // printf("%d subworld return from hoc call %s\n", nrnmpi_myid_world, fname->name);
    }
    delete[] s;
    for (i = 0; i < ns; ++i) {
        delete[] sarg[i];
    }
}
