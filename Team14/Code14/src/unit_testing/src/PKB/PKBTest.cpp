#include "catch.hpp"

#include "PKB/Relationship.h"
#include <iostream>

using namespace std;

void require1(bool b) {
	REQUIRE(b);
}

TEST_CASE("Test Relationship Constructor") {
	int mockRelationshipType = 1;
	int mockLeftEntity = 2;
	int mockRightEntity = 3;

	Relationship rs = Relationship(mockRelationshipType, mockLeftEntity, mockRightEntity);

	require1(rs.getRelationshipType() == mockRelationshipType);
	require1(rs.getLeftEntity() == mockLeftEntity);
	require1(rs.getRightEntity() == mockRightEntity);
}


