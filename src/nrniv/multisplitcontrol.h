#ifndef multisplitcontrol_h
#define multisplitcontrol_h

class MultiSplitThread {
public:
	MultiSplitThread();
	virtual ~MultiSplitThread();
	
	void del_sidA();
	void triang(NrnThread*);
	void bksub(NrnThread*);
	void triang_subtree2backbone(NrnThread*);
	void triang_backbone(NrnThread*);
	void bksub_backbone(NrnThread*);
	void bksub_short_backbone_part1(NrnThread*);
	void bksub_subtrees(NrnThread*);
	void v_setup(NrnThread*);

	double *sid1A, *sid1B; // to be filled in sid1 and sid0 columns
	int* sid0i; // interior node to sid0 index. parallel to sid1B
	// for mapping sid1A... to transfer buffer when ReducedTree not on this machine
	int nbackrt_; // number of backbones that send info to ReducedTree
	int* backsid_; // sid0
	int* backAindex_; // sid1A index for sid0
	int* backBindex_; // sid1B index for sid1
	int backbone_begin, backbone_long_begin, backbone_interior_begin;
	int backbone_sid1_begin, backbone_long_sid1_begin, backbone_end;
	int i1, i2, i3;
};

class MultiSplitControl {
public:
	MultiSplitControl();
	virtual ~MultiSplitControl();
	
	void multisplit_clear();
	void multisplit_nocap_v();
	void multisplit_nocap_v_part1(NrnThread*);
	void multisplit_nocap_v_part2(NrnThread*);
	void multisplit_nocap_v_part3(NrnThread*);
	void multisplit_adjust_rhs(NrnThread*);
	void prstruct();
	void reduce_solve();

	void multisplit(double, int, int);
	void solve();
	void reduced_mark(int, int, int, int*, int*, int*);
	void matrix_exchange();
	void matrix_exchange_nocap();
	void v_setup();
	void exchange_setup();
	void rt_map_update();
	void del_msti();
	void pmat(bool full = false);
	void pmatf(bool full = false);
	void pmat1(const char*);
	void pexch();

	int narea2buf_, narea2rt_;
	Area2Buf* area2buf_;
	Area2RT* area2rt_;

	int nthost_; // number of distinct hosts that need send-receive
	int ihost_reduced_long_, ihost_short_long_; // indices for groups
	MultiSplitTransferInfo* msti_; // will be nthost_ of them
	int tbsize;
	int ndbsize;
	double* trecvbuf_; //enough buffer for all receives
	double* tsendbuf_; // enough for all send
	int* nodeindex_buffer_; // nodeindex_ points into here
	int* nodeindex_buffer_th_; // thread for above
	int* nodeindex_rthost_; // ReducedTree machine that gets this node info. Normally -1.
	int narea_; // number of transfer nodes that need area adjustment
	int iarea_short_long_;// different ones get adjusted at different times
	int* buf_area_indices_;
	int* area_node_indices_;

	int nrtree_;
	ReducedTree** rtree_;

	MultiSplitTable* classical_root_to_multisplit_;
	MultiSplitList* multisplit_list_; // NrnHashIterate is not in insertion order

	int nth_;
	MultiSplitThread* mth_;
};

#endif
