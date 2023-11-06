#include "SyntacticValidator.h"
#include "SP/Errors/SyntaxError.h"

SyntacticValidator::SyntacticValidator(const std::vector<SPToken> &tokens) : tokens(tokens), curr(0) {}

std::vector<SPToken> SyntacticValidator::validate() {
    while (isCurrValid()) { validateProcedure(); }
    return tokens;
}

void SyntacticValidator::validateProcedure() {
    SPToken token = peekToken();
    if (token.getType() == TokenType::Name && token.getValue() == AppConstants::STRING_PROCEDURE) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected '" + AppConstants::STRING_PROCEDURE + "'");
    }

    validateName();
    validateOpenCurlyParen();
    validateStmtLst();
    validateCloseCurlyParen();
}

void SyntacticValidator::validateStmtLst() {
    if (peekToken().getType() == TokenType::CloseCurlyParenthesis) {
        throw SyntaxError("Syntax error: StmtLst cannot be empty");
    }

    while (isCurrValid()) {
        SPToken token = peekToken();

        if (token.getType() == TokenType::CloseCurlyParenthesis) {
            break;
        } else if (peekNextToken().getType() == TokenType::Equals) {
            validateAssign();
        } else if (token.getType() == TokenType::Name) {
            if (token.getValue() == AppConstants::STRING_READ || token.getValue() == AppConstants::STRING_PRINT ||
                token.getValue() == AppConstants::STRING_CALL) {
                validateReadPrintCall();
            } else if (token.getValue() == AppConstants::STRING_WHILE) {
                validateWhile();
            } else if (token.getValue() == AppConstants::STRING_IF) {
                validateIf();
            } else {
                throw SyntaxError("Syntax error: Invalid Stmt");
            }
        } else {
            throw SyntaxError("Syntax error: Expected TokenType Name for statement");
        }
    }
}

void SyntacticValidator::validateReadPrintCall() {
    popToken();// terminals validated by validateStmtLst()
    validateName();
    validateSemicolon();
}

void SyntacticValidator::validateAssign() {
    validateName();
    validateEquals();
    validateExpr();
    validateSemicolon();
}

void SyntacticValidator::validateExpr() {
    SPToken currToken = peekToken();
    SPToken nextToken = peekNextToken();

    // Recursively check for the next valid term or arithmetic operator til reach end of exp
    validateTerm();
    if (peekNextToken().getType() != TokenType::Semicolon && peekToken().getType() == TokenType::ArithmeticOperator) {
        validateArithmeticOperator();
        validateExpr();
    }
}

void SyntacticValidator::validateTerm() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::Name) {
        validateName();
    } else if (currToken.getType() == TokenType::Integer) {
        validateInteger();
    } else if (currToken.getType() == TokenType::OpenRoundParenthesis) {
        validateOpenRoundParen();
        validateExpr();
        validateCloseRoundParen();
    } else {
        throw SyntaxError("Syntax error: Invalid Term");
    }
}

void SyntacticValidator::validateWhile() {
    popToken();// 'while' terminal validated by validateStmtLst()

    // Read all tokens until "{"
    std::vector<SPToken> expression;
    while (peekToken().getType() != TokenType::OpenCurlyParenthesis) { expression.push_back(popToken()); }

    // Pass expression '(' conditional expr ')' into validator
    ConditionalValidator conditionalValidator(expression);
    conditionalValidator.validate();

    validateOpenCurlyParen();
    validateStmtLst();
    validateCloseCurlyParen();
}

void SyntacticValidator::validateIf() {
    popToken();// 'if' terminal validated by validateStmtLst()

    // Read all tokens until "then {"
    std::vector<SPToken> expression;
    while (peekNextToken().getType() != TokenType::OpenCurlyParenthesis) { expression.push_back(popToken()); }

    // Pass expression '(' conditional expr ')' into validator
    ConditionalValidator conditionalValidator(expression);
    conditionalValidator.validate();

    validateThen();
    validateOpenCurlyParen();
    validateStmtLst();
    validateCloseCurlyParen();

    validateElse();
    validateOpenCurlyParen();
    validateStmtLst();
    validateCloseCurlyParen();
}

void SyntacticValidator::validateThen() {
    if (peekToken().getType() == TokenType::Name && peekToken().getValue() == AppConstants::STRING_THEN) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected 'then' in if statement");
    }
}

void SyntacticValidator::validateElse() {
    if (peekToken().getType() == TokenType::Name && peekToken().getValue() == AppConstants::STRING_ELSE) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected 'else' in if statement");
    }
}

void SyntacticValidator::validateName() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::Name) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType Name");
    }
}

void SyntacticValidator::validateInteger() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::Integer) {
        if (currToken.getValue()[0] == '0' && currToken.getValue().size() != 1) {
            throw SyntaxError("Syntax error: Integer cannot start with 0");
        }
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType Integer");
    }
}

void SyntacticValidator::validateOpenRoundParen() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::OpenRoundParenthesis) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType OpenRoundParenthesis");
    }
}

void SyntacticValidator::validateCloseRoundParen() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::CloseRoundParenthesis) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType CloseRoundParenthesis");
    }
}

void SyntacticValidator::validateOpenCurlyParen() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::OpenCurlyParenthesis) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType OpenCurlyParenthesis");
    }
}

void SyntacticValidator::validateCloseCurlyParen() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::CloseCurlyParenthesis) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType CloseCurlyParenthesis");
    }
}

void SyntacticValidator::validateSemicolon() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::Semicolon) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType Semicolon");
    }
}

void SyntacticValidator::validateEquals() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::Equals) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType Equals");
    }
}

void SyntacticValidator::validateArithmeticOperator() {
    SPToken currToken = peekToken();
    if (currToken.getType() == TokenType::ArithmeticOperator) {
        popToken();
    } else {
        throw SyntaxError("Syntax error: Expected TokenType ArithmeticOperator");
    }
}

SPToken SyntacticValidator::peekToken() {
    if (isCurrValid()) {
        return tokens[curr];
    } else {
        throw std::out_of_range("Error: attempted to access out-of-range char in input file");
    }
}

SPToken SyntacticValidator::peekNextToken() {
    if (!isCurrValid()) {
        return tokens[curr + 1];
    } else {
        throw std::out_of_range("Error: attempted to access out-of-range char in input file");
    }
}

SPToken SyntacticValidator::popToken() {
    SPToken res = peekToken();
    curr += 1;
    return res;
}

bool SyntacticValidator::isCurrValid() { return curr >= 0 && curr < (int) tokens.size(); }