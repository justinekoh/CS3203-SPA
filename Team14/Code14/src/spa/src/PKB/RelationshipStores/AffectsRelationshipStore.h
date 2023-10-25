#pragma once

#include "PKB/RelationshipStores/RelationshipStore.h"
#include "Commons/Entities/Statement.h"

/**
 * @brief A class that stores AffectsRelationships
 * @details A class that stores AffectsRelationships
 */
class AffectsRelationshipStore : public RelationshipStore<Statement, Statement> {
public:
    /**
     * @brief Construct a new AffectsRelationshipStore object
     * @return A new AffectsRelationshipStore object
     */
    AffectsRelationshipStore();
};