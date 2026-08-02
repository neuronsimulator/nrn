// H4c hand-edit CURRENT experiment (2026-08-02) — NOT product codegen.
// build-gpu/src/nrnoc/hh.cpp only. With STATE hand-edit also applied.
// Baseline (state hand-edit only): nrn_cur_hh avg ~18 us
// After: nrn_cur_hh avg ~13 us (~28%%); wall multi-warm ~1.51-1.54 s (was ~1.68-1.75)
// Live SoA present: gnabar,gkbar,gl,el,m,h,n,g_unused (8 cols). Ions via deviceptr.
// Inlined numerical di/dv; no 25-arg nrn_current; no RANGE shadows for ena/ina in present.

/** update current */
    // H4c hand-edit: minimal present for CURRENT (not product codegen).
    static void nrn_cur_hh(const _nrn_model_sorted_token& _sorted_token, NrnThread* nt, Memb_list* _ml_arg, int _type) {
        if (nt->compute_gpu && !hh_global_gpu_resident) {
            (void) nrn_target_copyin(&hh_global, 1);
            hh_global_gpu_resident = true;
            hh_global_device_stale = false;
        }
        auto nodecount = _ml_arg->nodecount;
        double const _nrn_thread_t = nt->_t;
        (void) _nrn_thread_t;
        nrn_pragma_acc(data present(nt, _ml_arg, hh_global) if(nt->compute_gpu))
        {
            _nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml_arg, _ml_arg->type()};
            auto node_data = make_node_data_hh(*nt, *_ml_arg, nt->compute_gpu);
            if (nt->compute_gpu && hh_global_gpu_resident && hh_global_device_stale) {
                nrn_pragma_acc(update device (hh_global))
                nrn_pragma_omp(target update to(hh_global))
                hh_global_device_stale = false;
            }
            auto const* nodeindices = node_data.nodeindices;
            double* _d_voltages = nt->compute_gpu ? static_cast<double*>(acc_deviceptr(const_cast<double*>(node_data.node_voltages))) : const_cast<double*>(node_data.node_voltages);
            double* vec_rhs = node_data.node_rhs;
            // Live SoA for CURRENT: params, m/h/n, g_unused. Rates temps / Dm* unused.
            // Ion ena/ek/ina/ik as locals + dptrs (not RANGE present for shadows if avoidable).
            double* gnabar = _lmc.template fpfield_ptr<0>();
            double* gkbar = _lmc.template fpfield_ptr<1>();
            double* gl = _lmc.template fpfield_ptr<2>();
            double* el = _lmc.template fpfield_ptr<3>();
            double* m = _lmc.template fpfield_ptr<13>();
            double* h = _lmc.template fpfield_ptr<14>();
            double* n = _lmc.template fpfield_ptr<15>();
            double* g_unused = _lmc.template fpfield_ptr<24>();
            double* const* ion_ena = nullptr;
            int ion_ena_base = 0;
            double* const* ion_ina = nullptr;
            int ion_ina_base = 0;
            double* const* ion_dinadv = nullptr;
            int ion_dinadv_base = 0;
            double* const* ion_ek = nullptr;
            int ion_ek_base = 0;
            double* const* ion_ik = nullptr;
            int ion_ik_base = 0;
            double* const* ion_dikdv = nullptr;
            int ion_dikdv_base = 0;
            if (nt->compute_gpu && neuron::mechanism::_get::gpu_pdata_ptr_cache(_sorted_token, _ml_arg->type())) {
                auto* cache = neuron::mechanism::_get::gpu_pdata_ptr_cache(_sorted_token, _ml_arg->type());
                int base = static_cast<int>(_ml_arg->get_storage_offset());
                ion_ena = cache[0]; ion_ena_base = base;
                ion_ina = cache[1]; ion_ina_base = base;
                ion_dinadv = cache[2]; ion_dinadv_base = base;
                ion_ek = cache[3]; ion_ek_base = base;
                ion_ik = cache[4]; ion_ik_base = base;
                ion_dikdv = cache[5]; ion_dikdv_base = base;
            } else {
                ion_ena = _lmc.template dptr_field_ptr<0>();
                ion_ina = _lmc.template dptr_field_ptr<1>();
                ion_dinadv = _lmc.template dptr_field_ptr<2>();
                ion_ek = _lmc.template dptr_field_ptr<3>();
                ion_ik = _lmc.template dptr_field_ptr<4>();
                ion_dikdv = _lmc.template dptr_field_ptr<5>();
            }
            // present: 8 SoA columns (not 25) + matrix + ions as deviceptr
            nrn_pragma_acc(parallel loop present(_ml_arg, nt, nodeindices, vec_rhs[:nt->end], gnabar[:static_cast<std::size_t>(_ml_arg->nodecount)], gkbar[:static_cast<std::size_t>(_ml_arg->nodecount)], gl[:static_cast<std::size_t>(_ml_arg->nodecount)], el[:static_cast<std::size_t>(_ml_arg->nodecount)], m[:static_cast<std::size_t>(_ml_arg->nodecount)], h[:static_cast<std::size_t>(_ml_arg->nodecount)], n[:static_cast<std::size_t>(_ml_arg->nodecount)], g_unused[:static_cast<std::size_t>(_ml_arg->nodecount)]) deviceptr(_d_voltages, ion_ena, ion_ina, ion_dinadv, ion_ek, ion_ik, ion_dikdv) async(nt->stream_id) if(nt->compute_gpu))
            nrn_pragma_omp(target teams distribute parallel for if(nt->compute_gpu))
            for (int id = 0; id < nodecount; id++) {
                int node_id = nodeindices[id];
                double v = _d_voltages[node_id];
                double ena = (*ion_ena[id + ion_ena_base]);
                double ek = (*ion_ek[id + ion_ek_base]);
                double mm = m[id], hh = h[id], nn = n[id];
                // I(v+0.001)
                double gna1 = gnabar[id] * mm * mm * mm * hh;
                double ina1 = gna1 * ((v + 0.001) - ena);
                double gk1 = gkbar[id] * nn * nn * nn * nn;
                double ik1 = gk1 * ((v + 0.001) - ek);
                double il1 = gl[id] * ((v + 0.001) - el[id]);
                double I1 = il1 + ina1 + ik1;
                // I(v)
                double gna = gnabar[id] * mm * mm * mm * hh;
                double ina = gna * (v - ena);
                double gk = gkbar[id] * nn * nn * nn * nn;
                double ik = gk * (v - ek);
                double il = gl[id] * (v - el[id]);
                double I0 = il + ina + ik;
                double g = (I1 - I0) / 0.001;
                (*ion_dinadv[id + ion_dinadv_base]) += (ina1 - ina) / 0.001;
                (*ion_dikdv[id + ion_dikdv_base]) += (ik1 - ik) / 0.001;
                (*ion_ina[id + ion_ina_base]) += ina;
                (*ion_ik[id + ion_ik_base]) += ik;
                vec_rhs[node_id] -= I0;
                g_unused[id] = g;
            }
            if(nt->compute_gpu) {
                nrn_pragma_acc(wait(nt->stream_id))
            }
        }
    }


