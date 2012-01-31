#ifndef CPPAD_CG_CODE_HANDLER_INCLUDED
#define	CPPAD_CG_CODE_HANDLER_INCLUDED
/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2012 Ciengis

CppAD is distributed under multiple licenses. This distribution is under
the terms of the
                    Common Public License Version 1.0.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */

#include <assert.h>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <limits>

#include <cppad/cppad.hpp>

namespace CppAD {

    template<class Base>
    class CG;

    template<class Base>
    class CodeHandler {
    protected:
        std::ostream* _out;
        size_t _idCount;
    public:

        CodeHandler(std::ostream& out) {
            _out = &out;
            _idCount = 0;
        }

        std::ostream* getOutputStream() const {
            return _out;
        }

        void makeVariables(std::vector<CG<Base> >& variables) {
            for (typename std::vector<CG<Base> >::iterator it = variables.begin(); it != variables.end(); ++it) {
                it->makeVariable(*this, createID());
            }
        }

        virtual size_t createID() {
            return ++_idCount;
        }

        virtual size_t getMaximumVariableID() const {
            return _idCount;
        }

        virtual std::string createVariableName(const CG<Base>& val) const {
            return createVariableName(val.getVariableID());
        }

        virtual std::string createVariableName(size_t id) const {
            return std::string("var") + toString(id);
        }

        virtual std::string operations(const CG<Base>& val) const {
            assert(val.getCodeHandler() == NULL || val.getCodeHandler() == this);

            return val.isParameter() ? this->baseToString(val.getParameterValue()) : val.operations();
        }

        virtual void printOperationAssign(const std::string& var, const std::string& operations) const {
            (*this->_out) << var << " = " << operations << ";\n";
        }

        virtual void printOperationPlusAssign(const std::string& var, const std::string& operations) const {
            (*this->_out) << var << " += " << operations << ";\n";
        }

        virtual void printOperationMinusAssign(const std::string& var, const std::string& operations) const {
            (*this->_out) << var << " -= " << operations << ";\n";
        }

        virtual void printOperationMultAssign(const std::string& var, const std::string& operations) const {
            (*this->_out) << var << " *= " << operations << ";\n";
        }

        virtual void printOperationDivAssign(const std::string& var, const std::string& operations) const {
            (*this->_out) << var << " /= " << operations << ";\n";
        }

        virtual std::string baseToString(const Base& value) const {
            std::stringstream str;
            // make sure all digits of floating point values are printed
            str << std::scientific << std::setprecision(std::numeric_limits< Base >::digits10 + 2) << value;
            std::string result;
            str >> result;

            // take out the extra zeros
            size_t end = result.rfind('e') - 1;
            size_t start = end;
            while (result[start] == '0') {
                start--;
            }

            return result.substr(0, start + 1) + result.substr(end + 1);
        }

        virtual void printComparison(std::string leftOps, enum CompareOp op, std::string rightOps) {
            (*_out) << leftOps << " ";
            switch (op) {
                case CompareLt:
                    (*_out) << "<";
                    break;

                case CompareLe:
                    (*_out) << "<=";
                    break;

                case CompareEq:
                    (*_out) << "==";
                    break;

                case CompareGe:
                    (*_out) << ">=";
                    break;

                case CompareGt:
                    (*_out) << ">";
                    break;

                case CompareNe:
                    (*_out) << "!=";
                    break;

                default:
                    CPPAD_ASSERT_UNKNOWN(0);
            }

            (*_out) << " " << rightOps;
        }

        inline std::string toString(size_t v) const {
            std::stringstream str;
            str << v;
            std::string result;
            str >> result;
            return result;
        }

    private:

        CodeHandler() {
            throw 1;
        }

    };

}
#endif
