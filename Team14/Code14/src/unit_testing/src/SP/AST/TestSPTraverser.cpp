#include <vector>
#include <iostream>

#include "catch.hpp"
#include "SP/SPParser.h"
#include "SP/SPToken.h"
#include "SP/SPTokenType.h"
#include "SP/AST/Nodes/ProgramNode.h"
#include "SP/AST/Visitors/DesignExtractorVisitor.h"
#include "SP/AST/Visitors/EntityExtractorVisitor.h"
#include "SP/AST/Visitors/FollowsExtractorVisitor.h"
#include "SP/AST/Visitors/UsesExtractorVisitor.h"
#include "SP/AST/Visitors/ModifiesExtractorVisitor.h"
#include "SP/AST/Visitors/ParentExtractorVisitor.h"
#include "SP/AST/Traverser/Traverser.h"
#include "PKB/Pkb.h"
#include "PKB/PkbConcreteWriter.h"
#include "ASTGenerator.cpp"

TEST_CASE("Test AST Traverser - e2e for Follows and Uses") {
    SPParser parser;
    std::string varName = "num1";
    std::vector<SPToken> tokens = {
            SPToken(TokenType::Name, "procedure"),
            SPToken(TokenType::Name, "doMath"),
            SPToken(TokenType::OpenCurlyParenthesis, "{"),
            SPToken(TokenType::Name, "x"),
            SPToken(TokenType::Equals, "="),
            SPToken(TokenType::Name, "v"),
            SPToken(TokenType::ArithmeticOperator, "+"),
            SPToken(TokenType::Integer, "1"),
            SPToken(TokenType::ArithmeticOperator, "*"),
            SPToken(TokenType::Name, "y"),
            SPToken(TokenType::ArithmeticOperator, "+"),
            SPToken(TokenType::Name, "z"),
            SPToken(TokenType::ArithmeticOperator, "*"),
            SPToken(TokenType::Name, "t"),
            SPToken(TokenType::Semicolon, ";"),
            SPToken(TokenType::Name, "read"),
            SPToken(TokenType::Name, varName),
            SPToken(TokenType::Semicolon, ";"),
            SPToken(TokenType::Name, "print"),
            SPToken(TokenType::Name, varName),
            SPToken(TokenType::Semicolon, ";"),
            SPToken(TokenType::CloseCurlyParenthesis, "}")
    };

    std::shared_ptr<ProgramNode> rootNode = parser.parse(tokens);

    auto assignmentManager = std::make_shared<AssignmentManager>(AssignmentManager());
    auto constantStore = std::make_shared<ConstantStore>(ConstantStore());
    auto procedureStore = std::make_shared<ProcedureStore>(ProcedureStore());
    auto statementStore = std::make_shared<StatementStore>(StatementStore());
    auto variableStore = std::make_shared<VariableStore>(VariableStore());
    auto followsRelationshipManager = std::make_shared<FollowsRelationshipManager>();
    auto usesRelationshipManager = std::make_shared<UsesRelationshipManager>();
    auto modifiesRelationshipManager = std::make_shared<ModifiesRelationshipManager>();
    auto parentRelationshipManager = std::make_shared<ParentRelationshipManager>();
    auto pkbWriterManager = std::make_shared<PkbWriterManager>(
            assignmentManager,
            constantStore,
            procedureStore,
            statementStore,
            variableStore,
            followsRelationshipManager,
            usesRelationshipManager,
            modifiesRelationshipManager,
            parentRelationshipManager
    );
    std::shared_ptr<PkbConcreteWriter> pkbWriter = std::make_shared<PkbConcreteWriter>(pkbWriterManager);

    std::shared_ptr<EntityExtractorVisitor> entityExtractor = std::make_shared<EntityExtractorVisitor>(pkbWriter);
    std::shared_ptr<FollowsExtractorVisitor> followsExtractor = std::make_shared<FollowsExtractorVisitor>(pkbWriter);
    std::shared_ptr<UsesExtractorVisitor> usesExtractor = std::make_shared<UsesExtractorVisitor>(pkbWriter);
    std::shared_ptr<ModifiesExtractorVisitor> modifiesExtractor = std::make_shared<ModifiesExtractorVisitor>(pkbWriter);

    std::vector<std::shared_ptr<DesignExtractorVisitor>> visitors = { entityExtractor, followsExtractor, usesExtractor, modifiesExtractor };

    //Traverse the AST from root node
    Traverser traverser = Traverser(visitors);
    traverser.traverse(rootNode);

    REQUIRE(*(procedureStore->getEntity(std::make_shared<Procedure>(Procedure("doMath")))) == *(std::make_shared<Procedure>("doMath")));
    REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("x")))) == *(std::make_shared<Variable>("x")));
    REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("v")))) == *(std::make_shared<Variable>("v")));
    REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("y")))) == *(std::make_shared<Variable>("y")));
    REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("z")))) == *(std::make_shared<Variable>("z")));
    REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("t")))) == *(std::make_shared<Variable>("t")));
    REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("num1")))) == *(std::make_shared<Variable>("num1")));

    REQUIRE(*(statementStore->getEntity(std::make_shared<Statement>(Statement(1,StatementType::Assign)))) == *(std::make_shared<Statement>(1, StatementType::Assign)));
    REQUIRE(*(statementStore->getEntity(std::make_shared<Statement>(Statement(2, StatementType::Read)))) == *(std::make_shared<Statement>(2, StatementType::Read)));
    REQUIRE(*(statementStore->getEntity(std::make_shared<Statement>(Statement(3, StatementType::Print)))) == *(std::make_shared<Statement>(3, StatementType::Print)));

    REQUIRE(*(constantStore->getEntity(std::make_shared<Constant>(Constant(1)))) == *(std::make_shared<Constant>(1)));

    auto VarV = Variable("v");
    auto usesV = usesRelationshipManager->getUsesTypeIdent(StatementType::Assign, VarV);
    REQUIRE(usesV.size() == 1);
    REQUIRE(usesV.at(0) == Statement(1, StatementType::Assign));

    auto VarY = Variable("y");
    auto usesY = usesRelationshipManager->getUsesTypeIdent(StatementType::Assign, VarY);
    REQUIRE(usesY.size() == 1);
    REQUIRE(usesY.at(0) == Statement(1, StatementType::Assign));

    auto Stmt1 = Statement(1, StatementType::Assign);
    auto followsRS = followsRelationshipManager->getFollowsStmtType(Stmt1, StatementType::Stmt, true);
    REQUIRE(followsRS.size() == 1);
    REQUIRE(followsRS.at(0) == Statement(2, StatementType::Read));

    auto Stmt2 = Statement(2, StatementType::Read);
    auto followsRS2 = followsRelationshipManager->getFollowsStmtType(Stmt2, StatementType::Stmt, true);
    REQUIRE(followsRS2.size() == 1);
    REQUIRE(followsRS2.at(0) == Statement(3, StatementType::Print));

    //auto usesVarName = *(usesRelationshipManager->getVariableAssignment(std::make_shared<Variable>(varName)));
    //REQUIRE(usesVarName.size() == 1);
    ////REQUIRE(*(usesVarName.at(0)) == Statement(3, StatementType::Print));
}


TEST_CASE("Test AST Traverser - e2e with nested structure") {
    std::string sourceCode = "\
        procedure kk { \
        if (a < b) then { \
            if (c<d) then { \
            e = f; \
            g = 1; \
            } else { \
            read h; \
            } \
        } else { \
            print i; \
            while ( j<k ) { \
                a = 1; \
            } \
        } \
        read j; \
        m = x + c;\
        }";

    std::shared_ptr<ProgramNode> rootNode = ASTGenerator::generate(sourceCode);

    auto assignmentManager = std::make_shared<AssignmentManager>(AssignmentManager());
    auto constantStore = std::make_shared<ConstantStore>(ConstantStore());
    auto procedureStore = std::make_shared<ProcedureStore>(ProcedureStore());
    auto statementStore = std::make_shared<StatementStore>(StatementStore());
    auto variableStore = std::make_shared<VariableStore>(VariableStore());
    auto followsRelationshipManager = std::make_shared<FollowsRelationshipManager>();
    auto usesRelationshipManager = std::make_shared<UsesRelationshipManager>();
    auto modifiesRelationshipManager = std::make_shared<ModifiesRelationshipManager>();
    auto parentRelationshipManager = std::make_shared<ParentRelationshipManager>();
    auto pkbWriterManager = std::make_shared<PkbWriterManager>(
        assignmentManager,
        constantStore,
        procedureStore,
        statementStore,
        variableStore,
        followsRelationshipManager,
        usesRelationshipManager,
        modifiesRelationshipManager,
        parentRelationshipManager
    );
    std::shared_ptr<PkbConcreteWriter> pkbWriter = std::make_shared<PkbConcreteWriter>(pkbWriterManager);

    std::shared_ptr<EntityExtractorVisitor> entityExtractor = std::make_shared<EntityExtractorVisitor>(pkbWriter);
    std::shared_ptr<FollowsExtractorVisitor> followsExtractor = std::make_shared<FollowsExtractorVisitor>(pkbWriter);
    std::shared_ptr<UsesExtractorVisitor> usesExtractor = std::make_shared<UsesExtractorVisitor>(pkbWriter);
    std::shared_ptr<ModifiesExtractorVisitor> modifiesExtractor = std::make_shared<ModifiesExtractorVisitor>(pkbWriter);
    std::shared_ptr<ParentExtractorVisitor> parentExtractor = std::make_shared<ParentExtractorVisitor>(pkbWriter);

    std::vector<std::shared_ptr<DesignExtractorVisitor>> visitors = { entityExtractor, followsExtractor, usesExtractor, modifiesExtractor, parentExtractor };

    //Traverse the AST from root node
    Traverser traverser = Traverser(visitors);
    traverser.traverse(rootNode);

    REQUIRE(*(procedureStore->getEntity(std::make_shared<Procedure>(Procedure("kk")))) == *(std::make_shared<Procedure>("kk")));
    
    // Check Follows
    /*auto stmt1 = Statement(1, StatementType::If);
    auto follows1 = followsRelationshipManager->getFollowsStmtType(stmt1, StatementType::Stmt, true);
    REQUIRE(follows1.size() == 1);
    REQUIRE(follows1.at(0) == Statement(2, StatementType::Read));*/

    
    // Check Follows*
    
    // Check Uses
    
    // Check Modifies
    
    // Check Parent
    
    // Check Parent*

    //REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("x")))) == *(std::make_shared<Variable>("x")));
    //REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("v")))) == *(std::make_shared<Variable>("v")));
    //REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("y")))) == *(std::make_shared<Variable>("y")));
    //REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("z")))) == *(std::make_shared<Variable>("z")));
    //REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("t")))) == *(std::make_shared<Variable>("t")));
    //REQUIRE(*(variableStore->getEntity(std::make_shared<Variable>(Variable("num1")))) == *(std::make_shared<Variable>("num1")));

    //REQUIRE(*(statementStore->getEntity(std::make_shared<Statement>(Statement(1, StatementType::Assign)))) == *(std::make_shared<Statement>(1, StatementType::Assign)));
    //REQUIRE(*(statementStore->getEntity(std::make_shared<Statement>(Statement(2, StatementType::Read)))) == *(std::make_shared<Statement>(2, StatementType::Read)));
    //REQUIRE(*(statementStore->getEntity(std::make_shared<Statement>(Statement(3, StatementType::Print)))) == *(std::make_shared<Statement>(3, StatementType::Print)));

    //REQUIRE(*(constantStore->getEntity(std::make_shared<Constant>(Constant(1)))) == *(std::make_shared<Constant>(1)));

    //auto VarV = Variable("v");
    //auto usesV = usesRelationshipManager->getUsesTypeIdent(StatementType::Assign, VarV);
    //REQUIRE(usesV.size() == 1);
    //REQUIRE(usesV.at(0) == Statement(1, StatementType::Assign));

    //auto VarY = Variable("y");
    //auto usesY = usesRelationshipManager->getUsesTypeIdent(StatementType::Assign, VarY);
    //REQUIRE(usesY.size() == 1);
    //REQUIRE(usesY.at(0) == Statement(1, StatementType::Assign));

    //auto Stmt1 = Statement(1, StatementType::Assign);
    //auto followsRS = followsRelationshipManager->getFollowsStmtType(Stmt1, StatementType::Stmt, true);
    //REQUIRE(followsRS.size() == 1);
    //REQUIRE(followsRS.at(0) == Statement(2, StatementType::Read));

    //auto Stmt2 = Statement(2, StatementType::Read);
    //auto followsRS2 = followsRelationshipManager->getFollowsStmtType(Stmt2, StatementType::Stmt, true);
    //REQUIRE(followsRS2.size() == 1);
    //REQUIRE(followsRS2.at(0) == Statement(3, StatementType::Print));

    ////auto usesVarName = *(usesRelationshipManager->getVariableAssignment(std::make_shared<Variable>(varName)));
    ////REQUIRE(usesVarName.size() == 1);
    //////REQUIRE(*(usesVarName.at(0)) == Statement(3, StatementType::Print));
}