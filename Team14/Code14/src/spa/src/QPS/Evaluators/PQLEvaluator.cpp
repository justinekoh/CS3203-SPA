#include <numeric>
#include <queue>

#include "PQLEvaluator.h"
#include "QPS/QPSTypes.h"
#include "QPS/QPSUtil.h"
#include "QPS/QueryEntity.h"
#include "QPSOptimizer.h"

PQLEvaluator::PQLEvaluator(std::shared_ptr<PkbReader> pkbReader)
    : pkbReader(pkbReader), clauseHandler(std::make_shared<ClauseHandler>(pkbReader)) {}


ResultList PQLEvaluator::formatResult(Query &query, Result &result) {
    std::vector<Synonym> selects = query.getSelect();

    if (selects.empty()) {// case BOOLEAN query
        if (result.isFalse() || result.isEmpty()) { return ResultList{"FALSE"}; }
        return ResultList{"TRUE"};
    }

    // case tuple query
    SynonymMap indicesMap = result.getSynIndices();
    ResultSet results;
    auto transformations = getTransformations(indicesMap, selects);
    for (auto &tuple: result.getTuples()) {
        auto tmp = project(tuple, transformations);
        auto formattedResult = concat(tmp);
        if (!formattedResult.empty()) { results.insert(formattedResult); }
    }
    ResultList list_results(results.begin(), results.end());
    return list_results;
}

std::vector<std::pair<int, transformFunc>> PQLEvaluator::getTransformations(SynonymMap inputMap,
                                                                            std::vector<Synonym> resultClause) {
    std::vector<std::pair<int, transformFunc>> transformations;
    for (Synonym &elem: resultClause) {
        auto attrName = QPSUtil::getAttrName(elem);
        std::pair<int, transformFunc> transformation;
        if (attrName.empty()) {// case synonym
            transformation.first = inputMap[elem];
            transformation.second = [](auto ent) { return ent->getEntityValue(); };
        } else {                             // case attrRef
            auto syn = QPSUtil::getSyn(elem);// get Syn without attrName
            transformation.first = inputMap[syn];
            transformation.second = QPSUtil::getValueFunc[QPSUtil::strToAttrNameMap[attrName]];
        }
        transformations.push_back(transformation);
    }
    return transformations;
}

std::vector<std::string> PQLEvaluator::project(ResultTuple row,
                                               std::vector<std::pair<int, transformFunc>> &transformations) {
    std::vector<std::string> projection;
    for (auto &elem: transformations) {
        auto result = elem.second(row[elem.first]);
        projection.push_back(result);
    }
    return projection;
}

std::string PQLEvaluator::concat(std::vector<std::string> strings) {
    auto result = std::accumulate(strings.begin(), strings.end(), std::string(),
                                  [](std::string &a, const std::string &b) { return a += (a.empty() ? "" : " ") + b; });
    return result;
}

bool PQLEvaluator::evaluateBooleanGroup(const std::vector<std::shared_ptr<Clause>> &clauses) {
    for (auto &clause: clauses) {
        auto res = evaluateClause(clause);
        if (clause->isNegation()) { res->setBoolResult(!res->getBoolResult()); }
        if (res->isFalse()) { return false; }
    }
    return true;
}

std::shared_ptr<Result> PQLEvaluator::evaluateTupleGroup(std::vector<std::shared_ptr<Clause>> &clauses) {
    clauses = QPSOptimizer::sortClauses(clauses);
    auto groupRes = std::make_shared<Result>(true);
    for (auto &clause: clauses) {
        auto clauseRes = evaluateClause(clause);
        if (clause->isNegation()) {
            groupRes = evaluateNegation(groupRes, clauseRes);
        } else {
            groupRes = ResultHandler::getCombined(groupRes, clauseRes);
        }
        if (groupRes->isFalse()) { return groupRes; }// terminate early if intermediate result is empty
    }
    return groupRes;
}

Result PQLEvaluator::evaluate(Query &query) {
    setDeclarationMap(query);
    auto pairs = QPSOptimizer::getGroupScorePairs(query);
    std::priority_queue pq(pairs.begin(), pairs.end(), QPSOptimizer::compareGroupByScore);

    auto res = std::make_shared<Result>(true);
    while (!pq.empty()) {
        auto pair = pq.top();
        pq.pop();
        std::vector<std::shared_ptr<Clause>> group(pair.first.begin(), pair.first.end());
        if (!std::get<1>(pair.second)) {// no synonyms
            if (!evaluateBooleanGroup(group)) { return Result(false); }
        } else if (!std::get<0>(pair.second)) {// group with irrelevant synonyms
            auto groupRes = evaluateTupleGroup(group);
            if (groupRes->isFalse()) { return *groupRes; }
        } else {// those with selectSyns (and if select has synonym(s)
            auto groupRes = evaluateTupleGroup(group);
            if (groupRes->isFalse()) { return *groupRes; }
            res = ResultHandler::getCombined(res, groupRes);
        }
    }

    //  CASE RESULT-CLAUSE IN RESULT TABLE, check if ALL synonym in select is in result table
    auto selects = query.getSelect();
    std::vector<Synonym> unevaluatedSyn = getUnevaluatedSyn(selects, res);
    if (unevaluatedSyn.empty()) { return *res; }

    // CASE SOME RESULT-CLAUSE NOT IN clauses
    auto synResult = evaluateAll(unevaluatedSyn);
    auto finalResult = ResultHandler::getCombined(res, synResult);
    return *finalResult;
}

std::vector<Synonym> PQLEvaluator::getUnevaluatedSyn(const std::vector<Synonym> resultClause,
                                                     std::shared_ptr<Result> result) {
    auto synMap = result->getSynIndices();
    std::vector<Synonym> unevaluated;
    for (auto &elem: resultClause) {
        auto syn = QPSUtil::getSyn(elem);
        if (!synMap.count(syn)) { unevaluated.push_back(syn); }
    }
    return unevaluated;
}

std::shared_ptr<Result> PQLEvaluator::evaluateClause(const std::shared_ptr<Clause> clause) {
    std::shared_ptr<Strategy> strategy = QPSUtil::strategyCreatorMap[clause->getType()](pkbReader);
    clauseHandler->setStrategy(strategy);
    std::shared_ptr<Result> result = clauseHandler->executeClause(clause);
    return result;
}

std::shared_ptr<Result> PQLEvaluator::evaluateNegation(std::shared_ptr<Result> curr,
                                                       std::shared_ptr<Result> clauseRes) {
    auto [evaluatedSyns, unevaluatedSyns] = groupSynByEvaluated(curr, clauseRes);

    if (unevaluatedSyns.empty()) {// all syns present
        return ResultHandler::getDiff(curr, clauseRes);
    }

    if (evaluatedSyns.empty()) {// all syns unevaluated, take naive approach
        auto lhs = evaluateAll(unevaluatedSyns);
        auto negatedRes = ResultHandler::getDiff(lhs, clauseRes);
        return ResultHandler::getCombined(curr, negatedRes);
    }

    // syns partially evaluated, evaluate result of negated clause
    auto appendSet = getAll(declarationMap[unevaluatedSyns[0]]);// get column to append
    auto currTuples = curr->getTuples();
    auto filter = clauseRes->getTuples();

    // get negation result
    std::unordered_set<EntityPointer> found;// track which entities has been added to filtered
    ResultTuples filtered;
    std::pair<idx, idx> commonIdx =
            std::make_pair(curr->getSynIndices()[evaluatedSyns[0]], clauseRes->getSynIndices()[evaluatedSyns[0]]);
    for (const auto &row: currTuples) {
        auto commonEntity = row[commonIdx.first];
        if (!found.count(commonEntity)) {// new common column value encountered
            found.insert(commonEntity);
            for (const auto &newEntity: appendSet) {
                ResultTuple newRow(AppConstants::PAIR_TUPLE_SIZE);
                newRow[commonIdx.second] = commonEntity;
                newRow[!commonIdx.second] = newEntity;
                if (!filter.count(newRow)) { filtered.insert(newRow); }// if row not in rhsRes, add to filteredSet
            }
        }
    }
    clauseRes->setTuples(filtered);

    return ResultHandler::getCombined(curr, clauseRes);
}

std::unordered_set<std::shared_ptr<Entity>> PQLEvaluator::getAll(const std::shared_ptr<QueryEntity> &queryEntity) {
    QueryEntityType entityType = queryEntity->getType();
    return QPSUtil::entityGetterMap[entityType](pkbReader);
}

std::shared_ptr<Result> PQLEvaluator::evaluateAll(const std::vector<Synonym> &entitySyns) {
    auto result = std::make_shared<Result>(entitySyns);
    auto tupleSize = entitySyns.size();

    // retrieve the sets of entities and convert to vector
    std::vector<std::vector<EntityPointer>> input(tupleSize);
    for (int i = 0; i < tupleSize; i++) {
        auto set = getAll(declarationMap[entitySyns[i]]);
        if (set.empty()) { return result; }
        for (const auto entity: set) { input[i].push_back(entity); }
    }

    // permute all combinations
    std::vector<int> indices(tupleSize, 0);
    std::unordered_set<ResultTuple> resultTuples;
    while (true) {
        // add current combination of indices to result
        ResultTuple combination;
        for (int i = 0; i < tupleSize; ++i) { combination.push_back(input[i][indices[i]]); }
        resultTuples.insert(combination);

        // update indices for next combination
        int synToUpdate = tupleSize - 1;
        while (synToUpdate >= 0) {
            indices[synToUpdate]++;
            if (indices[synToUpdate] < input[synToUpdate].size()) {
                break;
            } else {
                indices[synToUpdate] = 0;
                synToUpdate--;
            }
        }

        if (synToUpdate < 0) { break; }
    }

    result->setTuples(resultTuples);
    return result;
}

void PQLEvaluator::setDeclarationMap(Query &query) { declarationMap = query.getDeclarations(); }

std::pair<std::vector<Synonym>, std::vector<Synonym>>
PQLEvaluator::groupSynByEvaluated(std::shared_ptr<Result> curr, std::shared_ptr<Result> clauseRes) {
    std::pair<std::vector<Synonym>, std::vector<Synonym>> synGroups;
    auto currSynMap = curr->getSynIndices();
    auto clauseSynMap = clauseRes->getSynIndices();
    for (auto &elem: clauseSynMap) {
        auto syn = QPSUtil::getSyn(elem.first);
        if (currSynMap.count(syn)) {
            synGroups.first.push_back(syn);
        } else {
            synGroups.second.push_back(syn);
        }
    }
    return synGroups;
}
