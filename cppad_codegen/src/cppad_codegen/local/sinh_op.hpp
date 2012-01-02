#ifndef CPPAD_CODEGEN_SINH_OP_INCLUDED
#define CPPAD_CODEGEN_SINH_OP_INCLUDED

/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2011 Ciengis

CppAD is distributed under multiple licenses. This distribution is under
the terms of the 
                    Common Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */


CPPAD_BEGIN_NAMESPACE
/*!
\file sinh_op.hpp
Forward and reverse mode calculations for z = sinh(x).
 */


/*!
Compute forward mode Taylor coefficient for result of op = SinhOp.

The C++ source code corresponding to this operation is
\verbatim
        z = sinh(x)
\endverbatim
The auxillary result is
\verbatim
        y = cosh(x)
\endverbatim
The value of y, and its derivatives, are computed along with the value
and derivatives of z.

\copydetails forward_unary2_op
 */
template <class Base>
inline void forward_code_gen_sinh_op(
        std::ostream& s_out,
        CodeGenNameProvider<Base>& n,
        size_t d,
        size_t i_z,
        size_t i_x) {
    // check assumptions
    CPPAD_ASSERT_UNKNOWN(NumArg(SinhOp) == 1);
    CPPAD_ASSERT_UNKNOWN(NumRes(SinhOp) == 2);
    CPPAD_ASSERT_UNKNOWN(i_x + 1 < i_z);

    // Taylor coefficients corresponding to argument and result
    size_t i_y = i_z - 1;

    std::string sx_d = n.generateVarName(d, i_x);
    std::string sy_d = n.generateVarName(d, i_y);
    std::string sz_d = n.generateVarName(d, i_z);

    // rest of this routine is identical for the following cases:
    // forward_sin_op, forward_cos_op, forward_sinh_op, forward_cosh_op.
    if (d == 0) {
        s_out << sz_d << " = sinh(" << sx_d << ")" << n.endl();
        s_out << sy_d << " = cosh(" << sx_d << ")" << n.endl();
    } else {
        s_out << sz_d << " = " << n.zero() << n.endl();
        s_out << sy_d << " = " << n.zero() << n.endl();
        for (size_t k = 1; k <= d; k++) {
            std::string sx_k = n.generateVarName(k, i_x);
            std::string sy_dk = n.generateVarName(d - k, i_y);
            std::string sz_dk = n.generateVarName(d - k, i_z);

            s_out << sz_d << " += " << k << " * " << sx_k << " * " << sy_dk << n.endl();
            s_out << sy_d << " += " << k << " * " << sx_k << " * " << sz_dk << n.endl();
        }
        s_out << sz_d << " /= " << n.CastToBase(d) << n.endl();
        s_out << sy_d << " /= " << n.CastToBase(d) << n.endl();
    }
}

/*!
Compute zero order forward mode Taylor coefficient for result of op = SinhOp.

The C++ source code corresponding to this operation is
\verbatim
        z = sinh(x)
\endverbatim
The auxillary result is
\verbatim
        y = cosh(x)
\endverbatim
The value of y is computed along with the value of z.

\copydetails forward_unary2_op_0
 */
template <class Base>
inline void forward_code_gen_sinh_op_0(
        std::ostream& s_out,
        CodeGenNameProvider<Base>& n,
        size_t i_z,
        size_t i_x) {
    // check assumptions
    CPPAD_ASSERT_UNKNOWN(NumArg(SinhOp) == 1);
    CPPAD_ASSERT_UNKNOWN(NumRes(SinhOp) == 2);
    CPPAD_ASSERT_UNKNOWN(i_x + 1 < i_z);

    // Taylor coefficients corresponding to argument and result
    size_t i_y = i_z - 1;

    std::string sx_0 = n.generateVarName(0, i_x);
    std::string sy_0 = n.generateVarName(0, i_y);
    std::string sz_0 = n.generateVarName(0, i_z);

    s_out << sz_0 << " = sinh(" << sx_0 << ")" << n.endl();
    s_out << sy_0 << " = cosh(" << sx_0 << ")" << n.endl();
}

/*!
Compute reverse mode partial derivatives for result of op = SinhOp.

The C++ source code corresponding to this operation is
\verbatim
        z = sinh(x)
\endverbatim
The auxillary result is
\verbatim
        y = cosh(x)
\endverbatim
The value of y is computed along with the value of z.

\copydetails reverse_unary2_op
 */

template <class Base>
inline void reverse_code_gen_sinh_op(
size_t d,
size_t i_z,
size_t i_x,
size_t nc_taylor,
const Base* taylor,
size_t nc_partial,
Base* partial) {
    //    // check assumptions
    //    CPPAD_ASSERT_UNKNOWN(NumArg(SinhOp) == 1);
    //    CPPAD_ASSERT_UNKNOWN(NumRes(SinhOp) == 2);
    //    CPPAD_ASSERT_UNKNOWN(i_x + 1 < i_z);
    //    CPPAD_ASSERT_UNKNOWN(d < nc_taylor);
    //    CPPAD_ASSERT_UNKNOWN(d < nc_partial);
    //
    //    // Taylor coefficients and partials corresponding to argument
    //    const Base* x = taylor + i_x * nc_taylor;
    //    Base* px = partial + i_x * nc_partial;
    //
    //    // Taylor coefficients and partials corresponding to first result
    //    const Base* s = taylor + i_z * nc_taylor; // called z in doc
    //    Base* ps = partial + i_z * nc_partial;
    //
    //    // Taylor coefficients and partials corresponding to auxillary result
    //    const Base* c = s - nc_taylor; // called y in documentation
    //    Base* pc = ps - nc_partial;
    //
    //    // rest of this routine is identical for the following cases:
    //    // reverse_sin_op, reverse_cos_op, reverse_sinh_op, reverse_cosh_op.
    //    size_t j = d;
    //    size_t k;
    //    while (j) {
    //        ps[j] /= Base(j);
    //        pc[j] /= Base(j);
    //        for (k = 1; k <= j; k++) {
    //            px[k] += ps[j] * Base(k) * c[j - k];
    //            px[k] += pc[j] * Base(k) * s[j - k];
    //
    //            ps[j - k] += pc[j] * Base(k) * x[k];
    //            pc[j - k] += ps[j] * Base(k) * x[k];
    //
    //        }
    //        --j;
    //    }
    //    px[0] += ps[0] * c[0];
    //    px[0] += pc[0] * s[0];

    throw "not implemented yet";
}

CPPAD_END_NAMESPACE
#endif