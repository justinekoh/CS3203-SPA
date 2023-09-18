
#include "SP/SyntacticValidator.h"
#include "SP/SPTokenizer.h"

#include <string>
#include "catch.hpp"

TEST_CASE("SyntacticValidator - Valid syntax") {

    SECTION("Empty Procedure") {
        std::string input = "procedure testProcedure {}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Single Read statement")  {
        std::string input = "procedure testProcedure {read num1;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Single Print statement")  {
        std::string input = "procedure testProcedure {print num1;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Two different statement")  {
        std::string input = "procedure testProcedure {read num1; print num2;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Single Assign statement")  {
        std::string input = "procedure testProcedure {x = 1;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Assign statement with expr")  {
        std::string input = "procedure testProcedure {x = 1 + 1;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Assign statement with complex expr")  {
        std::string input = "procedure testProcedure {x = a + b - 1 * d / 5 % f;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Assign statement with parantheses")  {
        std::string input = "procedure testProcedure {x = (1 + 1);}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Assign statement with parantheses 2")  {
        std::string input = "procedure testProcedure {x = (1 * 1) + 1;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }

    SECTION("Assign statement with nested paran")  {
        std::string input = "procedure testProcedure {x = (1 * (1)) + 1;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_NOTHROW(validator.validate());
    }
}

TEST_CASE("SyntacticValidator - Invalid syntax") {

    SECTION("No procedure terminal") {
        std::string input = "wrongProcedure test {}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_THROWS_WITH(validator.validate(),
                            "Syntax error: Expected 'procedure'");
    }

    SECTION("Expecting next procedure") {
        std::string input = "procedure testProcedure {}read num1;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_THROWS_WITH(validator.validate(),
                            "Syntax error: Expected 'procedure'");
    }

    SECTION("Invalid statement") {
        std::string input = "procedure testProcedure {num1}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_THROWS_WITH(validator.validate(),
                            "Syntax error: Expected TokenType EQUALS");
    }

    SECTION("Invalid statement") {
        std::string input = "procedure testProcedure {x = ((1 * 1)) + (1;}";
        SPTokenizer tokenizer(input);
        std::vector<SPToken> tokens = tokenizer.tokenize();
        SyntacticValidator validator(tokens);
        REQUIRE_THROWS_WITH(validator.validate(),
                            "Syntax error: Expected TokenType CLOSE_ROUND_PARAN");
    }
}