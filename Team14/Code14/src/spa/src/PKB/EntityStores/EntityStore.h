#pragma once

#include <unordered_set>
#include <vector>
#include <memory>

#include "../../Commons/Entities/Entity.h"

/*!
 * @brief A class that stores all the entities in the SIMPLE source program
 * @details This class is a template class that takes in a type T that is a subclass of Entity.
 * This class is a super class of all the entity stores.
 * @tparam T The type of the Entity that the EntityStore stores
 */
template <typename T>
class EntityStore {
private:
    /**
     * @brief The unordered_set that stores all the entities
     */
    std::unordered_set<
        std::shared_ptr<T>,
        std::hash<std::shared_ptr<Entity>>,
        std::equal_to<std::shared_ptr<Entity>>
    > entityStore;

public:
    /**
     * @brief Construct a new EntityStore object
     * @return A new EntityStore object
     */
    EntityStore();

    /**
     * @brief Adds an Entity object to the store
     * @param entity The Entity object to be added
     * @return None
     */
    void storeEntity(std::shared_ptr<T> entity);

    /**
     * @brief Retrieve the start iterator for the EntityStore
     * @return The start iterator for the EntityStore
     */
    typename std::unordered_set<std::shared_ptr<T>>::iterator getBeginIterator();

    /**
     * @brief Retrieve the end iterator for the EntityStore
     * @return The end iterator for the EntityStore
     */
    typename std::unordered_set<std::shared_ptr<T>>::iterator getEndIterator();

    /**
     * @brief Returns the entity from the EntityStore that is equal to the entity passed in
     * @param entity The entity to be compared against
     * @return The entity from the EntityStore that is equal to the entity passed in
     */
    std::shared_ptr<Entity> getEntity(std::shared_ptr<T> entity) const;
};

#include "EntityStore.hpp"