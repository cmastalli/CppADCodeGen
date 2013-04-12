#ifndef CPPAD_CG_DAE_INDEX_REDUCTION_INCLUDED
#define CPPAD_CG_DAE_INDEX_REDUCTION_INCLUDED
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

#include <cppadcg/cg.hpp>
#include <cppadcg/dae_index_reduction/cg_dae_var_info.hpp>

namespace CppAD {

    /**
     * Base class for algorithms that perform automatic (differentiation) index
     * reduction of implicit DAEs.
     */
    template<class Base>
    class DaeIndexReduction {
    protected:
        /**
         * The original model
         */
        ADFun<CG<Base> >* const fun_;
        // DAE variable information
        std::vector<DaeVarInfo> varInfo_;
    public:

        /**
         * Creates a new DAE model index reduction algorithm.
         * 
         * @param fun  The original (high index) model
         * @param varInfo  DAE  system variable information (in the same order 
         *                 as in the tape)
         */
        DaeIndexReduction(ADFun<CG<Base> >* fun,
                          const std::vector<DaeVarInfo>& varInfo) :
            fun_(fun),
            varInfo_(varInfo) {
            assert(fun_ != NULL);
            assert(varInfo_.size() == fun->Domain());
            for (size_t j = 0; j < varInfo_.size(); ++j) {
                varInfo_[j].setOriginalIndex(j);
                varInfo_[j].setId(j);
            }

            for (size_t j = 0; j < varInfo_.size(); ++j) {
                int deriv = varInfo_[j].getAntiDerivative();
                assert(deriv < int(varInfo_.size()));
                if (deriv >= 0) {
                    varInfo_[deriv].setDerivative(j);
                }
            }

            for (size_t j = 0; j < varInfo_.size(); ++j) {
                determineVariableOrder(varInfo_[j]);
            }
        }

        inline virtual ~DaeIndexReduction() {
        }

    private:

        inline void determineVariableOrder(DaeVarInfo& var) {
            if (var.getAntiDerivative() >= 0) {
                DaeVarInfo& antiD = varInfo_[var.getAntiDerivative()];
                if (antiD.getOriginalAntiDerivative() < 0) {
                    determineVariableOrder(antiD);
                }
                var.setOrder(antiD.getOrder() + 1);
                var.setOriginalAntiDerivative(var.getOrder() == 1 ? antiD.getOriginalIndex() : antiD.getOriginalAntiDerivative());
            }
        }
    };
}

#endif	

