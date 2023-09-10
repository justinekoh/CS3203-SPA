#pragma once

#include "Entity.h"
#include "Ref.h"
#include <string>

enum class RelationshipType {
    Uses,
    Follows,
    FollowsStar
};

class SuchThatClause {
private:
    RelationshipType type;
    Ref leftRef;
    Ref rightRef;

public:
    SuchThatClause();
    void setType(RelationshipType type);
    RelationshipType getType();
    void setLeftRef(Ref& ref);
    void setRightRef(Ref& ref);
    Ref getLeftRef();
    Ref getRightRef();

};
