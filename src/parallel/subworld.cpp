// included by ocbbs.cpp

void BBSImpl::subworld_worker_execute() {
	// execute the same thing that execute_worker is executing. This
	// is done for all the nrnmpi_myid_bbs == -1 workers associated with
	// the specific nrnmpi_myid == 0 with nrnmpi_myid_bbs >= 0.
	// All the nrnmpi/mpispike.c functions can be used since the
	// proper communicators for a subworld are used by those functions.
	// The broadcast functions are particularly useful and those are
	// how execute_worker passes messages into here.

printf("%d enter subworld_worker_execute\n", nrnmpi_myid_world);
	int info[2];
	// wait for something to do
	nrnmpi_int_broadcast(info, 2, 0);
	// info[0] = -1 means it was from a pc.context. Also -2 means
	// DONE.
printf("%d subworld_worker_execute info %d %d\n", nrnmpi_myid_world, info[0], info[1]);
	int id = info[0];
	if (id == -2) { // DONE, so quit.
		done();
	}
	hoc_ac_ = double(id);
	int style = info[1];
	if (style == 0) { // hoc statement form
		int size;
		nrnmpi_int_broadcast(&size, 1, 0); // includes terminator
		char *s = new char[size];
		nrnmpi_char_broadcast(s, size, 0);
		hoc_obj_run(s, nil);
		delete [] s;
printf("%d leave subworld_worker_execute\n", nrnmpi_myid_world);
		return;
	}
}


