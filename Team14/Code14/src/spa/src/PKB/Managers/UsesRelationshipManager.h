#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "PKB/RelationshipStorage/UsesRelationshipStore.h"
#include "PKB/Managers/AssignmentManager.h"
#include "Commons/Entities/Statement.h"
#include "Commons/Entities/Variable.h"

class UsesRelationshipManager {
private:
    std::shared_ptr<UsesRelationshipStore> usesRelationshipStore;
public:
    UsesRelationshipManager(std::shared_ptr<UsesRelationshipStore> usesRelationshipStore);

    std::shared_ptr<std::vector<std::shared_ptr<std::vector<std::shared_ptr<Entity>>>>> getAllAssignVariable() const;

    std::shared_ptr<std::vector<std::shared_ptr<Entity>>> getVariableAssignment(std::shared_ptr<Variable> variable) const;
};
