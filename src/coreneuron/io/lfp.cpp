#include "coreneuron/io/lfp.hpp"
#include "coreneuron/apps/corenrn_parameters.hpp"

#include <cmath>
#include <limits>
#include <sstream>


namespace coreneuron {
// extern variables require acc declare
#pragma acc declare create(pi)

namespace lfputils {

double line_source_lfp_factor(const Point3D& e_pos,
                              const Point3D& seg_0,
                              const Point3D& seg_1,
                              const double radius,
                              const double f) {
    nrn_assert(radius >= 0.0);
    Point3D dx = paxpy(seg_1, -1.0, seg_0);
    Point3D de = paxpy(e_pos, -1.0, seg_0);
    double dx2(dot(dx, dx));
    double dxn(std::sqrt(dx2));
    if (dxn < std::numeric_limits<double>::epsilon()) {
        return point_source_lfp_factor(e_pos, seg_0, radius, f);
    }
    double de2(dot(de, de));
    double mu(dot(dx, de) / dx2);
    Point3D de_star(paxpy(de, -mu, dx));
    double de_star2(dot(de_star, de_star));
    double q2(de_star2 / dx2);

    double delta(mu * mu - (de2 - radius * radius) / dx2);
    double one_m_mu(1.0 - mu);
    auto log_integral = [&q2, &dxn](double a, double b) {
        if (q2 < std::numeric_limits<double>::epsilon()) {
            if (a * b <= 0) {
                std::ostringstream s;
                s << "Log integral: invalid arguments " << b << " " << a
                  << ". Likely electrode exactly on the segment and "
                  << "no flooring is present.";
                throw std::invalid_argument(s.str());
            }
            return std::abs(std::log(b / a)) / dxn;
        } else {
            return std::log((b + std::sqrt(b * b + q2)) / (a + std::sqrt(a * a + q2))) / dxn;
        }
    };
    if (delta <= 0.0) {
        return f * log_integral(-mu, one_m_mu);
    } else {
        double sqr_delta(std::sqrt(delta));
        double d1(mu - sqr_delta);
        double d2(mu + sqr_delta);
        double parts = 0.0;
        if (d1 > 0.0) {
            double b(std::min(d1, 1.0) - mu);
            parts += log_integral(-mu, b);
        }
        if (d2 < 1.0) {
            double b(std::max(d2, 0.0) - mu);
            parts += log_integral(b, one_m_mu);
        };
        // complement
        double maxd1_0(std::max(d1, 0.0)), mind2_1(std::min(d2, 1.0));
        if (maxd1_0 < mind2_1) {
            parts += 1.0 / radius * (mind2_1 - maxd1_0);
        }
        return f * parts;
    };
}
}  // namespace lfputils

using namespace lfputils;

template <LFPCalculatorType Type, typename SegmentIdTy>
LFPCalculator<Type, SegmentIdTy>::LFPCalculator(const Point3Ds& seg_start,
                                                const Point3Ds& seg_end,
                                                const std::vector<double>& radius,
                                                const std::vector<SegmentIdTy>& segment_ids,
                                                const Point3Ds& electrodes,
                                                double extra_cellular_conductivity)
    : segment_ids_(segment_ids) {
    if (seg_start.size() != seg_end.size()) {
        throw std::invalid_argument("Different number of segment starts and ends.");
    }
    if (seg_start.size() != radius.size()) {
        throw std::invalid_argument("Different number of segments and radii.");
    }
    double f(1.0 / (extra_cellular_conductivity * 4.0 * pi));

    m.resize(electrodes.size());
    for (size_t k = 0; k < electrodes.size(); ++k) {
        auto& ms = m[k];
        ms.resize(seg_start.size());
        for (size_t l = 0; l < seg_start.size(); l++) {
            ms[l] = getFactor(electrodes[k], seg_start[l], seg_end[l], radius[l], f);
        }
    }
}

template <LFPCalculatorType Type, typename SegmentIdTy>
template <typename Vector>
inline void LFPCalculator<Type, SegmentIdTy>::lfp(const Vector& membrane_current) {
    std::vector<double> res(m.size());
    for (size_t k = 0; k < m.size(); ++k) {
        res[k] = 0.0;
        auto& ms = m[k];
        for (size_t l = 0; l < ms.size(); l++) {
            res[k] += ms[l] * membrane_current[segment_ids_[l]];
        }
    }
#if NRNMPI
    if (corenrn_param.mpi_enable) {
        lfp_values_.resize(res.size());
        int mpi_sum{1};
        nrnmpi_dbl_allreduce_vec(res.data(), lfp_values_.data(), res.size(), mpi_sum);
    } else
#endif
    {
        std::swap(res, lfp_values_);
    }
}


template LFPCalculator<LineSource>::LFPCalculator(const lfputils::Point3Ds& seg_start,
                                                  const lfputils::Point3Ds& seg_end,
                                                  const std::vector<double>& radius,
                                                  const std::vector<int>& segment_ids,
                                                  const lfputils::Point3Ds& electrodes,
                                                  double extra_cellular_conductivity);
template LFPCalculator<PointSource>::LFPCalculator(const lfputils::Point3Ds& seg_start,
                                                   const lfputils::Point3Ds& seg_end,
                                                   const std::vector<double>& radius,
                                                   const std::vector<int>& segment_ids,
                                                   const lfputils::Point3Ds& electrodes,
                                                   double extra_cellular_conductivity);
template void LFPCalculator<LineSource>::lfp(const DoublePtr& membrane_current);
template void LFPCalculator<PointSource>::lfp(const DoublePtr& membrane_current);
template void LFPCalculator<LineSource>::lfp(const std::vector<double>& membrane_current);
template void LFPCalculator<PointSource>::lfp(const std::vector<double>& membrane_current);

}  // namespace coreneuron
