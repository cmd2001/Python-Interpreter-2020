#ifndef PYTHON_INTERPRETER_EVALVISITOR_H
#define PYTHON_INTERPRETER_EVALVISITOR_H


#include "Python3BaseVisitor.h"

#include <iostream>
#define debug cerr
using namespace std;

#include "dataTypes.hpp"

VariableStack vs;

FlowStack flowStk, functionStk;

map<string, Python3Parser::FuncdefContext*> functions;
map<string, vector<pair<string, DataType> > > defaultArgs;

class EvalVisitor: public Python3BaseVisitor {
    virtual antlrcpp::Any visitFile_input(Python3Parser::File_inputContext *ctx) override {
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitFuncdef(Python3Parser::FuncdefContext *ctx) override {
        functions[ctx->NAME()->getText()] = ctx;
        if(ctx->parameters()->typedargslist()) {
            vector<pair<string, DataType> > args;
            const auto def_Argument_List =  ctx->parameters()->typedargslist();
            const auto tests = def_Argument_List->test();
            for (int i = 0, siz = tests.size(), siz2 = def_Argument_List->tfpdef().size(); i < siz; i++) {
                args.push_back(make_pair(def_Argument_List->tfpdef(siz2 - i - 1)->NAME()->getText(), visitTest(tests[tests.size() - i - 1]).as<DataType>()));
            }

            defaultArgs[ctx->NAME()->getText()] = args;
        }
        return DataType(None);
    }


    virtual antlrcpp::Any visitStmt(Python3Parser::StmtContext *ctx) override { // suite only includes stmt or simple_stmt.
        if(ctx->simple_stmt()) return visitSimple_stmt(ctx->simple_stmt());
        if(ctx->compound_stmt()) return visitCompound_stmt(ctx->compound_stmt());
    }

    virtual antlrcpp::Any visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) override {
        return visitSmall_stmt(ctx->small_stmt());
    }

    virtual antlrcpp::Any visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) override {
        if(ctx->expr_stmt()) return visitExpr_stmt(ctx->expr_stmt());
        if(ctx->flow_stmt()) return visitFlow_stmt(ctx->flow_stmt());
    }

    virtual antlrcpp::Any visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) override {
        if(ctx->augassign()) { // a ?= b
            const auto op = ctx->augassign()->getText();
            const auto hr = visitTest(ctx->testlist(1)->test(0)).as<DataType>();
            const auto name = ctx->testlist(0)->test(0)->getText();
            if(op == "+=") vs[name].getContent() += hr;
            else if(op == "-=") vs[name].getContent() -= hr;
            else if(op == "*=") vs[name].getContent() *= hr;
            else if(op == "/=") vs[name].getContent() /= hr;
            else if(op == "//=") dualDivEqual(vs[name].getContent(), hr);
            else if(op == "%=") vs[name].getContent() %= hr;
        } else {
            auto testLists = ctx->testlist();
            reverse(testLists.begin(), testLists.end());
            const auto ret = visitTestlist(testLists[0]);
            if(ret.is<DataType>()) {
                vector<DataType> nums;
                nums.push_back(ret.as<DataType>());
                for(unsigned i = 1; i < testLists.size(); i++) {
                    const auto tests = testLists[i]->test();
                    for(unsigned j = 0; j < tests.size(); j++) vs[tests[j]->getText()] = nums[j];
                }
            } else {
                const auto nums = ret.as<vector<DataType> >();
                for(unsigned i = 1; i < testLists.size(); i++) {
                    const auto tests = testLists[i]->test();
                    for(unsigned j = 0; j < tests.size(); j++) vs[tests[j]->getText()] = nums[j];
                }
            }
        }
        return DataType(None);
    }


    virtual antlrcpp::Any visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) override {
        if(ctx->break_stmt()) return visitBreak_stmt(ctx->break_stmt());
        if(ctx->continue_stmt()) return visitContinue_stmt(ctx->continue_stmt());
        if(ctx->return_stmt()) return visitReturn_stmt(ctx->return_stmt());
    }

    virtual antlrcpp::Any visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) override {
        flowStk.set(Broken);
        return DataType(None);
    }

    virtual antlrcpp::Any visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) override {
        flowStk.set(Continued);
        return DataType(None);
    }

    virtual antlrcpp::Any visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) override {
        functionStk.set(Returned);
        if(ctx->testlist()) return visitTestlist(ctx->testlist());
        return DataType(None);
    }

    virtual antlrcpp::Any visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) override {
        if(ctx->if_stmt()) return visitIf_stmt(ctx->if_stmt());
        if(ctx->while_stmt()) return visitWhile_stmt(ctx->while_stmt());
        if(ctx->funcdef()) return visitFuncdef(ctx->funcdef());
    }

    virtual antlrcpp::Any visitIf_stmt(Python3Parser::If_stmtContext *ctx) override {
        auto ls_test = ctx->test();
        auto ls_suite = ctx->suite();
        for(unsigned i = 0; i < ls_test.size(); i++) {
            if(visitTest(ls_test[i]).as<DataType>().toBool().data_Bool)
                    return visitSuite(ls_suite[i]);
        }
        if(ls_suite.size() > ls_test.size()) return visitSuite(*ls_suite.rbegin());
        return DataType(None);
    }

    virtual antlrcpp::Any visitWhile_stmt(Python3Parser::While_stmtContext *ctx) override {
        auto test = ctx->test();
        flowStk.push(Running);
        while(visitTest(test).as<DataType>().toBool().data_Bool) {
            auto suite = ctx->suite();
            auto ret = visitSuite(suite);
            if(functionStk.query() == Returned) {
                flowStk.pop();
                return ret;
            }
            if(flowStk.query() == Broken) break;
            flowStk.reset();
        }
        flowStk.pop();
        return DataType(None);
    }

    virtual antlrcpp::Any visitSuite(Python3Parser::SuiteContext *ctx) override {
        if(ctx->simple_stmt()) return visitSimple_stmt(ctx->simple_stmt());
        const auto ls = ctx->stmt();
        for(auto i: ls) {
            if(flowStk.query() != Running) break;
            auto ret = visitStmt(i);
            if(functionStk.query() == Returned) return ret;
        }
        return DataType(None);
    }

    virtual antlrcpp::Any visitTest(Python3Parser::TestContext *ctx) override {
        return visitOr_test(ctx->or_test());
    }

    virtual antlrcpp::Any visitOr_test(Python3Parser::Or_testContext *ctx) override {
        const auto ls = ctx->and_test();
        if((signed)ls.size() == 1) return visitAnd_test(ls[0]);
        for(auto i: ls) {
            DataType rt = visitAnd_test(i).as<DataType>();
            if(rt.toBool().data_Bool) return DataType((bool) 1);
        }
        return DataType((bool) 0);
    }

    virtual antlrcpp::Any visitAnd_test(Python3Parser::And_testContext *ctx) override {
        const auto ls = ctx->not_test();
        if((signed) ls.size() == 1) return visitNot_test(ls[0]);
        for(auto i: ls) {
            DataType rt = visitNot_test(i).as<DataType>();
            if(!rt.toBool().data_Bool) return DataType((bool) 0);
        }
        return DataType((bool) 1);
    }

    virtual antlrcpp::Any visitNot_test(Python3Parser::Not_testContext *ctx) override {
        if(ctx->comparison()) return visitComparison(ctx->comparison());
        auto test = ctx->not_test();
        DataType ret = visitNot_test(test).as<DataType>();
        return DataType((bool) (ret.toBool().data_Bool ? 0 : 1));
    }

    virtual antlrcpp::Any visitComparison(Python3Parser::ComparisonContext *ctx) override {
        const auto ops = ctx->comp_op();
        if(!ops.size()) return visitArith_expr(ctx->arith_expr(0));
        const auto ariths = ctx->arith_expr();
        DataType hl, hr = visitArith_expr(ariths[0]).as<DataType>();
        for(unsigned i = 0; i < ops.size(); i++) {
            hl = hr, hr = visitArith_expr(ariths[i + 1]).as<DataType>();
            const auto text = ops[i]->getText();
            bool flag = 1;
            if(text == "==") {
                if(hl == hr) flag = 0;
            } else if(text == "!=") {
                if(hl != hr) flag = 0;
            } else if(text == "<") {
                if(hl < hr) flag = 0;
            } else if(text == ">") {
                if(hl > hr) flag = 0;
            } else if(text == "<=") {
                if(hl <= hr) flag = 0;
            } else if(text == ">=") {
                if (hl >= hr) flag = 0;
            }
            if(flag) return DataType((bool) 0);
        }
        return DataType((bool) 1);
    }


    virtual antlrcpp::Any visitArith_expr(Python3Parser::Arith_exprContext *ctx) override {
        const auto list_ops = ctx->addsub_op();
        const auto list_nums = ctx->term();
        if(!list_ops.size()) return visitTerm(list_nums[0]);
        auto ret = visitTerm(list_nums[0]).as<DataType>();
        for(unsigned i = 0; i < list_ops.size(); i++) {
            const auto text = list_ops[i]->getText();
            if(text == "+") ret += visitTerm(list_nums[i + 1]).as<DataType>();
            else if(text =="-") ret -= visitTerm(list_nums[i + 1]).as<DataType>();
        }
        return ret;
    }

    virtual antlrcpp::Any visitTerm(Python3Parser::TermContext *ctx) override {
        const auto list_ops = ctx->muls_op();
        const auto list_nums = ctx->factor();
        if(!list_ops.size()) return visitFactor(list_nums[0]);
        auto ret = visitFactor(list_nums[0]).as<DataType>();
        for(unsigned i = 0; i < list_ops.size(); i++) {
            const auto text = list_ops[i]->getText();
            if(text == "*") ret = ret * visitFactor(list_nums[i + 1]).as<DataType>();
            else if(text == "/") ret = ret / visitFactor(list_nums[i + 1]).as<DataType>();
            else if(text == "//") ret = dualDiv(ret, visitFactor(list_nums[i + 1]).as<DataType>());
            else if(text == "%") ret = ret % visitFactor(list_nums[i + 1]).as<DataType>();
        }
        return ret;
    }

    virtual antlrcpp::Any visitFactor(Python3Parser::FactorContext *ctx) override {
        auto op = ctx->addsub_op();
        if(!op) return visitAtom_expr(ctx->atom_expr());
        if(op->getText() == "+") return visitFactor(ctx->factor());
        return -(visitFactor(ctx->factor()).as<DataType>());
    }

    virtual antlrcpp::Any visitAtom_expr(Python3Parser::Atom_exprContext *ctx) override {
        if(!ctx->trailer()) return visitAtom(ctx->atom());
        const string function_Name = ctx->atom()->getText();
        if(function_Name == "print") {
            if(ctx->trailer()->arglist()) {
                const auto provided_Argument_List = ctx->trailer()->arglist()->argument();
                for(unsigned i = 0; i < provided_Argument_List.size(); i++) {
                    const auto data = visitTest(provided_Argument_List[i]->test()).as<DataType>();
                    cout << data.toPrint();
                    if(i != provided_Argument_List.size() - 1) cout << " ";
                }
            }
            cout << endl;
            return DataType(None);
        } else if(function_Name == "int" || function_Name == "float" || function_Name == "str" || function_Name == "bool") {
            const auto only = visitTest(ctx->trailer()->arglist()->argument(0)->test()).as<DataType>();
            if(function_Name == "int") return only.toInt();
            if(function_Name == "float") return only.toFloat();
            if(function_Name == "str") return only.toString();
            if(function_Name == "bool") return only.toBool();
        } else {
            map<string, Variable> new_Argument_List;
            if(functions[function_Name]->parameters()->typedargslist()) { // push arguments.
                const auto def_Argument_List =  functions[function_Name]->parameters()->typedargslist();
                const auto arg2 = defaultArgs[function_Name];
                for(auto i: arg2) new_Argument_List[i.first] = i.second;
                if(ctx->trailer()->arglist()) {
                    const auto provided_Argument_List = ctx->trailer()->arglist()->argument();
                    int i;
                    for (i = 0; i < provided_Argument_List.size(); i++) {
                        const auto t = provided_Argument_List[i];
                        if (t->NAME()) break;
                        const string name = def_Argument_List->tfpdef(i)->getText();
                        new_Argument_List[name] = visitTest(t->test()).as<DataType>();
                    }
                    for (; i < provided_Argument_List.size(); i++) {
                        const auto t = provided_Argument_List[i];
                        new_Argument_List[t->NAME()->getText()] = visitTest(t->test()).as<DataType>();
                    }
                }
            }
            vs.push(0), vs.merge(new_Argument_List), functionStk.push(Running);
            auto ret = visitSuite(functions[function_Name]->suite());
            vs.pop(), functionStk.pop();
            return ret;
        }
    }


    virtual antlrcpp::Any visitAtom(Python3Parser::AtomContext *ctx) override {
        const string text = ctx->getText();
        if(ctx->NAME()) return vs[text].getContent(); // it must be a variable.
        if(ctx->NUMBER()) {
            if(text.find('.') != text.npos) {
                double a = 0;
                int i = text[0] == '-';
                for(; text[i] != '.'; i++) a = a * 10 + text[i] - '0';
                double mul = 0.1;
                for(int j = i + 1; j < text.length(); j++) a = a + mul * (text[j] - '0'), mul *= 0.1;
                return DataType(text[0] == '-' ? -a : a);
            } else return DataType(BigInt(text));
        }
        auto Strings = ctx->STRING();
        if(Strings.size()) {
            string ret = "";
            for(auto i: Strings) {
                string a = i->getText();
                ret = ret + a.substr(1, a.length() - 2);
            }
            return DataType(ret);
        }
        if(ctx->test()) return visitTest(ctx->test());
        if(text == "True") return DataType((bool) 1);
        if(text == "False") return DataType((bool) 0);
        if(text == "None") return DataType(None);
    }
    virtual antlrcpp::Any visitTestlist(Python3Parser::TestlistContext *ctx) override { // return an vector of DataType
        const auto tests = ctx->test();
        if(tests.size() == 1) return visitTest(tests[0]);
        vector<DataType> ret;
        for(auto i: tests) ret.push_back(visitTest(i).as<DataType>());
        return ret;
    }

};


#endif //PYTHON_INTERPRETER_EVALVISITOR_H