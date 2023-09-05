#pragma once

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
#include "Relationship.h"


Relationship::Relationship(int relationshipType, int leftEntity, int rightEntity) {
	this->relationshipType = relationshipType;
	this->leftEntity = leftEntity;
	this->rightEntity = rightEntity;
}
int Relationship::getRelationshipType() {
	return relationshipType;
}

int Relationship::getLeftEntity() {
	return leftEntity;
}

int Relationship::getRightEntity() {
	return rightEntity;
}