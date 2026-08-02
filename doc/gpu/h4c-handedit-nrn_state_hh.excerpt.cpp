// H4c hand-edit experiment (2026-08-02) — NOT product codegen.
// Applied to build-gpu/src/nrnoc/hh.cpp only; nmodl regen wipes this.
// Result: TABLE state_hh ACC_TIME avg ~18 us (was ~130-150); product 688 dV=0.
// Combined changes: (1) rates temps as stack locals (2) present only m,h,n
// (3) rates inlined into STATE (4) no 25-arg rates_hh call.
// Baseline post-H4a/H4b ~131 us TABLE / CN ~19 us.

static void nrn_state_hh(const _nrn_model_sorted_token& _sorted_token, NrnThread* nt, Memb_list* _ml_arg, int _type) {
        if (nt->compute_gpu && !hh_global_gpu_resident) {
            (void) nrn_target_copyin(&hh_global, 1);
            hh_global_gpu_resident = true;
            hh_global_device_stale = false;
        }
        nrn_pragma_acc(data present(nt, _ml_arg, hh_global) if(nt->compute_gpu))
        {
            _nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml_arg, _ml_arg->type()};
            auto inst = make_instance_hh(_ml_arg->get_storage_offset(), &_lmc, nt->compute_gpu);
            auto node_data = make_node_data_hh(*nt, *_ml_arg, nt->compute_gpu);
            auto* _thread = _ml_arg->_thread;
            auto nodecount = _ml_arg->nodecount;
            double const _nrn_thread_t = nt->_t;
            (void) _nrn_thread_t;
            if (nt->compute_gpu && hh_global_gpu_resident && hh_global_device_stale) {
                nrn_pragma_acc(update device (hh_global))
                nrn_pragma_omp(target update to(hh_global))
                hh_global_device_stale = false;
            }
            auto const* nodeindices = node_data.nodeindices;
            double* _d_voltages = nt->compute_gpu ? static_cast<double*>(acc_deviceptr(const_cast<double*>(node_data.node_voltages))) : const_cast<double*>(node_data.node_voltages);
            // H4c hand-edit experiment: rates temps on stack; STATE only needs m,h,n SoA.
            // Do not re-run nmodl on hh.mod or this edit is wiped.
            double* _present_fp_13 = _lmc.template fpfield_ptr<13>(); /* m */
            double* _present_fp_14 = _lmc.template fpfield_ptr<14>(); /* h */
            double* _present_fp_15 = _lmc.template fpfield_ptr<15>(); /* n */
            nrn_pragma_acc(parallel loop present(_ml_arg, nt, nodeindices, _present_fp_13[:static_cast<std::size_t>(_ml_arg->nodecount) * 1], _present_fp_14[:static_cast<std::size_t>(_ml_arg->nodecount) * 1], _present_fp_15[:static_cast<std::size_t>(_ml_arg->nodecount) * 1], hh_global) deviceptr(_d_voltages) async(nt->stream_id) if(nt->compute_gpu))
            nrn_pragma_omp(target teams distribute parallel for if(nt->compute_gpu))
            for (int id = 0; id < nodecount; id++) {
                int node_id = nodeindices[id];
                double v = _d_voltages[node_id];
                double minf, hinf, ninf, mtau, htau, ntau;
                // TABLE path (product default usetable=1). Analytic: same locals via f_rates math.
                if (hh_global.usetable == 0) {
                    double alpha, beta, sum, q10, vtrap_in_0, vtrap_in_1;
                    q10 = pow(3.0, ((celsius - 6.3) / 10.0));
                    {
                        double _lx = -(v + 40.0), _ly = 10.0;
                        if (fabs(_lx / _ly) < 1e-6) {
                            vtrap_in_0 = _ly * (1.0 - _lx / _ly / 2.0);
                        } else {
                            vtrap_in_0 = _lx / (exp(_lx / _ly) - 1.0);
                        }
                    }
                    alpha = .1 * vtrap_in_0;
                    beta = 4.0 * exp(-(v + 65.0) / 18.0);
                    sum = alpha + beta;
                    mtau = 1.0 / (q10 * sum);
                    minf = alpha / sum;
                    alpha = .07 * exp(-(v + 65.0) / 20.0);
                    beta = 1.0 / (exp(-(v + 35.0) / 10.0) + 1.0);
                    sum = alpha + beta;
                    htau = 1.0 / (q10 * sum);
                    hinf = alpha / sum;
                    {
                        double _lx = -(v + 55.0), _ly = 10.0;
                        if (fabs(_lx / _ly) < 1e-6) {
                            vtrap_in_1 = _ly * (1.0 - _lx / _ly / 2.0);
                        } else {
                            vtrap_in_1 = _lx / (exp(_lx / _ly) - 1.0);
                        }
                    }
                    alpha = .01 * vtrap_in_1;
                    beta = .125 * exp(-(v + 65.0) / 80.0);
                    sum = alpha + beta;
                    ntau = 1.0 / (q10 * sum);
                    ninf = alpha / sum;
                } else {
                    double xi = hh_global.mfac_rates * (v - hh_global.tmin_rates);
                    if (isnan(xi)) {
                        minf = hinf = ninf = mtau = htau = ntau = xi;
                    } else if (xi <= 0. || xi >= 200.) {
                        int index = (xi <= 0.) ? 0 : 200;
                        minf = hh_global.t_minf[index];
                        mtau = hh_global.t_mtau[index];
                        hinf = hh_global.t_hinf[index];
                        htau = hh_global.t_htau[index];
                        ninf = hh_global.t_ninf[index];
                        ntau = hh_global.t_ntau[index];
                    } else {
                        int i = int(xi);
                        double theta = xi - double(i);
                        minf = hh_global.t_minf[i] + theta * (hh_global.t_minf[i + 1] - hh_global.t_minf[i]);
                        mtau = hh_global.t_mtau[i] + theta * (hh_global.t_mtau[i + 1] - hh_global.t_mtau[i]);
                        hinf = hh_global.t_hinf[i] + theta * (hh_global.t_hinf[i + 1] - hh_global.t_hinf[i]);
                        htau = hh_global.t_htau[i] + theta * (hh_global.t_htau[i + 1] - hh_global.t_htau[i]);
                        ninf = hh_global.t_ninf[i] + theta * (hh_global.t_ninf[i + 1] - hh_global.t_ninf[i]);
                        ntau = hh_global.t_ntau[i] + theta * (hh_global.t_ntau[i + 1] - hh_global.t_ntau[i]);
                    }
                }
                // No SoA store of minf/mtau/... — integrate m,h,n only.
                double m = _present_fp_13[id];
                double h = _present_fp_14[id];
                double n = _present_fp_15[id];
                _present_fp_13[id] = minf - (minf - m) * exp(-nt->_dt / mtau);
                _present_fp_14[id] = hinf - (hinf - h) * exp(-nt->_dt / htau);
                _present_fp_15[id] = ninf - (ninf - n) * exp(-nt->_dt / ntau);
            }
            if(nt->compute_gpu) {
                nrn_pragma_acc(wait(nt->stream_id))
            }
        }
    }


