#ifndef CPPAD_CG_C_LANG_COMPILE_MODEL_HELPER_HES_INCLUDED
#define CPPAD_CG_C_LANG_COMPILE_MODEL_HELPER_HES_INCLUDED
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

namespace CppAD {

    template<class Base>
    void CLangCompileModelHelper<Base>::generateHessianSource(std::map<std::string, std::string>& sources) {
        const std::string jobName = "Hessian";

        startingJob("operation graph for '" + jobName + "'");

        CodeHandler<Base> handler;
        handler.setJobTimer(this);

        size_t m = _fun.Range();
        size_t n = _fun.Domain();


        // independent variables
        vector<CGBase> indVars(n);
        handler.makeVariables(indVars);
        if (_x.size() > 0) {
            for (size_t i = 0; i < n; i++) {
                indVars[i].setValue(_x[i]);
            }
        }

        // multipliers
        vector<CGBase> w(m);
        handler.makeVariables(w);
        if (_x.size() > 0) {
            for (size_t i = 0; i < m; i++) {
                w[i].setValue(Base(1.0));
            }
        }

        vector<CGBase> hess = _fun.Hessian(indVars, w);

        // make use of the symmetry of the Hessian in order to reduce operations
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < i; j++) {
                hess[i * n + j] = hess[j * n + i];
            }
        }

        finishedJob();

        CLanguage<Base> langC(_baseTypeName);
        langC.setMaxAssigmentsPerFunction(_maxAssignPerFunc, &sources);
        langC.setGenerateFunction(_name + "_" + FUNCTION_HESSIAN);

        std::ostringstream code;
        std::auto_ptr<VariableNameGenerator<Base> > nameGen(createVariableNameGenerator("hess"));
        CLangDefaultHessianVarNameGenerator<Base> nameGenHess(nameGen.get(), n);

        handler.generateCode(code, langC, hess, nameGenHess, _atomicFunctions, jobName);
    }

    template<class Base>
    void CLangCompileModelHelper<Base>::generateSparseHessianSource(std::map<std::string, std::string>& sources) throw (CGException) {
        /**
         * Determine the sparsity pattern p for Hessian of w^T F
         */
        determineHessianSparsity();

        if (_reverseTwo) {
            generateSparseHessianSourceFromRev2(sources);
        } else {
            generateSparseHessianSourceDirectly(sources);
        }
    }

    template<class Base>
    void CLangCompileModelHelper<Base>::generateSparseHessianSourceDirectly(std::map<std::string, std::string>& sources) throw (CGException) {
        const std::string jobName = "sparse Hessian";
        size_t m = _fun.Range();
        size_t n = _fun.Domain();

        /**
         * we might have to consider a slightly different order than the one
         * specified by the user according to the available elements in the sparsity
         */
        std::vector<size_t> evalRows, evalCols;
        determineSecondOrderElements4Eval(evalRows, evalCols);

        std::map<size_t, std::map<size_t, size_t> > locations;
        for (size_t e = 0; e < evalRows.size(); e++) {
            size_t j1 = evalRows[e];
            size_t j2 = evalCols[e];
            std::map<size_t, std::map<size_t, size_t> >::iterator itJ1 = locations.find(j1);
            if (itJ1 == locations.end()) {
                locations[j1][j2] = e;
            } else {
                std::map<size_t, size_t>& j22e = itJ1->second;
                if (j22e.find(j2) == j22e.end()) {
                    j22e[j2] = e; // OK
                } else {
                    // repeated elements not allowed
                    std::ostringstream ss;
                    ss << "Repeated Hessian element requested: " << j1 << " " << j2;
                    throw CGException(ss.str());
                }
            }
        }

        // make use of the symmetry of the Hessian in order to reduce operations
        std::vector<size_t> lowerHessRows, lowerHessCols, lowerHessOrder;
        lowerHessRows.reserve(_hessSparsity.rows.size() / 2);
        lowerHessCols.reserve(lowerHessRows.size());
        lowerHessOrder.reserve(lowerHessRows.size());

        std::map<size_t, size_t> duplicates; // the elements determined using symmetry
        std::map<size_t, std::map<size_t, size_t> >::const_iterator itJ;
        std::map<size_t, size_t>::const_iterator itI;
        for (size_t e = 0; e < evalRows.size(); e++) {
            bool add = true;
            size_t i = evalRows[e];
            size_t j = evalCols[e];
            if (i < j) {
                // find the symmetric value
                itJ = locations.find(j);
                if (itJ != locations.end()) {
                    itI = itJ->second.find(i);
                    if (itI != itJ->second.end()) {
                        size_t eSim = itI->second;
                        duplicates[e] = eSim;
                        add = false; // symmetric value being determined
                    }
                }
            }

            if (add) {
                lowerHessRows.push_back(i);
                lowerHessCols.push_back(j);
                lowerHessOrder.push_back(e);
            }
        }

        /**
         * 
         */
        startingJob("operation graph for '" + jobName + "'");

        CodeHandler<Base> handler;
        handler.setJobTimer(this);

        // independent variables
        vector<CGBase> indVars(n);
        handler.makeVariables(indVars);
        if (_x.size() > 0) {
            for (size_t i = 0; i < n; i++) {
                indVars[i].setValue(_x[i]);
            }
        }

        // multipliers
        vector<CGBase> w(m);
        handler.makeVariables(w);
        if (_x.size() > 0) {
            for (size_t i = 0; i < m; i++) {
                w[i].setValue(Base(1.0));
            }
        }

        vector<CGBase> hess(_hessSparsity.rows.size());
        if (_loopTapes.empty()) {
            CppAD::sparse_hessian_work work;
            vector<CGBase> lowerHess(lowerHessRows.size());
            _fun.SparseHessian(indVars, w, _hessSparsity.sparsity, lowerHessRows, lowerHessCols, lowerHess, work);

            for (size_t i = 0; i < lowerHessOrder.size(); i++) {
                hess[lowerHessOrder[i]] = lowerHess[i];
            }

            // make use of the symmetry of the Hessian in order to reduce operations
            std::map<size_t, size_t>::const_iterator it2;
            for (it2 = duplicates.begin(); it2 != duplicates.end(); ++it2) {
                hess[it2->first] = hess[it2->second];
            }
        } else {
            /**
             * with loops
             */
            hess = prepareSparseHessianWithLoops(handler, indVars, w,
                                                 lowerHessRows, lowerHessCols, lowerHessOrder,
                                                 duplicates);
        }

        finishedJob();

        CLanguage<Base> langC(_baseTypeName);
        langC.setMaxAssigmentsPerFunction(_maxAssignPerFunc, &sources);
        langC.setGenerateFunction(_name + "_" + FUNCTION_SPARSE_HESSIAN);

        std::ostringstream code;
        std::auto_ptr<VariableNameGenerator<Base> > nameGen(createVariableNameGenerator("hess"));
        CLangDefaultHessianVarNameGenerator<Base> nameGenHess(nameGen.get(), n);

        handler.generateCode(code, langC, hess, nameGenHess, _atomicFunctions, jobName);
    }

    template<class Base>
    void CLangCompileModelHelper<Base>::generateSparseHessianSourceFromRev2(std::map<std::string, std::string>& sources) throw (CGException) {
        using namespace std;

        /**
         * we might have to consider a slightly different order than the one
         * specified by the user according to the available elements in the sparsity
         */
        std::vector<size_t> evalRows, evalCols;
        determineSecondOrderElements4Eval(evalRows, evalCols);

        // elements[var]{var}
        map<size_t, std::vector<size_t> > elements;
        for (size_t e = 0; e < evalRows.size(); e++) {
            elements[evalRows[e]].push_back(evalCols[e]);
        }

        // maps each element to its position in the user hessian
        map<size_t, std::vector<set<size_t> > > userHessElLocation = determineOrderByRow(elements, evalRows, evalCols);

        /**
         * determine to which functions we can provide the hessian row directly
         * without needing a temporary array (compressed)
         */
        map<size_t, bool> ordered;
        map<size_t, std::vector<size_t> >::const_iterator it;
        for (it = elements.begin(); it != elements.end(); ++it) {
            size_t index = it->first;
            const std::vector<size_t>& els = it->second;
            const std::vector<set<size_t> >& location = userHessElLocation.at(index);
            assert(els.size() == location.size());
            assert(els.size() > 0);

            bool passed = true;
            size_t hessRowStart = *location[0].begin();
            for (size_t e = 0; e < els.size(); e++) {
                if (location[e].size() > 1) {
                    passed = false; // too many elements
                    break;
                }
                if (*location[e].begin() != hessRowStart + e) {
                    passed = false; // wrong order
                    break;
                }
            }
            ordered[index] = passed;
        }
        assert(elements.size() == ordered.size());

        /**
         * determine the maximum size of the temporary array
         */
        size_t maxCompressedSize = 0;

        map<size_t, bool>::const_iterator itOrd;
        for (it = elements.begin(), itOrd = ordered.begin(); it != elements.end(); ++it, ++itOrd) {
            if (it->second.size() > maxCompressedSize && !itOrd->second)
                maxCompressedSize = it->second.size();
        }

        if (!_loopTapes.empty()) {
            /**
             * with loops
             */
            generateSparseHessianWithLoopsSourceFromRev2(sources, userHessElLocation, ordered, maxCompressedSize);
            return;
        }

        string model_function = _name + "_" + FUNCTION_SPARSE_HESSIAN;
        string functionRev2 = _name + "_" + FUNCTION_SPARSE_REVERSE_TWO;
        string rev2Suffix = "indep";

        CLanguage<Base> langC(_baseTypeName);
        std::string argsDcl = langC.generateDefaultFunctionArgumentsDcl();

        _cache.str("");
        _cache << "#include <stdlib.h>\n"
                << CLanguage<Base>::ATOMICFUN_STRUCT_DEFINITION << "\n\n";
        generateFunctionDeclarationSource(_cache, functionRev2, rev2Suffix, elements, argsDcl);
        _cache << "\n"
                "void " << model_function << "(" << argsDcl << ") {\n"
                "   " << _baseTypeName << " const * inLocal[3];\n"
                "   " << _baseTypeName << " inLocal1 = 1;\n"
                "   " << _baseTypeName << " * outLocal[1];\n";
        if (maxCompressedSize > 0) {
            _cache << "   " << _baseTypeName << " compressed[" << maxCompressedSize << "];\n";
        }
        _cache << "   " << _baseTypeName << " * hess = out[0];\n"
                "\n"
                "   inLocal[0] = in[0];\n"
                "   inLocal[1] = &inLocal1;\n"
                "   inLocal[2] = in[1];\n";
        if (maxCompressedSize > 0) {
            _cache << "   outLocal[0] = compressed;";
        }

        langC.setArgumentIn("inLocal");
        langC.setArgumentOut("outLocal");
        std::string argsLocal = langC.generateDefaultFunctionArguments();
        bool lastCompressed = true;
        for (it = elements.begin(), itOrd = ordered.begin(); it != elements.end(); ++it, ++itOrd) {
            size_t index = it->first;
            const std::vector<size_t>& els = it->second;
            const std::vector<set<size_t> >& location = userHessElLocation.at(index);
            assert(els.size() == location.size());
            assert(els.size() > 0);

            _cache << "\n";
            if (itOrd->second) {
                _cache << "   outLocal[0] = &hess[" << *location[0].begin() << "];\n";
            } else if (!lastCompressed) {
                _cache << "   outLocal[0] = compressed;\n";
            }
            _cache << "   " << functionRev2 << "_" << rev2Suffix << index << "(" << argsLocal << ");\n";
            if (!itOrd->second) {
                for (size_t e = 0; e < els.size(); e++) {
                    _cache << "   ";
                    set<size_t>::const_iterator itl;
                    for (itl = location[e].begin(); itl != location[e].end(); ++itl) {
                        _cache << "hess[" << (*itl) << "] = ";
                    }
                    _cache << "compressed[" << e << "];\n";
                }
            }
            lastCompressed = !itOrd->second;
        }

        _cache << "\n"
                "}\n";
        sources[model_function + ".c"] = _cache.str();
        _cache.str("");
    }

    template<class Base>
    void CLangCompileModelHelper<Base>::determineSecondOrderElements4Eval(std::vector<size_t>& evalRows,
                                                                          std::vector<size_t>& evalCols) {
        /**
         * Atomic functions migth not have all the elements and thus there may 
         * be no symmetry. This will explore symmetry in order to provide the
         * second order elements requested by the user.
         */
        evalRows.reserve(_hessSparsity.rows.size());
        evalCols.reserve(_hessSparsity.cols.size());

        for (size_t e = 0; e < _hessSparsity.rows.size(); e++) {
            size_t i = _hessSparsity.rows[e];
            size_t j = _hessSparsity.cols[e];
            if (_hessSparsity.sparsity[i].find(j) == _hessSparsity.sparsity[i].end() &&
                    _hessSparsity.sparsity[j].find(i) != _hessSparsity.sparsity[j].end()) {
                // only the symmetric value is available
                // (it can be caused by atomic functions which may only be providing a partial hessian)
                evalRows.push_back(j);
                evalCols.push_back(i);
            } else {
                evalRows.push_back(i);
                evalCols.push_back(j);
            }
        }
    }

    template<class Base>
    void CLangCompileModelHelper<Base>::determineHessianSparsity() {
        if (_hessSparsity.sparsity.size() > 0) {
            return;
        }

        size_t m = _fun.Range();
        size_t n = _fun.Domain();

        /**
         * sparsity for the sum of the hessians of all equations
         */
        SparsitySetType r(n); // identity matrix
        for (size_t j = 0; j < n; j++)
            r[j].insert(j);
        SparsitySetType jac = _fun.ForSparseJac(n, r);

        SparsitySetType s(1);
        for (size_t i = 0; i < m; i++) {
            s[0].insert(i);
        }
        _hessSparsity.sparsity = _fun.RevSparseHes(n, s, false);
        //printSparsityPattern(_hessSparsity.sparsity, "hessian");

        if (_hessianByEquation || _reverseTwo) {
            /**
             * sparsity for the hessian of each equations
             */

            std::set<size_t>::const_iterator it;

            std::set<size_t> customVarsInHess;
            if (_custom_hess.defined) {
                customVarsInHess.insert(_custom_hess.row.begin(), _custom_hess.row.end());
                customVarsInHess.insert(_custom_hess.col.begin(), _custom_hess.col.end());

                r = SparsitySetType(n); //clear r
                for (it = customVarsInHess.begin(); it != customVarsInHess.end(); ++it) {
                    size_t j = *it;
                    r[j].insert(j);
                }
                jac = _fun.ForSparseJac(n, r);
            }

            /**
             * Coloring
             */
            const CppAD::vector<Color> colors = colorByRow(customVarsInHess, jac);

            /**
             * For each individual equation
             */
            _hessSparsities.resize(m);
            for (size_t i = 0; i < m; i++) {
                _hessSparsities[i].sparsity.resize(n);
            }

            for (size_t c = 0; c < colors.size(); c++) {
                const Color& color = colors[c];

                // first-order
                r = SparsitySetType(n); //clear r
                for (it = color.forbiddenRows.begin(); it != color.forbiddenRows.end(); ++it) {
                    size_t j = *it;
                    r[j].insert(j);
                }
                _fun.ForSparseJac(n, r);

                // second-order
                s[0].clear();
                const std::set<size_t>& equations = color.rows;
                std::set<size_t>::const_iterator itEq;
                for (itEq = equations.begin(); itEq != equations.end(); ++itEq) {
                    size_t i = *itEq;
                    s[0].insert(i);
                }

                SparsitySetType sparsityc = _fun.RevSparseHes(n, s, false);

                /**
                 * Retrieve the individual hessians for each equation
                 */
                const std::map<size_t, size_t>& var2Eq = color.column2Row;
                const std::set<size_t>& forbidden_c = color.forbiddenRows; //used variables
                std::set<size_t>::const_iterator itvar;
                for (itvar = forbidden_c.begin(); itvar != forbidden_c.end(); ++itvar) {
                    size_t j = *itvar;
                    if (sparsityc[j].size() > 0) {
                        size_t i = var2Eq.at(j);
                        _hessSparsities[i].sparsity[j].insert(sparsityc[j].begin(),
                                                              sparsityc[j].end());
                    }
                }

            }

            for (size_t i = 0; i < m; i++) {
                LocalSparsityInfo& hessSparsitiesi = _hessSparsities[i];

                if (!_custom_hess.defined) {
                    extra::generateSparsityIndexes(hessSparsitiesi.sparsity,
                                                   hessSparsitiesi.rows, hessSparsitiesi.cols);

                } else {
                    size_t nnz = _custom_hess.row.size();
                    for (size_t e = 0; e < nnz; e++) {
                        size_t i1 = _custom_hess.row[e];
                        size_t i2 = _custom_hess.col[e];
                        if (hessSparsitiesi.sparsity[i1].find(i2) != hessSparsitiesi.sparsity[i1].end()) {
                            hessSparsitiesi.rows.push_back(i1);
                            hessSparsitiesi.cols.push_back(i2);
                        }
                    }
                }
            }

        }

        if (!_custom_hess.defined) {
            extra::generateSparsityIndexes(_hessSparsity.sparsity,
                                           _hessSparsity.rows, _hessSparsity.cols);

        } else {
            _hessSparsity.rows = _custom_hess.row;
            _hessSparsity.cols = _custom_hess.col;
        }
    }

    template<class Base>
    void CLangCompileModelHelper<Base>::generateHessianSparsitySource(std::map<std::string, std::string>& sources) {
        determineHessianSparsity();

        generateSparsity2DSource(_name + "_" + FUNCTION_HESSIAN_SPARSITY, _hessSparsity);
        sources[_name + "_" + FUNCTION_HESSIAN_SPARSITY + ".c"] = _cache.str();
        _cache.str("");

        if (_hessianByEquation || _reverseTwo) {
            generateSparsity2DSource2(_name + "_" + FUNCTION_HESSIAN_SPARSITY2, _hessSparsities);
            sources[_name + "_" + FUNCTION_HESSIAN_SPARSITY2 + ".c"] = _cache.str();
            _cache.str("");
        }
    }

}

#endif