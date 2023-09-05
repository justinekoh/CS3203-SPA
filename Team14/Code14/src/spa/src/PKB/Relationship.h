#pragma once

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Relationship {
private:
	int relationshipType;
	int leftEntity;
	int rightEntity;

public:
	Relationship(int relationshipType, int leftEntity, int rightEntity);
	int getRelationshipType();
	int getLeftEntity();
	int getRightEntity();

};