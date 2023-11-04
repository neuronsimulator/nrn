#include <ocmatrix.h>
#include <vector>
#include <iostream>
#include "ivocvect.h"

#include <catch2/catch.hpp>
using namespace Catch::literals;

template <typename T = double>
class ApproxOrOpposite: public Catch::MatcherBase<std::vector<T>> {
    std::vector<T> vec;

  public:
    ApproxOrOpposite(std::vector<T> vec)
        : vec(vec) {}

    bool match(std::vector<T> const& in) const override {
        if (in.size() != vec.size()) {
            return false;
        }
        bool matched = true;
        for (int i = 0; i < in.size(); ++i) {
            if (in[i] != approx(vec[i])) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return true;
        }
        matched = true;
        for (int i = 0; i < in.size(); ++i) {
            if (in[i] != approx(-1 * vec[i])) {
                matched = false;
                break;
            }
        }

        return matched;
    }

    std::string describe() const override {
        std::ostringstream ss;
        ss << "is not approx or opposite approx of " << Catch::Detail::stringify(vec);
        return ss.str();
    }
    template <typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    ApproxOrOpposite& epsilon(T const& newEpsilon) {
        approx.epsilon(newEpsilon);
        return *this;
    }
    template <typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    ApproxOrOpposite& margin(T const& newMargin) {
        approx.margin(newMargin);
        return *this;
    }
    template <typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    ApproxOrOpposite& scale(T const& newScale) {
        approx.scale(newScale);
        return *this;
    }

    mutable Catch::Detail::Approx approx = Catch::Detail::Approx::custom();
};

bool compareMatrix(OcMatrix& m, const std::vector<std::vector<double>>& ref) {
    REQUIRE(m.nrow() == ref.size());
    for (int i = 0; i < m.nrow(); ++i) {
        REQUIRE(m.ncol() == ref[i].size());
        for (int j = 0; j < m.ncol(); ++j) {
            REQUIRE(m.getval(i, j) == Catch::Detail::Approx(ref[i][j]).margin(1e-10));
        }
    }
    return true;
}

SCENARIO("A Matrix", "[neuron_ivoc][OcMatrix]") {
    GIVEN("A 3x3 Full matrix") {
        OcFullMatrix m{3, 3};
        REQUIRE(m.nrow() == 3);
        REQUIRE(m.ncol() == 3);
        {
            m.ident();
            REQUIRE(compareMatrix(m, {{1., 0., 0.}, {0., 1., 0}, {0., 0., 1.}}));
        }
        {
            double* value = m.mep(0, 0);
            REQUIRE(*value == 1);
            *value = 3;
            REQUIRE(m.getval(0, 0) == 3);
        }
        m.resize(4, 3);
        {
            m.setrow(3, 2.0);
            REQUIRE(compareMatrix(m, {{3., 0., 0.}, {0., 1., 0.}, {0., 0., 1.}, {2., 2., 2.}}));
        }
        {
            std::vector<int> x, y;
            m.nonzeros(x, y);
            std::vector<int> res_x = {0, 1, 2, 3, 3, 3};
            std::vector<int> res_y = {0, 1, 2, 0, 1, 2};
            REQUIRE(x == res_x);
            REQUIRE(y == res_y);
        }
        {
            m.setcol(1, 4.0);
            REQUIRE(compareMatrix(m, {{3., 4., 0.}, {0., 4., 0.}, {0., 4., 1.}, {2., 4., 2.}}));
        }
        {
            m.setdiag(0, 5.0);
            REQUIRE(compareMatrix(m, {{5., 4., 0.}, {0., 5., 0.}, {0., 4., 5.}, {2., 4., 2.}}));
        }
        {
            m.setdiag(1, 6.0);
            REQUIRE(compareMatrix(m, {{5., 6., 0.}, {0., 5., 6.}, {0., 4., 5.}, {2., 4., 2.}}));
        }
        {
            m.setdiag(-1, 7.0);
            REQUIRE(compareMatrix(m, {{5., 6., 0.}, {7., 5., 6.}, {0., 7., 5.}, {2., 4., 7.}}));
        }

        {
            OcFullMatrix n(4, 3);
            n.ident();
            m.add(&n, &m);
            REQUIRE(compareMatrix(m, {{6., 6., 0.}, {7., 6., 6.}, {0., 7., 6.}, {2., 4., 7.}}));
        }
        {
            OcFullMatrix n(4, 3);
            m.bcopy(&n, 1, 1, 3, 2, 0, 0);
            REQUIRE(compareMatrix(n, {{6., 6., 0.}, {7., 6., 0.}, {4., 7., 0.}, {0., 0., 0.}}));
        }
        {
            OcFullMatrix n(4, 3);
            m.transpose(&n);
            REQUIRE(compareMatrix(n, {{6., 7., 0., 2.}, {6., 6., 7., 4.}, {0., 6., 6., 7.}}));
        }
        {
            IvocVect v(3);
            m.getrow(2, &v);
            REQUIRE_THAT(v.vec(), Catch::Matchers::Approx(std::vector<double>({0., 7., 6.})));
            m.setrow(0, &v);
            REQUIRE(compareMatrix(m, {{0., 7., 6.}, {7., 6., 6.}, {0., 7., 6.}, {2., 4., 7.}}));
        }
        {
            IvocVect v(4);
            m.getcol(2, &v);
            REQUIRE_THAT(v.vec(), Catch::Matchers::Approx(std::vector<double>({6., 6., 6., 7.})));
            m.setcol(1, &v);
            REQUIRE(compareMatrix(m, {{0., 6., 6.}, {7., 6., 6.}, {0., 6., 6.}, {2., 7., 7.}}));
        }
        {
            m.resize(3, 3);
            REQUIRE(compareMatrix(m, {{0., 6., 6.}, {7., 6., 6.}, {0., 6., 6.}}));
        }
        {
            OcFullMatrix n(4, 3);
            m.exp(&n);
            REQUIRE(n(0, 0) == Catch::Detail::Approx(442925.));
            REQUIRE(compareMatrix(n,
                                  {{442925., 938481., 938481.},
                                   {651970., 1381407., 1381407.},
                                   {442926., 938481., 938482.}}));
        }
        {
            m.pow(2, &m);
            REQUIRE(compareMatrix(m, {{42., 72., 72.}, {42., 114., 114.}, {42., 72., 72.}}));
        }
        {
            int e{};
            double det = m.det(&e);
            REQUIRE(det == 0.);
            REQUIRE(e == 0);
        }
        *m.mep(2, 0) = 1;
        *m.mep(2, 2) = 2;
        {
            int e{};
            double det = m.det(&e);
            REQUIRE(det == -1.2348_a);
            REQUIRE(e == 5);
        }
        {
            OcFullMatrix n(4, 3);
            m.inverse(&n);
            n.resize(3, 3);  // ???
            REQUIRE(compareMatrix(n,
                                  {{0.064625850, -0.040816326, 0.},
                                   {-0.00024295432, -0.0000971817, 0.01428571},
                                   {-0.023566569, 0.0239067055, -0.014285714}}));
            n.zero();
            REQUIRE(compareMatrix(n, {{0., 0., 0.}, {0., 0., 0.}, {0., 0., 0.}}));
        }
        {
            IvocVect v(3);
            m.getdiag(1, &v);
            REQUIRE_THAT(v.vec(), Catch::Matchers::Approx(std::vector<double>({72., 114., 0.})));
            v.vec() = {0., 72., 114.};
            m.setdiag(-1, &v);
            REQUIRE(compareMatrix(m, {{42., 72., 72.}, {72., 114., 114.}, {1., 114., 2.}}));
        }
        {
            IvocVect v(3);
            m.getdiag(-2, &v);
            REQUIRE(v.vec()[2] == Catch::Detail::Approx(1.0));
            v.vec() = {1., 0., 0.};
            m.setdiag(2, &v);
            REQUIRE(compareMatrix(m, {{42., 72., 1.}, {72., 114., 114.}, {1., 114., 2.}}));
        }
        {
            IvocVect v(3);
            v.vec() = {1, 1, 1};
            IvocVect vout(3);
            m.mulv(&v, &vout);
            REQUIRE_THAT(vout.vec(),
                         Catch::Matchers::Approx(std::vector<double>({115., 300., 117.})));
        }
        {
            OcFullMatrix n(3, 3);
            m.copy(&n);
            REQUIRE(compareMatrix(n, {{42., 72., 1.}, {72., 114., 114.}, {1., 114., 2.}}));
            OcFullMatrix o(3, 3);
            m.mulm(&n, &o);
            REQUIRE(compareMatrix(
                o, {{6949., 11346., 8252.}, {11346., 31176., 13296.}, {8252., 13296., 13001.}}));
        }
        {
            OcFullMatrix n(3, 3);
            m.muls(2, &n);
            REQUIRE(compareMatrix(n, {{84., 144., 2.}, {144., 228., 228.}, {2., 228., 4.}}));
        }
        {
            IvocVect v(3);
            v.vec() = {1, 1, 1};
            IvocVect vout(3);
            m.solv(&v, &vout, false);
            REQUIRE_THAT(vout.vec(),
                         Catch::Matchers::Approx(
                             std::vector<double>({0.0088700, 0.0087927, -0.00562299})));
            m.solv(&v, &vout, true);
            REQUIRE_THAT(vout.vec(),
                         Catch::Matchers::Approx(
                             std::vector<double>({0.0088700, 0.0087927, -0.00562299})));
        }
        {
            IvocVect v(3);
            OcFullMatrix n(3, 3);
            v.vec() = {1, 2, 3};
            m.setrow(0, &v);
            v.vec() = {2, 1, 4};
            m.setrow(1, &v);
            v.vec() = {3, 4, 1};
            m.setrow(2, &v);
            m.symmeigen(&n, &v);
            REQUIRE_THAT(v.vec(),
                         Catch::Matchers::Approx(
                             std::vector<double>({7.074673, -0.88679, -3.18788})));
            n.getcol(0, &v);
            REQUIRE_THAT(v.vec(), ApproxOrOpposite({0.50578, 0.5843738, 0.634577}));
            n.getcol(1, &v);
            REQUIRE_THAT(v.vec(), ApproxOrOpposite({-0.8240377, 0.544925, 0.154978}));
            n.getcol(2, &v);
            REQUIRE_THAT(v.vec(), ApproxOrOpposite({-0.255231, -0.601301, 0.7571611}));
        }
        {
            m.resize(2, 2);
            OcFullMatrix u(2, 2);
            OcFullMatrix v(2, 2);
            IvocVect d(2);
            m.svd1(&u, &v, &d);
            REQUIRE_THAT(d.vec(), Catch::Matchers::Approx(std::vector<double>({3., 1.})));
            // For comparison of u and v and problems with signs, see:
            // https://www.educative.io/blog/sign-ambiguity-in-singular-value-decomposition
            IvocVect c(4);
            c.vec() = {u(0, 0), u(0, 1), v(0, 0), v(0, 1)};
            CHECK_THAT(c.vec(), ApproxOrOpposite({0.70710, 0.70710, 0.70710, 0.70710}));
            c.vec() = {u(1, 0), u(1, 1), v(1, 0), v(1, 1)};
            CHECK_THAT(c.vec(), ApproxOrOpposite({0.70710, -0.70710, -0.70710, 0.70710}));
        }
        {
            m.resize(2, 3);
            {
                IvocVect s(3);
                s.vec() = {3., 2., 2.};
                m.setrow(0, &s);
                s.vec() = {2., 3., -2.};
                m.setrow(1, &s);
            }
            OcFullMatrix u(2, 2);
            OcFullMatrix v(3, 3);
            IvocVect d(2);
            m.svd1(&u, &v, &d);
            REQUIRE_THAT(d.vec(), Catch::Matchers::Approx(std::vector<double>({5., 3.})));
            // For comparison of u and v and problems with signs, see:
            // https://www.educative.io/blog/sign-ambiguity-in-singular-value-decomposition
            IvocVect c(5);
            c.vec() = {u(0, 0), u(0, 1), v(0, 0), v(0, 1), v(0, 2)};
            CHECK_THAT(c.vec(),
                       ApproxOrOpposite({0.70710, 0.70710, 0.70710, 0.70710, 0.}).margin(1e-10));
            c.vec() = {u(1, 0), u(1, 1), v(1, 0), v(1, 1), v(1, 2)};
            CHECK_THAT(c.vec(),
                       ApproxOrOpposite({0.70710, -0.70710, 0.235702, -0.235702, 0.942809}));
            c.vec() = {0., 0., v(2, 0), v(2, 1), v(2, 2)};
            CHECK_THAT(c.vec(), ApproxOrOpposite({0., 0., 0.66666, -0.66666, -0.3333333}));
        }
        {  // Try with vectors too short
            IvocVect s(2);
            s.vec() = {1., 2.};
            m.setrow(0, &s);
            REQUIRE(compareMatrix(m, {{1., 2., 2.}, {2., 3., -2.}}));
            m.setcol(0, &s);
            REQUIRE(compareMatrix(m, {{1., 2., 2.}, {2., 3., -2.}}));
            IvocVect d(1);
            d.vec() = {1.};
            m.setdiag(0, &d);
            REQUIRE(compareMatrix(m, {{1., 2., 2.}, {2., 3., -2.}}));
        }
    }
    GIVEN("A 3x3 Sparse matrix") {
        OcSparseMatrix m{3, 3};
        REQUIRE(m.nrow() == 3);
        REQUIRE(m.ncol() == 3);
        {
            m.ident();
            REQUIRE(compareMatrix(m, {{1., 0., 0.}, {0., 1., 0}, {0., 0., 1.}}));
            REQUIRE(m.sprowlen(1) == 1);
        }
        {
            std::vector<int> x, y, result = {0, 1, 2};
            m.nonzeros(x, y);
            REQUIRE(x == result);
            REQUIRE(y == result);
        }
        {
            double* pmep = m.mep(1, 1);
            REQUIRE(*pmep == 1);
            pmep = m.mep(1, 0);
            REQUIRE(*pmep == 0);
        }
        {
            int col{};
            double value = m.spgetrowval(2, 0, &col);
            REQUIRE(col == 2);
            REQUIRE(value == Catch::Detail::Approx(1.0));
        }
        {  // m.zero() don't erase the matrix but only replace existing values by 0.
            m.zero();
            REQUIRE(m.sprowlen(2) == 1);
            REQUIRE(compareMatrix(m, {{0., 0., 0.}, {0., 0., 0}, {0., 0., 0.}}));
        }
        {
            m.setrow(1, 2);
            REQUIRE(compareMatrix(m, {{0., 0., 0.}, {2., 2., 2.}, {0., 0., 0.}}));
        }
        {
            m.setcol(0, 3);
            REQUIRE(compareMatrix(m, {{3., 0., 0.}, {3., 2., 2.}, {3., 0., 0.}}));
        }
        {
            m.setdiag(0, 1);
            REQUIRE(compareMatrix(m, {{1., 0., 0.}, {3., 1., 2.}, {3., 0., 1.}}));
        }
        {
            m.setdiag(-1, 4);
            REQUIRE(compareMatrix(m, {{1., 0., 0.}, {4., 1., 2.}, {3., 4., 1.}}));
        }
        {
            m.setdiag(2, 5);
            REQUIRE(compareMatrix(m, {{1., 0., 5.}, {4., 1., 2.}, {3., 4., 1.}}));
        }
        REQUIRE(m.sprowlen(1) == 3);

        {
            IvocVect v(3);
            v.vec() = {1, 2, 3};
            m.setrow(0, &v);
            REQUIRE(compareMatrix(m, {{1., 2., 3.}, {4., 1., 2.}, {3., 4., 1.}}));
        }
        {
            IvocVect v(3);
            v.vec() = {1, 2, 3};
            m.setcol(0, &v);
            REQUIRE(compareMatrix(m, {{1., 2., 3.}, {2., 1., 2.}, {3., 4., 1.}}));
        }
        {
            IvocVect v(3);
            v.vec() = {1, 2, 3};
            m.setdiag(0, &v);
            REQUIRE(compareMatrix(m, {{1., 2., 3.}, {2., 2., 2.}, {3., 4., 3.}}));
        }
        {
            IvocVect v(3);
            v.vec() = {0., 1., 2.};
            m.setdiag(-1, &v);
            REQUIRE(compareMatrix(m, {{1., 2., 3.}, {1., 2., 2.}, {3., 2., 3.}}));
        }
        {
            IvocVect v(3);
            v.vec() = {1, 2, 3};
            IvocVect out(3);
            m.mulv(&v, &out);
            REQUIRE_THAT(out.vec(), Catch::Matchers::Approx(std::vector<double>({14., 11., 16.})));
        }
        {
            IvocVect v(3);
            v.vec() = {1, 1, 1};
            IvocVect vout(3);
            m.solv(&v, &vout, false);
            REQUIRE_THAT(vout.vec(), Catch::Matchers::Approx(std::vector<double>({0., 0.5, 0.})));
            m.solv(&v, &vout, true);
            REQUIRE_THAT(vout.vec(), Catch::Matchers::Approx(std::vector<double>({0., 0.5, 0.})));
        }
    }
}
