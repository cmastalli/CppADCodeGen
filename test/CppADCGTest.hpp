/* --------------------------------------------------------------------------
 *  CppADCodeGen: C++ Algorithmic Differentiation with Source Code Generation:
 *    Copyright (C) 2012 Ciengis
 *
 *  CppADCodeGen is distributed under multiple licenses:
 *
 *   - Common Public License Version 1.0 (CPL1), and
 *   - GNU General Public License Version 2 (GPL2).
 *
 * CPL1 terms and conditions can be found in the file "epl-v10.txt", while
 * terms and conditions for the GPL2 can be found in the file "gpl2.txt".
 * ----------------------------------------------------------------------------
 * Author: Joao Leal
 */
#include <vector>

#include <gtest/gtest.h>

#include <cppadcg/cg.hpp>
#include <cppad/memory_leak.hpp>
#include <cppad/near_equal.hpp>

#ifndef CPPADCGOO_CPPADCGTEST_HPP
#define	CPPADCGOO_CPPADCGTEST_HPP

namespace CppAD {

    class CppADCGTest : public ::testing::Test {
    protected:
        typedef CppAD::CG<double> CGD;
        typedef CppAD::AD<CGD> ADCGD;
        bool verbose_;
        bool printValues_;
        bool memory_check_;
    public:

        inline CppADCGTest(bool verbose = false, bool printValues = false) :
            verbose_(verbose),
            printValues_(printValues),
            memory_check_(true) {
        }

        virtual void TearDown() {
            if (memory_check_) {
                ASSERT_FALSE(CppAD::memory_leak());
            }
        }

    protected:

        template<class T>
        inline void compareValues(const std::vector<T>& depCGen,
                                  const std::vector<T>& dep,
                                  T epsilonR = 1e-14, T epsilonA = 1e-14) {

            compareValues(depCGen, &dep[0], "depCGEN", "depTape", epsilonR, epsilonA);
        }

        template<class T>
        inline void compareValues(const std::vector<std::vector<T> >& depCGen,
                                  const std::vector<std::vector<T> >& dep,
                                  T epsilonR = 1e-14, T epsilonA = 1e-14) {

            assert(depCGen.size() == dep.size());

            for (size_t i = 0; i < depCGen.size(); i++) {
                compareValues(depCGen[i], &dep[i][0], "depCGEN", "depTape", epsilonR, epsilonA);
                //std::cerr << testType << " failed (dataset: " << i << ")" << endl;
            }
        }

        template<class VectorT, class T>
        inline void compareValues(const VectorT& cgen, const VectorT& orig,
                                  const std::string& nameCgen, const std::string& nameOrig,
                                  T epsilonR = 1e-14, T epsilonA = 1e-14) {
            compareValues(cgen, &orig[0], nameCgen, nameOrig, epsilonR, epsilonA);
        }

        template<class VectorT, class T>
        inline void compareValues(const VectorT& cgen, const T* orig,
                                  const std::string& nameCgen, const std::string& nameOrig,
                                  T epsilonR = 1e-14, T epsilonA = 1e-14) {
            for (size_t i = 0; i < cgen.size(); i++) {
                nearEqual(cgen[i], orig[i], epsilonR, epsilonA);
                //std::cerr << nameCgen << "[" << i << "] = " << std::setprecision(8) << cgen[i] <<
                //        " != "
                //        << nameOrig << "[" << i << "] = " << std::setprecision(8) << orig[i] << std::endl;
            }
        }

        template<class VectorBool>
        inline void compareBoolValues(const VectorBool& expected, const VectorBool& value) {
            ASSERT_EQ(expected.size(), value.size());
            for (size_t i = 0; i < expected.size(); i++) {
                ASSERT_EQ(expected[i], value[i]);
            }
        }

        template <class T>
        inline void nearEqual(const T &x, const T &y,
                              const T &r = std::numeric_limits<T>::epsilon() * 10,
                              const T &a = std::numeric_limits<T>::epsilon() * 10) {
            //bool ne = CppAD::NearEqual(x, y, r, a);

            T zero(0);
            ASSERT_GT(r, zero);
            ASSERT_GT(a, zero);

            // check for special cases
            T infinity = T(1) / zero;

            // NaN case
            ASSERT_TRUE(x == x);
            ASSERT_TRUE(y == y);

            // infinite cases
            ASSERT_FALSE(x == infinity || x == -infinity);
            ASSERT_FALSE(y == infinity || y == -infinity);

            T ax = x;
            if (ax < zero)
                ax = -ax;

            T ay = y;
            if (ay < zero)
                ay = -ay;

            T ad = x - y;
            if (ad < zero)
                ad = -ad;

            ASSERT_LE(ad, a);

            if (ad >= a)
                ASSERT_LE(ad, r * (ax + ay));
        }
    };

}

#endif