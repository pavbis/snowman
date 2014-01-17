/* The file is part of Snowman decompiler.             */
/* See doc/licenses.txt for the licensing information. */

/* * SmartDec decompiler - SmartDec is a native code to C/C++ decompiler
 * Copyright (C) 2015 Alexander Chernov, Katerina Troshina, Yegor Derevenets,
 * Alexander Fokin, Sergey Levin, Leonid Tsvetkov
 *
 * This file is part of SmartDec decompiler.
 *
 * SmartDec decompiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SmartDec decompiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SmartDec decompiler.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <nc/config.h>

#include <boost/unordered_map.hpp>

#include "DeclarationGenerator.h"

namespace nc {

class SizedValue;

namespace core {

namespace likec {
    class Block;
    class Expression;
    class LabelDeclaration;
    class Statement;
}

namespace ir {

class BinaryOperator;
class BasicBlock;
class Dominators;
class JumpTarget;
class Statement;
class UnaryOperator;

namespace cflow {
    class Graph;
    class Node;
}

namespace dflow {
    class Dataflow;
    class Uses;
}

namespace vars {
    class Variable;
    class Variables;
}

namespace liveness {
    class Liveness;
}

namespace cgen {

class SwitchContext;

/**
 * Generator of function definitions.
 */
class DefinitionGenerator: public DeclarationGenerator {
    const dflow::Dataflow &dataflow_; ///< Dataflow information.
    const cflow::Graph &graph_; ///< Reduced control-flow graph.
    const liveness::Liveness &liveness_; ///< Liveness information.
    std::unique_ptr<dflow::Uses> uses_; ///< Information about which term is used by which term.
    std::unique_ptr<Dominators> dominators_; ///< Dominator sets for the function.

    likec::FunctionDefinition *definition_; ///< Function's definition.

    int serial_; ///< Last serial number of local variable.
    boost::unordered_map<const ir::vars::Variable *, likec::VariableDeclaration *> variableDeclarations_; ///< Local variables of current function definition.
    boost::unordered_map<const BasicBlock *, likec::LabelDeclaration *> labels_; ///< Labels inside the function.
    boost::unordered_map<const vars::Variable *, bool> isSingleAssignment_; ///< Memoized results of isSingleAssignment().
    boost::unordered_map<const vars::Variable *, bool> isIntermediate_; ///< Memoized results of isIntermediate().

public:
    /**
     * \param[in] parent Parent code generator.
     * \param[in] function Valid pointer to the function being translated.
     * \param[in] canceled Cancellation token.
     */
    DefinitionGenerator(CodeGenerator &parent, const Function *function, const CancellationToken &canceled);

    /**
     * Destructor.
     */
    ~DefinitionGenerator();

    /**
     * \return Dataflow information about the function.
     */
    const dflow::Dataflow &dataflow() const { return dataflow_; }

    /**
     * \return Liveness information.
     */
    const liveness::Liveness &liveness() const { return liveness_; }

    /**
     * \return Reduced control-flow graph.
     */
    const cflow::Graph &graph() const { return graph_; }

    /**
     * \return Generated function's definition.
     */
    likec::FunctionDefinition *definition() const { return definition_; }

    /**
     * Sets function's definition without taking ownership on it.
     *
     * \param[in] definition Function's definition.
     *
     * \return Valid pointer to the created definition.
     */
    void setDefinition(likec::FunctionDefinition *definition);

    /**
     * Creates function's declaration and sets function's declaration to it.
     */
    std::unique_ptr<likec::FunctionDefinition> createDefinition();

protected:
    /**
     * Creates a declaration of function argument for given term.
     * Declaration is automatically added to the list of formal arguments
     * of function's declaration and to the list of its variables.
     *
     * \param[in] term Valid pointer to a term.
     *
     * \return Created declaration of function's formal argument.
     */
    likec::ArgumentDeclaration *makeArgumentDeclaration(const Term *term);

    /**
     * \param[in] variable Valid pointer to a local variable.
     *
     * \return Valid pointer to the local variable declaration.
     */
    likec::VariableDeclaration *makeLocalVariableDeclaration(const vars::Variable *variable);

    /**
     * \param[in] variable Valid pointer to a variable.
     *
     * \return Valid pointer to the declaration of this variable.
     */
    likec::VariableDeclaration *makeVariableDeclaration(const vars::Variable *variable);

    /**
     * \param[in] basicBlock Valid pointer to a basic block.
     *
     * \return Declaration of a label for given basic block.
     * If necessary, the declaration is created and added to the current function definition.
     */
    likec::LabelDeclaration *makeLabel(const BasicBlock *basicBlock);

    /**
     * Adds usual and case labels corresponding to the basic block to the given block.
     *
     * \param[in] basicBlock    Valid pointer to a basic block.
     * \param     block         Valid pointer to the block where the labels must be added.
     * \param     switchContext Switch context.
     */
    void addLabels(const BasicBlock *basicBlock, likec::Block *block, SwitchContext &switchContext);

    /**
     * Generates code for the given node and adds it to the given block.
     *
     * \param node          Valid pointer to the node to generate code for.
     * \param block         Valid pointer to the block where the code will be added.
     * \param nextBB        Pointer to the basic block getting control after the generated code. Can be NULL.
     * \param breakBB       Pointer to the basic block getting control by 'break' statement. Can be NULL.
     * \param continueBB    Pointer to the basic block getting control by 'continue' statement. Can be NULL.
     * \param switchContext Switch context.
     */
    void makeStatements(const cflow::Node *node, likec::Block *block, const BasicBlock *nextBB, const BasicBlock *breakBB, const BasicBlock *continueBB, SwitchContext &switchContext);

    /**
     * Generates code for the given sequence of nodes.
     *
     * \param nodes         Valid pointers to the node to generate code for.
     * \param block         Valid pointer to the block where the code will be added.
     * \param nextBB        Pointer to the basic block getting control after the generated code. Can be NULL.
     * \param breakBB       Pointer to the basic block getting control by 'break' statement. Can be NULL.
     * \param continueBB    Pointer to the basic block getting control by 'continue' statement. Can be NULL.
     * \param switchContext Switch context.
     */
    void makeStatements(const std::vector<cflow::Node *> &nodes, likec::Block *block, const BasicBlock *nextBB, const BasicBlock *breakBB, const BasicBlock *continueBB, SwitchContext &switchContext);

    /**
     * Creates an expression from a condition node.
     *
     * \param node Valid pointer to the node.
     * \param block Pointer to the block. Can be NULL.
     * \param thenBB Valid pointer to the basic block that will get control if the generated expression is true.
     * \param elseBB Valid pointer to the basic block that will get control if the generated expression is false.
     * \param switchContext Switch context.
     *
     * \return Valid pointer to the generated expression.
     */
    std::unique_ptr<likec::Expression> makeExpression(const cflow::Node *node, likec::Block *block, const BasicBlock *thenBB, const BasicBlock *elseBB, SwitchContext &switchContext);

    /**
     * Creates a LikeC statement for given IR statement and sets the pointer to source
     * IR statement in created statement.
     *
     * \param[in] statement     Valid pointer to an IR Statement.
     * \param[in] nextBB        Pointer to the basic block, whose code will textually
     *                          follow the basic block of the statement. Can be NULL.
     * \param[in] breakBB       Pointer to the basic block getting control by break statement. Can be NULL.
     * \param[in] continueBB    Pointer to the basic block getting control by continue statement. Can be NULL.
     *
     * \return Valid pointer to created LikeC statement.
     */
    std::unique_ptr<likec::Statement> makeStatement(const Statement *statement, const BasicBlock *nextBB, const BasicBlock *breakBB, const BasicBlock *continueBB);

    /**
     * Actually creates a LikeC statement for given IR statement.
     *
     * \param[in] statement             Valid pointer to the IR Statement.
     * \param[in] nextBB                Pointer to a basic block, whose code will textually
     *                                  follow the basic block given statement is in. Can be null.
     * \param[in] breakBB               Pointer to a basic block, where control will be transferred
     *                                  by break statement.
     * \param[in] continueBB               Pointer to a basic block, where control will be transferred
     *                                  by continue statement.
     *
     * \return Valid pointer to created LikeC statement.
     */
    std::unique_ptr<likec::Statement> doMakeStatement(const Statement *statement, const BasicBlock *nextBB, const BasicBlock *breakBB, const BasicBlock *continueBB);

    /**
     * Creates a goto statement to the given target.
     *
     * \param[in] target        Valid pointer to the target basic block.
     * \param[in] nextBB        Pointer to the basic block, whose code will textually
     *                          follow the basic block of the created statement. Can be NULL.
     * \param[in] breakBB       Pointer to the basic block getting control by break statement. Can be NULL.
     * \param[in] continueBB    Pointer to the basic block getting control by continue statement. Can be NULL.
     *
     * \return Pointer to the created goto, break, or continue statement, or NULL if the target is equal to nextBB.
     */
    std::unique_ptr<likec::Statement> makeJump(const BasicBlock *target, const BasicBlock *nextBB, const BasicBlock *breakBB, const BasicBlock *continueBB);

    /**
     * Creates a goto statement to given target.
     *
     * \param[in] target   Valid pointer to the target basic block.
     * \param[in] nextBB        Pointer to the basic block, whose code will textually
     *                          follow the basic block of the created statement. Can be NULL.
     * \param[in] breakBB       Pointer to the basic block getting control by break statement. Can be NULL.
     * \param[in] continueBB    Pointer to the basic block getting control by continue statement. Can be NULL.
     *
     * \return Pointer to the created goto, break, or continue statement, or NULL if the target is equal to nextBB.
     */
    std::unique_ptr<likec::Statement> makeJump(const JumpTarget &target, const BasicBlock *nextBB, const BasicBlock *breakBB, const BasicBlock *continueBB);

    /**
     * Creates a LikeC expression for given term and sets the pointer to source IR term
     * in created IR expression.
     *
     * \param[in] term Term to create expression from.
     *
     * \return Valid pointer to created LikeC expression.
     */
    std::unique_ptr<likec::Expression> makeExpression(const Term *term);

    /**
     * Actually creates a LikeC expression for given term.
     *
     * \param[in] term Term to create expression from.
     *
     * \return Valid pointer to created LikeC expression.
     */
    std::unique_ptr<likec::Expression> doMakeExpression(const Term *term);

    /**
     * Actually creates a LikeC expression for given UnaryOperator.
     *
     * \param[in] unary Term to create expression from.
     *
     * \return Valid pointer to created LikeC expression.
     */
    std::unique_ptr<likec::Expression> doMakeExpression(const UnaryOperator *unary);

    /**
     * Actually creates a LikeC expression for given BinaryOperator.
     *
     * \param[in] binary Term to create expression from.
     *
     * \return Valid pointer to created LikeC expression.
     */
    std::unique_ptr<likec::Expression> doMakeExpression(const BinaryOperator *binary);

    /**
     * Creates an integer constant with given value from given term.
     *
     * \param[in] term Valid pointer to a term.
     * \param[in] value Value of the constant.
     */
    std::unique_ptr<likec::Expression> makeConstant(const Term *term, const SizedValue &value);

    /**
     * Creates an access to a variable associated with a term.
     *
     * \param term Valid pointer to the term.
     */
    std::unique_ptr<likec::Expression> makeVariableAccess(const Term *term);

    /**
     * \param[in] write Valid pointer to a write term.
     * \param[in] read Valid pointer to a read term.
     *
     * \return True if the write dominates the read, false otherwise.
     */
    bool isDominating(const Term *write, const Term *read) const;

    /**
     * \param[in] variable Valid pointer to a variable.
     *
     * \return Pointer to a write term belonging to the variable, if
     *         there is only one such term. Otherwise, NULL.
     */
    const Term *getSingleDefinition(const vars::Variable *variable) const;

    /**
     * \param[in] variable Valid pointer to a variable.
     *
     * \return Pointer to a live read term belonging to the variable,
     *         if there is only only one such term. Otherwise, NULL.
     */
    const Term *getSingleUse(const vars::Variable *variable) const;

    /**
     * \param[in] variable Valid pointer to a variable.
     *
     * \return True if the variable is written to only once, and all its
     *         live uses are guaranteed to read initialized value.
     */
    bool isSingleAssignment(const vars::Variable *variable);

    /**
     * \param[in] term Valid pointer to a term.
     *
     * \return True if the expression generated from this term can be
     *         moved to a different place without its value being
     *         affected, false otherwise.
     */
    bool isMovable(const Term *term);

    /**
     * \param[in] variable Valid pointer to a variable.
     *
     * \return True if all occurrences of this variable can be replaced
     *         by the expression assigned to it.
     */
    bool isIntermediate(const vars::Variable *variable);
};

} // namespace cgen
} // namespace ir
} // namespace core
} // namespace nc

/* vim:set et sts=4 sw=4: */
