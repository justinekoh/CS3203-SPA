#include "QPS/Parsers/PQLParser.h"
#include "QPS/Exceptions/SyntaxException.h"
#include "QPS/Exceptions/SemanticException.h"
#include "QPSTestUtil.h"

#include "catch.hpp"


TEST_CASE("single declaration, single Select") {
    std::string input = "stmt s; Select s";
    PQLParser parser(input);
    Query query = parser.parse();
    QueryEntity expectedEntity = QueryEntity(QueryEntityType::Stmt, "s");
    std::shared_ptr<QueryEntity> declarationEntity = query.getEntity("s");
    Synonym selectEntity = query.getSelect()[0];

    REQUIRE(query.hasDeclarations());
    REQUIRE(query.getEntity("s"));
    REQUIRE(*declarationEntity == expectedEntity);
    REQUIRE(declarationEntity == query.getEntity(selectEntity));
}

TEST_CASE("processDeclarations serial declaration") {
    std::string input = "variable v,v1,v2; Select v";
    PQLParser parser(input);
    Query query = parser.parse();
    auto declaration_map = query.getDeclarations();
    std::shared_ptr<QueryEntity> declarationEntity = query.getEntity("v");
    Synonym selectEntity = query.getSelect()[0];

    REQUIRE(query.hasDeclarations());
    REQUIRE(declaration_map.size()==3);
    REQUIRE(query.getEntity("v"));
    REQUIRE(query.getEntity("v1"));
    REQUIRE(query.getEntity("v2"));
    REQUIRE(declarationEntity == query.getEntity(selectEntity));
}

TEST_CASE("processDeclarations multiple declaration") {
    std::string input = "procedure p; stmt s; read re; print pr; assign a; \n while w; if i; variable v; constant k; \n Select p";
    PQLParser parser(input);
    Query query = parser.parse();
    auto declaration_map = query.getDeclarations();
    std::shared_ptr<QueryEntity> declarationEntity = query.getEntity("p");
    Synonym selectEntity = query.getSelect()[0];

    REQUIRE(query.hasDeclarations());
    REQUIRE(declaration_map.size()==9);
    REQUIRE(query.getEntity("p"));
    REQUIRE(query.getEntity("s"));
    REQUIRE(query.getEntity("re"));
    REQUIRE(query.getEntity("pr"));
    REQUIRE(query.getEntity("a"));
    REQUIRE(query.getEntity("w"));
    REQUIRE(query.getEntity("i"));
    REQUIRE(query.getEntity("v"));
    REQUIRE(query.getEntity("k"));
    REQUIRE(declarationEntity == query.getEntity(selectEntity));
}

TEST_CASE("processDeclarations Errors") {
    SECTION("SyntaxExceptions") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("assignment a; Select a", "Expected a declaration but found none");
        testcases.emplace_back("assign a Select s", "Expected ; but found 'Select'");
        testcases.emplace_back("assign a a1; Select a1", "Expected ; but found 'a1'");
        testcases.emplace_back("assign a;", "Expected Select clause but found ''");
        testcases.emplace_back("assign 1; Select 1", "Invalid synonym");
        testcases.emplace_back("assign -a ; Select -a", "Invalid synonym");
        testcases.emplace_back("stmt", "Invalid synonym");
        testcases.emplace_back("stmt ; ",  "Invalid synonym");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }
    }

    SECTION("SemanticExceptions") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("Select s ", "Undeclared synonym in Select clause");
        testcases.emplace_back("stmt s; assign s; Select s ", "Trying to redeclare a synonym");
        testcases.emplace_back("Select s ", "undeclared synonym");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SemanticException);
        }
    }
}

TEST_CASE("processSelect Errors") {
    SECTION("SyntaxException") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("stmt s; where s", "Expected Select clause but found 'where'");
        testcases.emplace_back("assign a; Select", "Invalid synonym syntax");
        testcases.emplace_back("assign a; Select a;", "Invalid synonym syntax");
        testcases.emplace_back("assign a; Select -a", "Invalid synonym syntax");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }
    }
    SECTION("SemanticException") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("assign a; Select s", "Undeclared synonym in Select clause");
        testcases.emplace_back("assign Synonym; Select synonym", "Undeclared synonym in Select clause");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SemanticException);
        }
    }

}

TEST_CASE("processSuchThatClause Uses") {
    SECTION("Valid Uses query") {
        PQLParser parser("assign a; variable v;\nSelect a such that Uses(a, v)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Uses);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Assign);
        REQUIRE(leftRef.getRep() == "a");
        REQUIRE(rightRef.getType() == RefType::EntRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Variable);
        REQUIRE(rightRef.getRep() == "v");
    }

    SECTION("Valid Uses query") {
        PQLParser parser("assign x;\nSelect x such that Uses(x, \"x\")");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Uses);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Assign);
        REQUIRE(leftRef.getRep() == "x");
        REQUIRE(rightRef.getType() == RefType::EntRef);
        REQUIRE(rightRef.getRootType() == RootType::Ident);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "x");
    }
}

TEST_CASE("processSuchThatClause Modifies") {
    SECTION("Valid Modifies(s,v) query") {
        PQLParser parser("stmt s; variable v; \nSelect v such that Modifies(s,v)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Modifies);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Stmt);
        REQUIRE(leftRef.getRep() == "s");
        REQUIRE(rightRef.getType() == RefType::EntRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Variable);
        REQUIRE(rightRef.getRep() == "v");
    }

    SECTION("Valid Modifies(s,ident) query") {
        PQLParser parser("read re; \nSelect re such that Modifies(re,\"variable\")");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Modifies);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Read);
        REQUIRE(leftRef.getRep() == "re");
        REQUIRE(rightRef.getType() == RefType::EntRef);
        REQUIRE(rightRef.getRootType() == RootType::Ident);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "variable");
    }

    SECTION("Valid Modifies(integer,v) query") {
        PQLParser parser("variable l; \nSelect l such that Modifies(3,l)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Modifies);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "3");
        REQUIRE(rightRef.getType() == RefType::EntRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Variable);
        REQUIRE(rightRef.getRep() == "l");
    }

    SECTION("Valid Modifies(integer,ident) query") {
        PQLParser parser("stmt s; \nSelect s such that Modifies(1,\"hello\")");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Modifies);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "1");
        REQUIRE(rightRef.getType() == RefType::EntRef);
        REQUIRE(rightRef.getRootType() == RootType::Ident);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "hello");
    }

    SECTION("Valid Modifies(s,_) query") {
        PQLParser parser("assign a; \nSelect a such that Modifies(a,_)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Modifies);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Assign);
        REQUIRE(leftRef.getRep() == "a");
        REQUIRE(rightRef.getType() == RefType::EntRef);
        REQUIRE(rightRef.getRootType() == RootType::Wildcard);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "_");
    }

    SECTION("Valid Modifies(integer,_) query") {
        PQLParser parser("assign a; \nSelect a such that Modifies(5,_)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Modifies);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "5");
        REQUIRE(rightRef.getType() == RefType::EntRef);
        REQUIRE(rightRef.getRootType() == RootType::Wildcard);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "_");
    }
}

TEST_CASE("processSuchThatClause Follows") {
    SECTION("Valid Follows(s1,s2)") {
        PQLParser parser("stmt s1, s2;\nSelect s1 such that Follows (s1,s2)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Follows);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Stmt);
        REQUIRE(leftRef.getRep() == "s1");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Stmt);
        REQUIRE(rightRef.getRep() == "s2");
    }

    SECTION("Valid Follows(s,integer)") {
        PQLParser parser("print pr;\nSelect pr  such  that  Follows(pr,1)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Follows);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Print);
        REQUIRE(leftRef.getRep() == "pr");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Integer);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "1");
    }

    SECTION("Valid Follows(s,_)") {
        PQLParser parser("if ifs;\nSelect ifs such that Follows(ifs,_)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Follows);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::If);
        REQUIRE(leftRef.getRep() == "ifs");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Wildcard);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "_");
    }

    SECTION("Valid Follows(integer,s)") {
        PQLParser parser("while w;\nSelect w such  that  Follows(1,w)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Follows);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "1");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::While);
        REQUIRE(rightRef.getRep() == "w");
    }

    SECTION("Valid Follows(_,s)") {
        PQLParser parser("assign a;\nSelect a such that Follows(_,a)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Follows);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Wildcard);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "_");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Assign);
        REQUIRE(rightRef.getRep() == "a");
    }

    SECTION("Valid Follows(integer,integer)") {
        PQLParser parser("stmt s1, s2;\nSelect s1  such  that  Follows(2,3)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Follows);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "2");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Integer);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "3");
    }

    SECTION("Valid Follows(_,_)") {
        PQLParser parser("stmt s1, s2;\nSelect s1  such  that  Follows(_,_)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Follows);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Wildcard);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "_");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Wildcard);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "_");
    }
}

TEST_CASE("processSuchThatClause Parent") {
    SECTION("Valid Parent(s1,s2)") {
        PQLParser parser("print s1, s2;\nSelect s1 such that Parent (s1,s2)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Parent);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Print);
        REQUIRE(leftRef.getRep() == "s1");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Print);
        REQUIRE(rightRef.getRep() == "s2");
    }

    SECTION("Valid Parent(s,integer)") {
        PQLParser parser("print pr;\nSelect pr  such  that  Parent(pr,1)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Parent);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Print);
        REQUIRE(leftRef.getRep() == "pr");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Integer);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "1");
    }

    SECTION("Valid Parent(s,_)") {
        PQLParser parser("if ifs;\nSelect ifs such that Parent(ifs,_)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Parent);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::If);
        REQUIRE(leftRef.getRep() == "ifs");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Wildcard);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "_");
    }

    SECTION("Valid Parent(integer,s)") {
        PQLParser parser("while w;\nSelect w such  that  Parent(1,w)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Parent);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "1");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::While);
        REQUIRE(rightRef.getRep() == "w");
    }

    SECTION("Valid Parent(_,s)") {
        PQLParser parser("assign a;\nSelect a such that Parent(_,a)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Parent);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Wildcard);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "_");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Assign);
        REQUIRE(rightRef.getRep() == "a");
    }

    SECTION("Valid Parent(integer,integer)") {
        PQLParser parser("stmt s1, s2;\nSelect s1  such  that  Parent(2,3)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Parent);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "2");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Integer);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "3");
    }

    SECTION("Valid Parent(_,_)") {
        PQLParser parser("stmt s1, s2;\nSelect s1  such  that  Parent(_,_)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Parent);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Wildcard);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "_");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Wildcard);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "_");
    }
}

TEST_CASE("processSuchThatClause Parent*") {
    SECTION("Valid Parent*(s1,s2)") {
        PQLParser parser("read s1, s2;\nSelect s1 such that Parent* (s1,s2)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::ParentStar);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Read);
        REQUIRE(leftRef.getRep() == "s1");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Read);
        REQUIRE(rightRef.getRep() == "s2");
    }

    SECTION("Valid Parent*(s,integer)") {
        PQLParser parser("assign assign;\nSelect assign such that  Parent*(assign,3)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::ParentStar);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Assign);
        REQUIRE(leftRef.getRep() == "assign");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Integer);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "3");
    }

    SECTION("Valid Parent*(s,_)") {
        PQLParser parser("read hello;\nSelect hello such that Parent*(hello,_)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::ParentStar);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Read);
        REQUIRE(leftRef.getRep() == "hello");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Wildcard);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "_");
    }

    SECTION("Valid Parent*(integer,s)") {
        PQLParser parser("stmt read;\nSelect read such  that  Parent*(1,read)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::ParentStar);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "1");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Stmt);
        REQUIRE(rightRef.getRep() == "read");
    }

    SECTION("Valid Parent*(_,s)") {
        PQLParser parser("while while;\nSelect while such that Parent*(_,while)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::ParentStar);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Wildcard);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "_");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Synonym);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::While);
        REQUIRE(rightRef.getRep() == "while");
    }

    SECTION("Valid Parent*(integer,integer)") {
        PQLParser parser("stmt s1; stmt s2;\nSelect s1  such  that  Parent*(2,3)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::ParentStar);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Integer);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "2");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Integer);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "3");
    }

    SECTION("Valid Parent*(_,_)") {
        PQLParser parser("stmt s1, s2;\nSelect s1  such  that  Parent*(_,_)");
        Query query = parser.parse();
        std::shared_ptr<SuchThatClause> clause = query.getSuchThat()[0];
        Ref leftRef = clause->getFirstParam();
        Ref rightRef = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::ParentStar);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Wildcard);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(leftRef.getRep() == "_");
        REQUIRE(rightRef.getType() == RefType::StmtRef);
        REQUIRE(rightRef.getRootType() == RootType::Wildcard);
        REQUIRE(rightRef.getEntityType() == QueryEntityType::Invalid);
        REQUIRE(rightRef.getRep() == "_");
    }
}

TEST_CASE("processSuchThatClause Calls") {
    SECTION("Valid Calls(_, _)") {
        PQLParser parser("print s1; Select s1 such that Calls (_,_)");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::Calls,
                                                        RefType::EntRef, RootType::Wildcard, QueryEntityType::Invalid, "_",
                                                        RefType::EntRef, RootType::Wildcard, QueryEntityType::Invalid, "_");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }

    SECTION("Valid Calls(p, procName)") {
        PQLParser parser("procedure procedure; Select procedure such that Calls (procedure, \"procName\")");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::Calls,
                                                                RefType::EntRef, RootType::Synonym, QueryEntityType::Procedure, "procedure",
                                                                RefType::EntRef, RootType::Ident, QueryEntityType::Invalid, "procName");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }

    SECTION("Valid Calls(procName, _)") {
        PQLParser parser("procedure procedure; Select procedure such that Calls (\"procedure\", _)");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::Calls,
                                                                RefType::EntRef, RootType::Ident, QueryEntityType::Invalid, "procedure",
                                                                RefType::EntRef, RootType::Wildcard, QueryEntityType::Invalid, "_");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }

    SECTION("Valid Calls(procName, p)") {
        PQLParser parser("procedure procedure, q; Select procedure such that Calls (\"procedure\", q)");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::Calls,
                                                                RefType::EntRef, RootType::Ident, QueryEntityType::Invalid, "procedure",
                                                                RefType::EntRef, RootType::Synonym, QueryEntityType::Procedure, "q");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }
}

TEST_CASE("processSuchThatClause Calls*") {
    SECTION("Valid Calls*(_, p)") {
        PQLParser parser("print s1; procedure q; Select s1 such that Calls* (_,q)");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::CallsStar,
                                                                RefType::EntRef, RootType::Wildcard, QueryEntityType::Invalid, "_",
                                                                RefType::EntRef, RootType::Synonym, QueryEntityType::Procedure, "q");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }

    SECTION("Valid Calls*(_, procName)") {
        PQLParser parser("procedure procedure; Select procedure such that Calls* (_, \"procName\")");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::CallsStar,
                                                                RefType::EntRef, RootType::Wildcard, QueryEntityType::Invalid, "_",
                                                                RefType::EntRef, RootType::Ident, QueryEntityType::Invalid, "procName");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }

    SECTION("Valid Calls*(procName, procName)") {
        PQLParser parser("procedure procedure; Select procedure such that Calls (\"procedure\", \"procedure\")");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::Calls,
                                                                RefType::EntRef, RootType::Ident, QueryEntityType::Invalid, "procedure",
                                                                RefType::EntRef, RootType::Ident, QueryEntityType::Invalid, "procedure");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }

    SECTION("Valid Calls*(p,_)") {
        PQLParser parser("procedure procedure, q; Select procedure such that Calls (q,_)");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::Calls,
                                                                RefType::EntRef, RootType::Synonym, QueryEntityType::Procedure, "q",
                                                                RefType::EntRef, RootType::Wildcard, QueryEntityType::Invalid, "_");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }

    SECTION("Valid Calls*(p,q)") {
        PQLParser parser("procedure procedure, q; Select procedure such that Calls (procedure, q)");
        Query query = parser.parse();
        auto expectedClause = QPSTestUtil::createSuchThatClause(ClauseType::Calls,
                                                                RefType::EntRef, RootType::Synonym, QueryEntityType::Procedure, "procedure",
                                                                RefType::EntRef, RootType::Synonym, QueryEntityType::Procedure, "q");
        REQUIRE(*query.getSuchThat()[0] == *expectedClause);
    }
}

TEST_CASE("Invalid processSuchThat cases") {
    SECTION("Invalid Syntax - general queries") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("assign a; print d;\nSelect a such",
                               "Invalid query syntax");
        testcases.emplace_back("assign a; print d;\nSelect a such that random(a",
                               "Invalid token, abstraction expected");
        testcases.emplace_back("assign a; print d;\nSelect a such that Follows* ",
                               "No left parenthesis");
        testcases.emplace_back("assign a; print d;\nSelect a such that Uses(a",
                               "No comma");
        testcases.emplace_back("assign a; print d;\nSelect a such that Follows(a, d",
                               "No right parenthesis");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }
    }

    SECTION("Invalid Uses queries") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("print a; print d;\nSelect a such that Uses(\"\", d)",
                               "Identity invalid");
        testcases.emplace_back("assign a; print d;\nSelect a such that Uses(a, 2)",
                               "Invalid RHS entRef");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }

        std::vector<std::pair<std::string, std::string>> testcases2;
        testcases2.emplace_back("assign a; variable v;\nSelect a such that Uses(_, v)",
                               "Invalid LHS, wildcard found");
        testcases2.emplace_back("assign a; variable v;\nSelect a such that Uses(v, a)",
                               "Invalid LHS synonym, non-statement found");
        testcases2.emplace_back("assign a; print d;\nSelect a such that Uses(b, d)",
                               "Invalid LHS, undeclared synonym found");

        for (const auto& testcase : testcases2) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SemanticException);
        }
    }

    SECTION("Invalid Modifies queries") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("assign a; constant d;\nSelect a such that Modifies(\"test, d)",
                               "No right quote");
        testcases.emplace_back("print a; print d;\nSelect a such that Modifies(a, 3)",
                              "Invalid RHS entRef");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }

        std::vector<std::pair<std::string, std::string>> testcases2;
        testcases2.emplace_back("assign a; variable v;\nSelect a such that Modifies(_, v)",
                               "Invalid LHS, wildcard found");
        testcases2.emplace_back("print a; constant v;\nSelect a such that Modifies(v, a)",
                               "Invalid LHS synonym, non-statement found");
        testcases2.emplace_back("stmt a; variable d;\nSelect a such that Modifies(b, d)",
                               "Invalid LHS, undeclared synonym found");

        for (const auto& testcase : testcases2) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SemanticException);
        }
    }

    SECTION("Invalid Follow queries") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("procedure a; read v;\nSelect a such that Follows(\"hello\", v)",
                               "Invalid LHS, stmtRef expected");
        testcases.emplace_back("assign a; if v;\nSelect a such that Follows(a, \"world\")",
                               "Invalid RHS, stmtRef expected");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }

        std::vector<std::pair<std::string, std::string>> testcases2;
        testcases2.emplace_back("stmt a; variable v;\nSelect v such that Follows(v, a)",
                               "Invalid LHS synonym, non-statement found");
        testcases2.emplace_back("procedure a; print v;\nSelect v such that Follows(v, a)",
                               "Invalid RHS synonym, non-statement found");
        testcases2.emplace_back("procedure a; assign v;\nSelect v such that Follows(hello, a)",
                               "Invalid LHS, undeclared synonym found");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }
    }

    SECTION("Invalid Parent queries") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("stmt a; print v;\nSelect a such that Parent(\"hello\", v)",
                               "Invalid LHS, stmtRef expected");
        testcases.emplace_back("print a; assign v;\nSelect a such that Parent(a, \"world\")",
                               "Invalid RHS, stmtRef expected");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }

        std::vector<std::pair<std::string, std::string>> testcases2;
        testcases2.emplace_back("stmt a; variable v;\nSelect v such that Parent(v, a)",
                               "Invalid LHS synonym, non-statement found");
        testcases2.emplace_back("procedure a; stmt v;\nSelect v such that Parent(v, a)",
                               "Invalid RHS synonym, non-statement found");
        testcases2.emplace_back("constant a; stmt v;\nSelect v such that Parent(a, v)",
                               "Invalid LHS synonym, non-statement found");
        testcases2.emplace_back("procedure a; stmt v;\nSelect v such that Parent(hello, a)",
                               "Invalid LHS, undeclared synonym found");
        for (const auto& testcase : testcases2) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SemanticException);
        }
    }

    SECTION("Invalid Parent* queries") {
        std::vector<std::pair<std::string, std::string>> testcases;
        testcases.emplace_back("stmt a; print v;\nSelect a such that Parent*(\"hello\", v)",
                               "Invalid LHS, stmtRef expected");
        testcases.emplace_back("print a; assign v;\nSelect a such that Parent*(a, \"world\")",
                               "Invalid RHS, stmtRef expected");

        for (const auto& testcase : testcases) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
        }

        std::vector<std::pair<std::string, std::string>> testcases2;
        testcases2.emplace_back("stmt a; variable v;\nSelect v such that Parent*(v, a)",
                               "Invalid LHS synonym, non-statement found");
        testcases2.emplace_back("procedure a; stmt v;\nSelect v such that Parent*(v, a)",
                               "Invalid RHS synonym, non-statement found");
        testcases2.emplace_back("constant a; stmt v;\nSelect v such that Parent*(a, v)",
                               "Invalid LHS synonym, non-statement found");
        testcases2.emplace_back("procedure a; stmt v;\nSelect v such that Parent*(hello, a)",
                               "Invalid LHS, undeclared synonym found");

        for (const auto& testcase : testcases2) {
            PQLParser parser(testcase.first);
            REQUIRE_THROWS_AS(parser.parse(), SemanticException);
        }
    }

    SECTION("Invalid Calls/Calls* queries"){
        PQLParser parser("assign a; variable v; Select a such that Calls(1,_)");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v; procedure p;Select a such that Calls*(p, 1)");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v; Select a such that Calls(v,_)");
        REQUIRE_THROWS_AS(parser.parse(), SemanticException);

        parser = PQLParser("assign a; variable v; procedure p;Select a such that Calls*(p, a)");
        REQUIRE_THROWS_AS(parser.parse(), SemanticException);
    }

}

TEST_CASE("processPatternClause") {
    SECTION("Valid wildcard pattern") {
        PQLParser parser("assign a; variable v;\nSelect a pattern a(_,_)");
        Query query = parser.parse();

        std::shared_ptr<PatternClause> actualClause = query.getPattern()[0];
        Ref leftRef = actualClause->getFirstParam();
        ExpressionSpec rightRef = actualClause->getSecondParam();
        REQUIRE(actualClause->getType() == ClauseType::Assign);
        REQUIRE(leftRef.getType() == RefType::EntRef);
        REQUIRE(leftRef.getRootType() == RootType::Wildcard);
        REQUIRE(leftRef.getRep() == "_");
        REQUIRE(rightRef.first == ExpressionSpecType::Wildcard);
        REQUIRE(rightRef.second.empty());
    }

    SECTION("Valid pattern, Synonym entRef and exact match") {
        PQLParser parser("assign a; variable v;\nSelect a pattern a(v,\"x * (b * a + (n + (1%c))) \")");
        Query query = parser.parse();

        auto actualClause = query.getPattern()[0];
        Ref leftRef = actualClause->getFirstParam();
        ExpressionSpec rightRef = actualClause->getSecondParam();
        REQUIRE(actualClause->getType() == ClauseType::Assign);
        REQUIRE(leftRef.getType() == RefType::EntRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getRep() == "v");
        REQUIRE(rightRef.first == ExpressionSpecType::ExactMatch);
        REQUIRE(rightRef.second == "((x)*(((b)*(a))+((n)+((1)%(c)))))");
    }

    SECTION("Valid pattern, Synonym entRef and exact match") {
        PQLParser parser("assign a; variable v;\nSelect a pattern a(\"y\",_\"x\"_)");
        Query query = parser.parse();

        auto actualClause = query.getPattern()[0];
        Ref leftRef = actualClause->getFirstParam();
        ExpressionSpec rightRef = actualClause->getSecondParam();
        REQUIRE(actualClause->getType() == ClauseType::Assign);
        REQUIRE(leftRef.getType() == RefType::EntRef);
        REQUIRE(leftRef.getRootType() == RootType::Ident);
        REQUIRE(leftRef.getRep() == "y");
        REQUIRE(rightRef.first == ExpressionSpecType::PartialMatch);
        REQUIRE(rightRef.second == "(x)");
    }

    SECTION("Valid pattern, Synonym entRef and exact match") {
        PQLParser parser("assign a; variable v;\nSelect a such that Uses(a, v) pattern a(\"y\",_\"x\"_)");
        Query query = parser.parse();

        auto actualClause = query.getPattern()[0];
        Ref leftRef = actualClause->getFirstParam();
        ExpressionSpec rightRef = actualClause->getSecondParam();
        REQUIRE(actualClause->getType() == ClauseType::Assign);
        REQUIRE(leftRef.getType() == RefType::EntRef);
        REQUIRE(leftRef.getRootType() == RootType::Ident);
        REQUIRE(leftRef.getRep() == "y");
        REQUIRE(rightRef.first == ExpressionSpecType::PartialMatch);
        REQUIRE(rightRef.second == "(x)");

        auto clause = query.getSuchThat()[0];
        leftRef = clause->getFirstParam();
        Ref rightRef1 = clause->getSecondParam();
        REQUIRE(clause->getType() == ClauseType::Uses);
        REQUIRE(leftRef.getType() == RefType::StmtRef);
        REQUIRE(leftRef.getRootType() == RootType::Synonym);
        REQUIRE(leftRef.getEntityType() == QueryEntityType::Assign);
        REQUIRE(leftRef.getRep() == "a");
        REQUIRE(rightRef1.getType() == RefType::EntRef);
        REQUIRE(rightRef1.getRootType() == RootType::Synonym);
        REQUIRE(rightRef1.getEntityType() == QueryEntityType::Variable);
        REQUIRE(rightRef1.getRep() == "v");
    }

    SECTION("invalid pattern") {
        PQLParser parser("assign a; variable v;\nSelect a pattern a(\"y\",_ _)");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern a(1,_)");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern a(_, varName)");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern v(\"y\",_);");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern v(\"y\",_");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern v(\"y\",_\"\"_");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern v(\"y\",\"\"");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern v(\"y\" _)");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern v(_,_)");
        REQUIRE_THROWS_AS(parser.parse(), SemanticException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern a1(_,_)");
        REQUIRE_THROWS_AS(parser.parse(), SemanticException);

        parser = PQLParser("assign a; variable v;\nSelect a pattern a(v1,_)");
        REQUIRE_THROWS_AS(parser.parse(), SemanticException);
    }

}

TEST_CASE("both clause present") {
    PQLParser parser("assign a; variable v;\nSelect a such that Uses(a, v) pattern a(\"y\",_\"x\"_)");
    Query query = parser.parse();

    auto actualClause = query.getPattern()[0];
    Ref leftRef = actualClause->getFirstParam();
    ExpressionSpec rightRef = actualClause->getSecondParam();
    REQUIRE(actualClause->getType() == ClauseType::Assign);
    REQUIRE(leftRef.getType() == RefType::EntRef);
    REQUIRE(leftRef.getRootType() == RootType::Ident);
    REQUIRE(leftRef.getRep() == "y");
    REQUIRE(rightRef.first == ExpressionSpecType::PartialMatch);
    REQUIRE(rightRef.second == "(x)");

    auto clause = query.getSuchThat()[0];
    leftRef = clause->getFirstParam();
    Ref rightRef1 = clause->getSecondParam();
    REQUIRE(clause->getType() == ClauseType::Uses);
    REQUIRE(leftRef.getType() == RefType::StmtRef);
    REQUIRE(leftRef.getRootType() == RootType::Synonym);
    REQUIRE(leftRef.getEntityType() == QueryEntityType::Assign);
    REQUIRE(leftRef.getRep() == "a");
    REQUIRE(rightRef1.getType() == RefType::EntRef);
    REQUIRE(rightRef1.getRootType() == RootType::Synonym);
    REQUIRE(rightRef1.getEntityType() == QueryEntityType::Variable);
    REQUIRE(rightRef1.getRep() == "v");

}

TEST_CASE("invalid multi-clause queries") {
    SECTION("Syntax Errors") {
        // Use and between different clause types
        PQLParser parser("assign a; variable v; Select v such that Modifies(a,v) and pattern a(_,_) ");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        // Use both `and` & clause keyword (such that)
        parser = PQLParser("assign a; variable v; Select v such that Modifies(a,v) and such that Follows(1, 2)");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        // Use both `and` & clause keyword (pattern)
        parser = PQLParser("assign a; variable v; Select a pattern a(_,_) and pattern a(v,_)");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);

        // Trailing `and`
        parser = PQLParser("assign a; variable v; Select a pattern v(\"y\",_) and ");
        REQUIRE_THROWS_AS(parser.parse(), SyntaxException);
    }
}

TEST_CASE("valid multi-clause queries") {

    SECTION("use AND in such that") {
        PQLParser parser("assign a; variable v; Select v such that Modifies(a,v) and Follows*(1,2) and Uses(a,v)");
        Query query = parser.parse();

        auto stClauses = query.getSuchThat();
        REQUIRE(stClauses.size() == 3);

        auto pClauses = query.getPattern();
        REQUIRE(pClauses.empty());

        auto c1 = QPSTestUtil::createSuchThatClause(ClauseType::Modifies,
                                                    RefType::StmtRef, RootType::Synonym, QueryEntityType::Assign, "a",
                                                    RefType::EntRef, RootType::Synonym, QueryEntityType::Variable, "v");
        auto c2 = QPSTestUtil::createSuchThatClause(ClauseType::FollowsStar,
                                                    RefType::StmtRef, RootType::Integer, QueryEntityType::Invalid, "1",
                                                    RefType::StmtRef, RootType::Integer, QueryEntityType::Invalid, "2");
        auto c3 = QPSTestUtil::createSuchThatClause(ClauseType::Uses,
                                                    RefType::StmtRef, RootType::Synonym, QueryEntityType::Assign, "a",
                                                    RefType::EntRef, RootType::Synonym, QueryEntityType::Variable, "v");
        REQUIRE(*stClauses[0] == *c1);
        REQUIRE(*stClauses[1] == *c2);
        REQUIRE(*stClauses[2] == *c3);

    }

    SECTION("use AND in pattern") {
        PQLParser parser(R"(assign a,pattern; variable v; assign a1; Select a pattern a(_,_) and pattern (v,_) and a1("y","1"))");
        Query query = parser.parse();

        auto stClauses = query.getSuchThat();
        REQUIRE(stClauses.empty());

        auto pClauses = query.getPattern();
        REQUIRE(pClauses.size() == 3);

        auto c1 = QPSTestUtil::createPatternClause(ClauseType::Assign, "a",
                                                   RootType::Wildcard, "_",
                                                   ExpressionSpecType::Wildcard, "" );
        auto c2 = QPSTestUtil::createPatternClause(ClauseType::Assign, "pattern",
                                                   RootType::Synonym, "v",
                                                   ExpressionSpecType::Wildcard, "");
        auto c3 = QPSTestUtil::createPatternClause(ClauseType::Assign, "a1",
                                                   RootType::Ident, "y",
                                                   ExpressionSpecType::ExactMatch, "(1)");
        REQUIRE(*pClauses[0] == *c1);
        REQUIRE(*pClauses[1] == *c2);
        REQUIRE(*pClauses[2] == *c3);

    }

    SECTION("use AND with all clause types") {
        PQLParser parser(R"(assign a; assign and, such; variable v; Select and pattern a(_,_) and and (v,_"1+x"_) such that  Follows(1,2) pattern such(v,_) such that Parent*(1,10))");
        Query query = parser.parse();

        auto stClauses = query.getSuchThat();
        REQUIRE(stClauses.size() == 2);

        auto pClauses = query.getPattern();
        REQUIRE(pClauses.size() == 3);

        auto pc1 = QPSTestUtil::createPatternClause(ClauseType::Assign, "a",
                                                   RootType::Wildcard, "_",
                                                   ExpressionSpecType::Wildcard, "" );
        auto pc2 = QPSTestUtil::createPatternClause(ClauseType::Assign, "and",
                                                   RootType::Synonym, "v",
                                                   ExpressionSpecType::PartialMatch, "((1)+(x))");
        auto pc3 = QPSTestUtil::createPatternClause(ClauseType::Assign, "such",
                                                   RootType::Synonym, "v",
                                                   ExpressionSpecType::Wildcard, "");
        auto sc1 = QPSTestUtil::createSuchThatClause(ClauseType::Follows,
                                                    RefType::StmtRef, RootType::Integer, QueryEntityType::Invalid, "1",
                                                    RefType::StmtRef, RootType::Integer, QueryEntityType::Invalid, "2");
        auto sc2 = QPSTestUtil::createSuchThatClause(ClauseType::ParentStar,
                                                    RefType::StmtRef, RootType::Integer, QueryEntityType::Invalid, "1",
                                                    RefType::StmtRef, RootType::Integer, QueryEntityType::Invalid, "10");

        REQUIRE(*pClauses[0] == *pc1);
        REQUIRE(*pClauses[1] == *pc2);
        REQUIRE(*pClauses[2] == *pc3);

        REQUIRE(*stClauses[0] == *sc1);
        REQUIRE(*stClauses[1] == *sc2);
    }
}