#include "Statement.h"

Statement::Statement(StatementNumber lineNo, StatementType statementType)
	: Entity(std::make_shared<EntityValue>(std::to_string(lineNo)), EntityType::Statement) {
    this->statementType = statementType;
}

StatementType Statement::getStatementType() const {
    return this->statementType;
}

bool Statement::isStatementType(StatementType statementType) const {
    return this->statementType == statementType || statementType == StatementType::Stmt;
}