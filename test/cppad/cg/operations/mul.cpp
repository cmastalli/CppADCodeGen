/* --------------------------------------------------------------------------
 *  CppADCodeGen: C++ Algorithmic Differentiation with Source Code Generation:
 *    Copyright (C) 2012 Ciengis
 *
 *  CppADCodeGen is distributed under multiple licenses:
 *
 *   - Eclipse Public License Version 1.0 (EPL1), and
 *   - GNU General Public License Version 3 (GPL3).
 *
 *  EPL1 terms and conditions can be found in the file "epl-v10.txt", while
 *  terms and conditions for the GPL3 can be found in the file "gpl3.txt".
 * ----------------------------------------------------------------------------
 * Author: Joao Leal
 */
#include "CppADCGOperationTest.hpp"
#include "mul.hpp"

using namespace CppAD;
using namespace CppAD::cg;

TEST_F(CppADCGOperationTest, MulTestOne) {
    std::vector<double> u{3, 2}; // independent variable vector

    test0nJac("MulTestOne", &MulTestOneFunc<double >, &MulTestOneFunc<CG<double> >, u);
}

TEST_F(CppADCGOperationTest, MulTestTwo) {
    std::vector<double> u{.5}; // independent variable vector

    test0nJac("MulTestTwo", &MulTestTwoFunc<double >, &MulTestTwoFunc<CG<double> >, u);
}
