#pragma once

#include "QueryEntity.h"
#include "Ref.h"

/*
 * @brief QPSUtil class
 */
class QPSUtil {
public:
    /*
     * @brief Checks if the entityType is of stmt type
     * @param entityType The QueryEntityType to be checked
     * @return The result of the check in boolean
     */
    static bool isOfStmtType(QueryEntityType entityType);

    /*
     * @brief Checks if the rootType matches the pattern of entRef
     * @param rootType The RootType to be checked
     * @return The result of the check in boolean
     */
    static bool isRootOfEntref(RootType rootType);

    /*
     * @brief Checks if the rootType matches the pattern of stmtRef
     * @param rootType The RootType to be checked
     * @return The result of the check in boolean
     */
    static bool isRootOfStmtref(RootType rootType);
};