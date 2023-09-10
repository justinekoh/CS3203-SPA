#pragma once

#include "UsesRelationshipManager.h"

UsesRelationshipManager::UsesRelationshipManager(std::shared_ptr<UsesRelationshipStore> usesRelationshipStore) {
    this->usesRelationshipStore = usesRelationshipStore;
}

std::shared_ptr<std::vector<std::shared_ptr<std::pair<std::shared_ptr<Entity>, std::shared_ptr<Entity>>>>> UsesRelationshipManager::getAllAssignVariable() const {
    auto pairs = std::make_shared<std::vector<std::shared_ptr<std::pair<std::shared_ptr<Entity>, std::shared_ptr<Entity>>>>>();
    for (auto it = usesRelationshipStore->getBeginIterator(); it != usesRelationshipStore->getEndIterator(); it++) {
        if (std::dynamic_pointer_cast<Statement>((*it)->getLeftEntity())->getStatementType() == StatementType::Assign) {
            pairs->push_back(std::make_shared<std::pair<std::shared_ptr<Entity>, std::shared_ptr<Entity>>>(
                    std::make_pair((*it)->getLeftEntity(), (*it)->getRightEntity())));
        }
    }
    return pairs;
}

std::shared_ptr<std::vector<std::shared_ptr<std::pair<std::shared_ptr<Entity>, std::shared_ptr<Entity>>>>> UsesRelationshipManager::getVariableAssignment(std::shared_ptr<Variable> variable) const {
    auto pairs = std::make_shared<std::vector<std::shared_ptr<std::pair<std::shared_ptr<Entity>, std::shared_ptr<Entity>>>>>();
    for (auto it = usesRelationshipStore->getBeginIterator(); it != usesRelationshipStore->getEndIterator(); it++) {
        if ((*it)->getRightEntity() == variable && std::dynamic_pointer_cast<Statement>((*it)->getLeftEntity())->getStatementType() == StatementType::Assign) {
            pairs->push_back(std::make_shared<std::pair<std::shared_ptr<Entity>, std::shared_ptr<Entity>>>(
                    std::make_pair((*it)->getLeftEntity(), (*it)->getRightEntity())));
        }
    }
    return pairs;
}