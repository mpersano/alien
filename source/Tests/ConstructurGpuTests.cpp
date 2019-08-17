#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/copy.hpp>

#include "Base/ServiceLocator.h"
#include "IntegrationGpuTestFramework.h"
#include "ModelBasic/CellComputerCompiler.h"
#include "ModelBasic/QuantityConverter.h"

class ConstructorGpuTests : public IntegrationGpuTestFramework
{
public:
    ConstructorGpuTests()
        : IntegrationGpuTestFramework()
    {}

    virtual ~ConstructorGpuTests() = default;

protected:
    virtual void SetUp();

    enum class WithSeparation
    {
        No,
        Yes
    };
    float getOffspringDistance(WithSeparation value = WithSeparation::No) const;
    QVector2D constructorPositionForHorizontalClusterAfterCreation(
        vector<QVector2D> constructionSite,
        QVector2D constructor,
        vector<QVector2D> remainingCells,
        float distanceBetweenOffspringToConstructionSite,
        WithSeparation withSeparation) const;

    struct TokenForConstructionParameters
    {
        MEMBER_DECLARATION(TokenForConstructionParameters, optional<float>, energy, boost::none);
        MEMBER_DECLARATION(
            TokenForConstructionParameters,
            Enums::ConstrIn::Type,
            constructionInput,
            Enums::ConstrIn::DO_NOTHING);
        MEMBER_DECLARATION(
            TokenForConstructionParameters,
            Enums::ConstrInOption::Type,
            constructionOption,
            Enums::ConstrInOption::STANDARD);
        MEMBER_DECLARATION(
            TokenForConstructionParameters,
            Enums::CellFunction::Type,
            cellFunctionType,
            Enums::CellFunction::COMPUTER);
        MEMBER_DECLARATION(TokenForConstructionParameters, int, cellBranchNumber, 0);
        MEMBER_DECLARATION(TokenForConstructionParameters, int, maxConnections, 0);
        MEMBER_DECLARATION(TokenForConstructionParameters, QByteArray, staticData, QByteArray());
        MEMBER_DECLARATION(TokenForConstructionParameters, QByteArray, mutableData, QByteArray());
        MEMBER_DECLARATION(TokenForConstructionParameters, float, angle, 0.0f);
        MEMBER_DECLARATION(TokenForConstructionParameters, float, distance, 1.0f);
    };
    TokenDescription createTokenForConstruction(TokenForConstructionParameters tokenParameters) const;

    struct TestResult
    {
        QVector2D movementOfCenter;
        int increaseNumberOfCells;
        
        TokenDescription origToken;
        CellDescription origSourceCell;
        CellDescription origConstructorCell;
        vector<CellDescription> origConstructor;
        vector<CellDescription> origConstructionSite;

        TokenDescription token;
        optional<CellDescription> sourceCell;  //possibly be destroyed
        CellDescription constructorCell;
        vector<CellDescription> constructionSite;

        optional<CellDescription> getConstructedCell() const;
        optional<CellDescription> getFirstCellOfOrigConstructionSite() const;
        optional<CellDescription> getCellOfConstructionSite(uint64_t id) const;
    };
    struct StartConstructionOnHorizontalClusterTestParameters
    {
        MEMBER_DECLARATION(
            StartConstructionOnHorizontalClusterTestParameters,
            optional<float>,
            horizontalObstacleAt,
            boost::none);
        MEMBER_DECLARATION(
            StartConstructionOnHorizontalClusterTestParameters,
            TokenDescription,
            token,
            TokenDescription());
    };
    TestResult runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters const& parameters) const;
    TestResult
    runStartConstructionOnWedgeClusterTest(TokenDescription const& token, float wedgeAngle, float clusterAngle) const;
    TestResult runStartConstructionOnTriangleClusterTest(TokenDescription const& token) const;

    struct ContinueConstructionOnHorizontalClusterTestParameters
    {
        MEMBER_DECLARATION(
            ContinueConstructionOnHorizontalClusterTestParameters,
            TokenDescription,
            token,
            TokenDescription());
        MEMBER_DECLARATION(
            ContinueConstructionOnHorizontalClusterTestParameters,
            optional<float>,
            horizontalObstacleAt,
            boost::none);
    };
    TestResult runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters const& parameters) const;
    TestResult
        runContinueConstructionOnSelfTouchingClusterTest(TokenDescription const& token, int cellLength) const;

    struct Expectations
    {
        MEMBER_DECLARATION(Expectations, Enums::ConstrOut::Type, tokenOutput, Enums::ConstrOut::SUCCESS);
        MEMBER_DECLARATION(Expectations, optional<QVector2D>, relPosOfFirstCellOfConstructionSite, boost::none);
        MEMBER_DECLARATION(Expectations, optional<TokenDescription>, constructedToken, boost::none);
        MEMBER_DECLARATION(Expectations, bool, destruction, false);
    };

    class _ResultChecker
    {
    public:
        _ResultChecker(SimulationParameters const& parameters)
            : _parameters(parameters)
        {}

        void check(TestResult const& testResult, Expectations const& expectations) const;

    private:
        void checkIfDestruction(TestResult const& testResult, Expectations const& expectations) const;
        void checkIfNoDestruction(TestResult const& testResult, Expectations const& expectations) const;
        void checkIfNoDestructionAndSuccess(TestResult const& testResult, Expectations const& expectations) const;

        void checkCellPosition(TestResult const& testResult, Expectations const& expectations) const;
        void checkCellAttributes(TokenDescription const& token, CellDescription const& cell) const;
        void checkCellConnections(TestResult const& testResult) const;
        void checkConstructedToken(TestResult const& testResult, Expectations const& expectations) const;

    private:
        SimulationParameters _parameters;
    };
    using ResultChecker = boost::shared_ptr<_ResultChecker>;

    static bool isFinished(TokenDescription const& token);
    static bool isReduced(TokenDescription const& token);
    static bool isSeparated(TokenDescription const& token);

protected:
    ResultChecker _resultChecker;
};


/************************************************************************/
/* Implementation                                                       */
/************************************************************************/

void ConstructorGpuTests::SetUp()
{
    _parameters.radiationProb = 0;  //exclude radiation
    _parameters.cellFunctionConstructorOffspringCellDistance = 1;
    _context->setSimulationParameters(_parameters);

    _resultChecker = boost::make_shared<_ResultChecker>(_parameters);
}

auto ConstructorGpuTests::runStartConstructionOnHorizontalClusterTest(
    StartConstructionOnHorizontalClusterTestParameters const& parameters) const -> TestResult
{
    DataDescription origData;
    auto cluster = createHorizontalCluster(2, QVector2D{10.5, 10.5}, QVector2D{}, 0);

    auto& firstCell = cluster.cells->at(0);
    firstCell.tokenBranchNumber = 0;
    firstCell.addToken(parameters._token);

    auto& secondCell = cluster.cells->at(1);
    secondCell.tokenBranchNumber = 1;
    secondCell.cellFeature = CellFeatureDescription().setType(Enums::CellFunction::CONSTRUCTOR);

    origData.addCluster(cluster);

    std::unordered_set<uint64_t> obstacleCellIds;
    if (parameters._horizontalObstacleAt) {

        //following calculation only works for 0-angle
        CHECK(0 == parameters._token.data->at(Enums::Constr::INOUT_ANGLE));

        auto const withSeparation = isSeparated(parameters._token);

        auto const estimatedConstructorAbsPos = constructorPositionForHorizontalClusterAfterCreation(
            {}, {11, 10.5}, {{10, 10.5}}, 0, withSeparation ? WithSeparation::Yes : WithSeparation::No);
        auto const obstacleCellAbsPos = estimatedConstructorAbsPos + QVector2D{*parameters._horizontalObstacleAt, 0};

        QVector2D obstacleCenterPos;
        if (*parameters._horizontalObstacleAt > 0) {
            obstacleCenterPos = obstacleCellAbsPos + QVector2D{1.5f + _parameters.cellMinDistance / 2, 0};
        } else {
            obstacleCenterPos = obstacleCellAbsPos - QVector2D{1.5f + _parameters.cellMinDistance / 2, 0};
        }
        auto obstacle = createHorizontalCluster(4, obstacleCenterPos, QVector2D{}, 0);
        origData.addCluster(obstacle);
        for (auto const& cell : *obstacle.cells) {
            obstacleCellIds.insert(cell.id);
        }
    }

    IntegrationTestHelper::updateData(_access, origData);

    //perform test
    IntegrationTestHelper::runSimulation(1, _controller);

    //check results
    DataDescription newData = IntegrationTestHelper::getContent(_access, {{0, 0}, {_universeSize.x, _universeSize.y}});

    checkEnergy(origData, newData);

    auto newCellByCellId = IntegrationTestHelper::getCellByCellId(newData);

    std::unordered_map<uint64_t, CellDescription> newCellsWithoutObstacleByCellId;
    for (auto const& newCell : newCellByCellId | boost::adaptors::map_values) {
        if (obstacleCellIds.find(newCell.id) == obstacleCellIds.end()) {
            newCellsWithoutObstacleByCellId.insert_or_assign(newCell.id, newCell);
        }
    }

    QVector2D newCenter;
    for (auto const& newCell : newCellsWithoutObstacleByCellId | boost::adaptors::map_values) {
        newCenter += *newCell.pos;
    }
    newCenter /= newCellsWithoutObstacleByCellId.size();

    TestResult result;
    result.movementOfCenter = newCenter - *cluster.pos;
    result.increaseNumberOfCells = newCellsWithoutObstacleByCellId.size() - cluster.cells->size();

    auto const& newSecondCell = newCellByCellId.at(secondCell.id);
    auto const& newToken = newSecondCell.tokens->at(0);

    result.origToken = parameters._token;
    result.token = newToken;
    result.origSourceCell = firstCell;
    if (newCellByCellId.find(firstCell.id) != newCellByCellId.end()) {
        result.sourceCell = newCellByCellId.at(firstCell.id);
    }
    result.origConstructorCell = secondCell;
    result.constructorCell = newSecondCell;
    result.origConstructor = *cluster.cells;

    std::list<CellDescription> remainingCells;
    for (auto const& newCell : newCellByCellId | boost::adaptors::map_values) {
        if (newCell.id != firstCell.id && newCell.id != secondCell.id
            && obstacleCellIds.find(newCell.id) == obstacleCellIds.end()) {
            remainingCells.push_back(newCell);
        }
    }
    EXPECT_GE(1, remainingCells.size());

    if (!remainingCells.empty()) {
        result.constructionSite.emplace_back(*remainingCells.begin());
    }

    return result;
}

auto ConstructorGpuTests::runStartConstructionOnWedgeClusterTest(
    TokenDescription const& token,
    float wedgeAngle,
    float clusterAngle) const -> TestResult
{
    ClusterDescription cluster;
    cluster.setId(_numberGen->getId()).setVel(QVector2D{}).setAngle(0).setAngularVel(0);

    auto const center = QVector2D{10.5f, 10.5f};
    auto const cellEnergy = _parameters.cellFunctionConstructorOffspringCellEnergy;
    auto const relPos1 = Physics::unitVectorOfAngle(clusterAngle + 270 + wedgeAngle / 2);
    auto const relPos2 = QVector2D{0, 0};
    auto const relPos3 = Physics::unitVectorOfAngle(clusterAngle + 270 - wedgeAngle / 2);
    auto const cellId1 = _numberGen->getId();
    auto const cellId2 = _numberGen->getId();
    auto const cellId3 = _numberGen->getId();
    cluster.addCells({CellDescription()
                          .setEnergy(cellEnergy)
                          .setPos(center + relPos1)
                          .setMaxConnections(1)
                          .setConnectingCells({cellId2})
                          .setTokenBranchNumber(0)
                          .setId(cellId1)
                          .setCellFeature(CellFeatureDescription())
                          .addToken(token),
                      CellDescription()
                          .setEnergy(cellEnergy)
                          .setPos(center + relPos2)
                          .setMaxConnections(2)
                          .setConnectingCells({cellId1, cellId3})
                          .setTokenBranchNumber(1)
                          .setId(cellId2)
                          .setCellFeature(CellFeatureDescription().setType(Enums::CellFunction::CONSTRUCTOR)),
                      CellDescription()
                          .setEnergy(cellEnergy)
                          .setPos(center + relPos3)
                          .setMaxConnections(1)
                          .setConnectingCells({cellId2})
                          .setTokenBranchNumber(2)
                          .setId(cellId3)
                          .setCellFeature(CellFeatureDescription())});
    auto const& cell1 = cluster.cells->at(0);
    auto const& cell2 = cluster.cells->at(1);

    cluster.setPos(cluster.getClusterPosFromCells());

    DataDescription origData;
    origData.addCluster(cluster);

    IntegrationTestHelper::updateData(_access, origData);

    //perform test
    IntegrationTestHelper::runSimulation(1, _controller);

    //check results
    DataDescription newData = IntegrationTestHelper::getContent(_access, {{0, 0}, {_universeSize.x, _universeSize.y}});
    checkEnergy(origData, newData);

    auto const& newCluster = newData.clusters->at(0);

    TestResult result;
    result.movementOfCenter = *newCluster.pos - *cluster.pos;
    result.increaseNumberOfCells = newCluster.cells->size() - cluster.cells->size();

    auto newCellByCellId = IntegrationTestHelper::getCellByCellId(newData);

    auto const& newCell2 = newCellByCellId.at(cellId2);
    auto const& newToken = newCell2.tokens->at(0);

    result.origToken = token;
    result.token = newToken;
    result.origSourceCell = cell1;
    if (newCellByCellId.find(cellId1) != newCellByCellId.end()) {
        result.sourceCell = newCellByCellId.at(cellId1);
    }
    result.origConstructorCell = cell2;
    result.constructorCell = newCell2;
    result.origConstructor = *cluster.cells;

    newCellByCellId.erase(cellId1);
    newCellByCellId.erase(cellId2);
    newCellByCellId.erase(cellId3);
    if (!newCellByCellId.empty()) {
        result.constructionSite.emplace_back(newCellByCellId.begin()->second);
    }

    return result;
}

auto ConstructorGpuTests::runStartConstructionOnTriangleClusterTest(TokenDescription const& token) const -> TestResult
{
    ClusterDescription cluster;
    cluster.setId(_numberGen->getId()).setVel(QVector2D{}).setAngle(0).setAngularVel(0);

    auto const center = QVector2D{10, 10};
    auto const cellEnergy = _parameters.cellFunctionConstructorOffspringCellEnergy;
    auto const relPos1 = QVector2D{0, -1};
    auto const relPos2 = QVector2D{-1, 0};
    auto const relPos3 = QVector2D{0, 1};
    auto const relPos4 = QVector2D{0, 0};
    auto const cellId1 = _numberGen->getId();
    auto const cellId2 = _numberGen->getId();
    auto const cellId3 = _numberGen->getId();
    auto const cellId4 = _numberGen->getId();
    cluster.addCells({CellDescription()
                          .setEnergy(cellEnergy)
                          .setPos(center + relPos1)
                          .setMaxConnections(1)
                          .setConnectingCells({cellId4})
                          .setTokenBranchNumber(0)
                          .setId(cellId1)
                          .setCellFeature(CellFeatureDescription())
                          .addToken(token),
                      CellDescription()
                          .setEnergy(cellEnergy)
                          .setPos(center + relPos2)
                          .setMaxConnections(1)
                          .setConnectingCells({cellId4})
                          .setTokenBranchNumber(0)
                          .setId(cellId2)
                          .setCellFeature(CellFeatureDescription()),
                      CellDescription()
                          .setEnergy(cellEnergy)
                          .setPos(center + relPos3)
                          .setMaxConnections(1)
                          .setConnectingCells({cellId4})
                          .setTokenBranchNumber(0)
                          .setId(cellId3)
                          .setCellFeature(CellFeatureDescription()),
                      CellDescription()
                          .setEnergy(cellEnergy)
                          .setPos(center + relPos4)
                          .setMaxConnections(3)
                          .setConnectingCells({cellId1, cellId2, cellId3})
                          .setTokenBranchNumber(1)
                          .setId(cellId4)
                          .setCellFeature(CellFeatureDescription().setType(Enums::CellFunction::CONSTRUCTOR))});
    auto const& cell1 = cluster.cells->at(0);
    auto const& cell4 = cluster.cells->at(3);

    cluster.setPos(cluster.getClusterPosFromCells());

    DataDescription origData;
    origData.addCluster(cluster);

    IntegrationTestHelper::updateData(_access, origData);

    //perform test
    IntegrationTestHelper::runSimulation(1, _controller);

    //check results
    DataDescription newData = IntegrationTestHelper::getContent(_access, {{0, 0}, {_universeSize.x, _universeSize.y}});
    checkEnergy(origData, newData);

    EXPECT_EQ(1, newData.clusters->size());
    auto const& newCluster = newData.clusters->at(0);

    TestResult result;
    result.movementOfCenter = *newCluster.pos - *cluster.pos;
    result.increaseNumberOfCells = newCluster.cells->size() - cluster.cells->size();

    auto newCellByCellId = IntegrationTestHelper::getCellByCellId(newData);

    auto const& newCell4 = newCellByCellId.at(cellId4);
    auto const& newToken = newCell4.tokens->at(0);

    result.origToken = token;
    result.token = newToken;
    result.origSourceCell = cell1;
    if (newCellByCellId.find(cellId1) != newCellByCellId.end()) {
        result.sourceCell = newCellByCellId.at(cellId1);
    }
    result.origConstructorCell = cell4;
    result.constructorCell = newCell4;
    result.origConstructor = *cluster.cells;

    newCellByCellId.erase(cellId1);
    newCellByCellId.erase(cellId2);
    newCellByCellId.erase(cellId3);
    newCellByCellId.erase(cellId4);
    if (!newCellByCellId.empty()) {
        result.constructionSite.emplace_back(newCellByCellId.begin()->second);
    }

    return result;
}

auto ConstructorGpuTests::runContinueConstructionOnHorizontalClusterTest(
    ContinueConstructionOnHorizontalClusterTestParameters const& parameters) const -> TestResult
{
    auto cluster = createHorizontalCluster(3, QVector2D{10.5, 10.5}, QVector2D{}, 0);

    auto& cell1 = cluster.cells->at(0);
    cell1.tokenBranchNumber = 0;
    cell1.addToken(parameters._token);

    auto& cell2 = cluster.cells->at(1);
    cell2.tokenBranchNumber = 1;
    cell2.cellFeature = CellFeatureDescription().setType(Enums::CellFunction::CONSTRUCTOR);

    auto& cell3 = cluster.cells->at(2);
    cell3.tokenBlocked = true;

    DataDescription origData;
    origData.addCluster(cluster);

    std::unordered_set<uint64_t> obstacleCellIds;
    if (parameters._horizontalObstacleAt) {

        //following calculation only works for 0-angle
        CHECK(0 == parameters._token.data->at(Enums::Constr::INOUT_ANGLE));

        auto const withSeparation = isSeparated(parameters._token);
        auto const distance =
            QuantityConverter::convertDataToDistance(parameters._token.data->at(Enums::Constr::IN_DIST));
        auto const estimatedConstructorAbsPos = constructorPositionForHorizontalClusterAfterCreation(
            {{11.5, 10.5}},
            {10.5, 10.5},
            {{9.5, 10.5}},
            distance,
            withSeparation ? WithSeparation::Yes : WithSeparation::No);
        auto const obstacleCellAbsPos = estimatedConstructorAbsPos + QVector2D{ *parameters._horizontalObstacleAt, 0 };

        QVector2D obstacleCenterPos;
        if (*parameters._horizontalObstacleAt > 0) {
            obstacleCenterPos = obstacleCellAbsPos + QVector2D{ 1.5f + _parameters.cellMinDistance / 2, 0 };
        }
        else {
            obstacleCenterPos = obstacleCellAbsPos - QVector2D{ 1.5f + _parameters.cellMinDistance / 2, 0 };
        }
        auto obstacle = createHorizontalCluster(4, obstacleCenterPos, QVector2D{}, 0);
        origData.addCluster(obstacle);
        for (auto const& cell : *obstacle.cells) {
            obstacleCellIds.insert(cell.id);
        }
    }

    IntegrationTestHelper::updateData(_access, origData);

    //perform test
    IntegrationTestHelper::runSimulation(1, _controller);

    //check results
    DataDescription newData = IntegrationTestHelper::getContent(_access, {{0, 0}, {_universeSize.x, _universeSize.y}});
    checkEnergy(origData, newData);

    auto newClusterByCellId = IntegrationTestHelper::getClusterByCellId(newData);
    unordered_map<uint64_t, ClusterDescription> newClusters;
    for (auto const& cell : *cluster.cells) {
        auto const cellIdAndNewCluster = newClusterByCellId.find(cell.id);
        if (cellIdAndNewCluster != newClusterByCellId.end()) {
            auto const& newCluster = cellIdAndNewCluster->second;
            newClusters.emplace(newCluster.id, newCluster);
        }
    }
    EXPECT_EQ(1, newClusters.size());
    auto const newCluster = newClusters.begin()->second;

    TestResult result;
    result.movementOfCenter = *newCluster.pos - *cluster.pos;
    result.increaseNumberOfCells = newCluster.cells->size() - cluster.cells->size();

    auto newCellByCellId = IntegrationTestHelper::getCellByCellId(newData);
    auto const& newCell2 = newCellByCellId.at(cell2.id);
    auto const& newToken = newCell2.tokens->at(0);

    result.origToken = parameters._token;
    result.token = newToken;
    result.origSourceCell = cell1;
    if (newCellByCellId.find(cell1.id) != newCellByCellId.end()) {
        result.sourceCell = newCellByCellId.at(cell1.id);
    }
    result.origConstructorCell = cell2;
    result.origConstructionSite.emplace_back(cell3);
    result.constructorCell = newCell2;
    result.origConstructor.emplace_back(cell1);
    result.origConstructor.emplace_back(cell2);

    std::vector<CellDescription> remainingCells;
    for (auto const& newCell : newCellByCellId | boost::adaptors::map_values) {
        if (newCell.id != cell1.id && newCell.id != cell2.id
            && obstacleCellIds.find(newCell.id) == obstacleCellIds.end()) {
            remainingCells.push_back(newCell);
        }
    }
    EXPECT_GE(2, remainingCells.size());

    result.constructionSite = std::move(remainingCells);

    return result;
}

auto ConstructorGpuTests::runContinueConstructionOnSelfTouchingClusterTest(
    TokenDescription const& token,
    int cellLength) const -> TestResult
{
    ClusterDescription cluster;
    cluster.setId(_numberGen->getId()).setVel(QVector2D{}).setAngle(0).setAngularVel(0);

    auto const center = QVector2D{ 10.5f, 10.5f };
    auto const cellEnergy = _parameters.cellFunctionConstructorOffspringCellEnergy;

    vector<uint64_t> cellIds;
    for (int i = 0; i < cellLength + 4; ++i) {
        cellIds.emplace_back(_numberGen->getId());
    }
    cluster.addCells({CellDescription()  //construction site
                          .setId(cellIds[0])
                          .setConnectingCells({cellIds[1]})
                          .setEnergy(cellEnergy)
                          .setPos(center)
                          .setMaxConnections(1)
                          .setTokenBranchNumber(0)
                          .setCellFeature(CellFeatureDescription())
                          .setFlagTokenBlocked(true),
                      CellDescription()  //constructor
                          .setId(cellIds[1])
                          .setConnectingCells({cellIds[0], cellIds[2]})
                          .setEnergy(cellEnergy)
                          .setPos(center + QVector2D{-1, 0})
                          .setMaxConnections(2)
                          .setTokenBranchNumber(1)
                          .setCellFeature(CellFeatureDescription().setType(Enums::CellFunction::CONSTRUCTOR)),
                      CellDescription()
                          .setId(cellIds[2])
                          .setConnectingCells({cellIds[1], cellIds[3]})
                          .setEnergy(cellEnergy)
                          .setPos(center + QVector2D{-2, 0})
                          .setMaxConnections(2)
                          .setTokenBranchNumber(0)
                          .setCellFeature(CellFeatureDescription())
                          .addToken(token)});
    for (int i = 0; i < cellLength; ++i) {
        cluster.addCell(CellDescription()
                            .setId(cellIds[3 + i])
                            .setConnectingCells({cellIds[2 + i], cellIds[4 + i]})
                            .setEnergy(cellEnergy)
                            .setPos(center + QVector2D{-2.0f + i, 1.0f })
                            .setMaxConnections(2)
                            .setTokenBranchNumber(0)
                            .setCellFeature(CellFeatureDescription()));
    }
    cluster.addCell(CellDescription()
                        .setId(cellIds[3 + cellLength])
                        .setConnectingCells({cellIds[2 + cellLength]})
                        .setEnergy(cellEnergy)
                        .setPos(center + QVector2D{-2.0f + cellLength - 1.0f, 0})
                        .setMaxConnections(1)
                        .setTokenBranchNumber(0)
                        .setCellFeature(CellFeatureDescription()));

    auto const& origCells = *cluster.cells;

    cluster.setPos(cluster.getClusterPosFromCells());

    DataDescription origData;
    origData.addCluster(cluster);

    IntegrationTestHelper::updateData(_access, origData);

    //perform test
    IntegrationTestHelper::runSimulation(1, _controller);

    //check results
    DataDescription newData = IntegrationTestHelper::getContent(_access, { { 0, 0 },{ _universeSize.x, _universeSize.y } });
    checkEnergy(origData, newData);

    auto const& newCluster = newData.clusters->at(0);

    TestResult result;
    result.movementOfCenter = *newCluster.pos - *cluster.pos;
    result.increaseNumberOfCells = newCluster.cells->size() - cluster.cells->size();

    auto newCellByCellId = IntegrationTestHelper::getCellByCellId(newData);

    auto const& newConstructor = newCellByCellId.at(cellIds[1]);
    auto const& newToken = newConstructor.tokens->at(0);

    result.origToken = token;
    result.token = newToken;
    result.origSourceCell = origCells[0];
    if (newCellByCellId.find(cellIds[0]) != newCellByCellId.end()) {
        result.sourceCell = newCellByCellId.at(cellIds[0]);
    }
    result.origConstructorCell = origCells[1];
    result.constructorCell = newConstructor;
    for (int i = 1; i <= 3 + cellLength; ++i) {
        result.origConstructor.emplace_back(origCells[i]);
        newCellByCellId.erase(cellIds[i]);
    }
    result.origConstructionSite.emplace_back(origCells[0]);
    for (auto const& cell : newCellByCellId | boost::adaptors::map_values) {
        result.constructionSite.emplace_back(cell);
    }

    return result;
}

TokenDescription ConstructorGpuTests::createTokenForConstruction(TokenForConstructionParameters tokenParameters) const
{
    auto token = createSimpleToken();
    auto& tokenData = *token.data;
    tokenData[Enums::Constr::IN] = tokenParameters._constructionInput;
    tokenData[Enums::Constr::IN_OPTION] = tokenParameters._constructionOption;
    tokenData[Enums::Constr::INOUT_ANGLE] = QuantityConverter::convertAngleToData(tokenParameters._angle);
    tokenData[Enums::Constr::IN_DIST] = QuantityConverter::convertDistanceToData(tokenParameters._distance);
    tokenData[Enums::Constr::IN_CELL_MAX_CONNECTIONS] = tokenParameters._maxConnections;
    tokenData[Enums::Constr::IN_CELL_BRANCH_NO] = tokenParameters._cellBranchNumber;
    tokenData[Enums::Constr::IN_CELL_FUNCTION_DATA] = tokenParameters._staticData.size();
    tokenData.replace(
        Enums::Constr::IN_CELL_FUNCTION_DATA + 1, tokenParameters._staticData.size(), tokenParameters._staticData);
    int const mutableDataIndex = Enums::Constr::IN_CELL_FUNCTION_DATA + 1 + tokenParameters._staticData.size();
    tokenData[mutableDataIndex] = tokenParameters._mutableData.size();
    tokenData.replace(mutableDataIndex + 1, tokenParameters._mutableData.size(), tokenParameters._mutableData);

    token.energy = tokenParameters._energy.get_value_or(
        2 * _parameters.tokenMinEnergy + 2 * _parameters.cellFunctionConstructorOffspringCellEnergy);
    return token;
}

optional<CellDescription> ConstructorGpuTests::TestResult::getConstructedCell() const
{
    map<uint64_t, CellDescription> cellsBefore;
    for (auto const& cell : origConstructionSite) {
        cellsBefore.insert_or_assign(cell.id, cell);
    }
    map<uint64_t, CellDescription> cellsRemaining;
    for (auto const& cell : constructionSite) {
        if (cellsBefore.find(cell.id) == cellsBefore.end()) {
            cellsRemaining.insert_or_assign(cell.id, cell);
        }
    }

    CHECK(cellsRemaining.size() <= 1);
    if (!cellsRemaining.empty()) {
        return cellsRemaining.begin()->second;
    }

    return boost::none;
}

optional<CellDescription> ConstructorGpuTests::TestResult::getFirstCellOfOrigConstructionSite() const
{
    for (auto const& origConstructionSiteCell : origConstructionSite) {
        if (*origConstructionSiteCell.tokenBlocked && origConstructionSiteCell.isConnectedTo(constructorCell.id)) {
            return origConstructionSiteCell;
        }
    }
    return boost::none;
}

optional<CellDescription> ConstructorGpuTests::TestResult::getCellOfConstructionSite(uint64_t id) const
{
    for (auto const& constructionSiteCell : constructionSite) {
        if (id == constructionSiteCell.id) {
            return constructionSiteCell;
        }
    }
    return boost::none;
}

void ConstructorGpuTests::_ResultChecker::check(TestResult const& testResult, Expectations const& expectations) const
{
    if (expectations._destruction) {
        checkIfDestruction(testResult, expectations);
    } else {
        checkIfNoDestruction(testResult, expectations);
    }
}

void ConstructorGpuTests::_ResultChecker::checkIfDestruction(
    TestResult const& testResult,
    Expectations const& expectations) const
{
    auto const& token = testResult.token;

    EXPECT_EQ(expectations._tokenOutput, token.data->at(Enums::Constr::OUT));

    if (Enums::ConstrIn::DO_NOTHING == token.data->at(Enums::Constr::IN)) {
        EXPECT_FALSE(testResult.getConstructedCell());
        return;
    }
}

void ConstructorGpuTests::_ResultChecker::checkIfNoDestruction(
    TestResult const& testResult,
    Expectations const& expectations) const
{
    auto const& token = testResult.token;

    EXPECT_EQ(expectations._tokenOutput, token.data->at(Enums::Constr::OUT));
    EXPECT_TRUE(isCompatible(testResult.movementOfCenter, QVector2D{}));

    if (Enums::ConstrIn::DO_NOTHING == token.data->at(Enums::Constr::IN)) {
        EXPECT_FALSE(testResult.getConstructedCell());
        return;
    }

    if (Enums::ConstrOut::SUCCESS == expectations._tokenOutput) {
        checkIfNoDestructionAndSuccess(testResult, expectations);
    } else {
        EXPECT_FALSE(testResult.getConstructedCell());
    }
}

void ConstructorGpuTests::_ResultChecker::checkIfNoDestructionAndSuccess(
    TestResult const& testResult,
    Expectations const& expectations) const
{
    EXPECT_TRUE(testResult.getConstructedCell());

    checkCellPosition(testResult, expectations);
    checkCellAttributes(testResult.token, *testResult.getConstructedCell());
    checkCellConnections(testResult);
    checkConstructedToken(testResult, expectations);
}

void ConstructorGpuTests::_ResultChecker::checkCellPosition(
    TestResult const& testResult,
    Expectations const& expectations) const
{
    if (testResult.origConstructionSite.empty()) {
        EXPECT_PRED3(
            predEqual,
            0,
            (*testResult.constructorCell.pos + *expectations._relPosOfFirstCellOfConstructionSite
             - *testResult.getConstructedCell()->pos)
                .length(),
            0.05);
    } else {

        //check distances
        auto const firstCellOfOrigConstructionSite = *testResult.getFirstCellOfOrigConstructionSite();
        auto const secondCellOfConstructionSite =
            testResult.getCellOfConstructionSite(firstCellOfOrigConstructionSite.id);
        {
            auto const displacement = *secondCellOfConstructionSite->pos - *testResult.getConstructedCell()->pos;
            auto const expectedDistance =
                QuantityConverter::convertDataToDistance(testResult.token.data->at(Enums::Constr::IN_DIST));
            EXPECT_PRED3(predEqual, expectedDistance, displacement.length(), 0.05);
        }

        {
            auto const displacement = *testResult.getConstructedCell()->pos - *testResult.constructorCell.pos;
            EXPECT_TRUE(isCompatible(_parameters.cellFunctionConstructorOffspringCellDistance, displacement.length()));
        }

        //check angles
        if (testResult.sourceCell) {
            auto const origAngle = Physics::clockwiseAngleFromFirstToSecondVector(
                *firstCellOfOrigConstructionSite.pos - *testResult.origConstructorCell.pos,
                *testResult.origSourceCell.pos - *testResult.origConstructorCell.pos);
            auto const angle = Physics::clockwiseAngleFromFirstToSecondVector(
                *testResult.getConstructedCell()->pos - *testResult.constructorCell.pos,
                *testResult.sourceCell->pos - *testResult.constructorCell.pos);
            EXPECT_TRUE(isCompatible(origAngle, angle));
        }

        auto const expectedAngle =
            QuantityConverter::convertDataToAngle(testResult.origToken.data->at(Enums::Constr::INOUT_ANGLE));
        auto const actualAngle = Physics::clockwiseAngleFromFirstToSecondVector(
            *testResult.constructorCell.pos - *testResult.getConstructedCell()->pos,
            *secondCellOfConstructionSite->pos - *testResult.getConstructedCell()->pos);
        EXPECT_PRED3(predEqual, expectedAngle + 180.0f, actualAngle, 0.05);

        if (testResult.sourceCell) {
            auto const firstCellOfOrigConstructionSite = *testResult.getFirstCellOfOrigConstructionSite();
            vector<QVector2D> relPositionOfMasses;
            std::transform(
                testResult.origConstructor.begin(),
                testResult.origConstructor.end(),
                std::inserter(relPositionOfMasses, relPositionOfMasses.begin()),
                [&firstCellOfOrigConstructionSite](auto const& cell) {
                    return *cell.pos - *firstCellOfOrigConstructionSite.pos;
                });
            auto const angularMassConstructor = Physics::angularMass(relPositionOfMasses);

            relPositionOfMasses.clear();
            std::transform(
                testResult.origConstructionSite.begin(),
                testResult.origConstructionSite.end(),
                std::inserter(relPositionOfMasses, relPositionOfMasses.begin()),
                [&firstCellOfOrigConstructionSite](auto const& cell) {
                    return *cell.pos - *firstCellOfOrigConstructionSite.pos;
                });
            auto const angularMassConstructionSite = Physics::angularMass(relPositionOfMasses);

            auto const sumAngularMasses = angularMassConstructor + angularMassConstructionSite;
            auto const expectedDeltaAngleConstructionSite = angularMassConstructor * expectedAngle / sumAngularMasses;
            auto const expectedDeltaAngleConstructor = -angularMassConstructionSite * expectedAngle / sumAngularMasses;

            auto const origAngleConstructor =
                Physics::angleOfVector(*testResult.origSourceCell.pos - *testResult.origConstructorCell.pos);
            auto const angleConstructor =
                Physics::angleOfVector(*testResult.sourceCell->pos - *testResult.constructorCell.pos);
            EXPECT_TRUE(isCompatible(expectedDeltaAngleConstructor, angleConstructor - origAngleConstructor));

            if (testResult.origConstructionSite.size() >= 2) {
                auto const& origConstructionSiteCell1 = testResult.origConstructionSite.at(0);
                auto const& origConstructionSiteCell2 = testResult.origConstructionSite.at(1);
                auto const constructionSiteCell1 = testResult.getCellOfConstructionSite(origConstructionSiteCell1.id);
                auto const constructionSiteCell2 = testResult.getCellOfConstructionSite(origConstructionSiteCell2.id);
                auto const origAngleConstructionSite =
                    Physics::angleOfVector(*origConstructionSiteCell1.pos - *origConstructionSiteCell2.pos);
                auto const angleConstructionSite =
                    Physics::angleOfVector(*constructionSiteCell1->pos - *constructionSiteCell2->pos);
                EXPECT_TRUE(isCompatible(
                    expectedDeltaAngleConstructionSite, angleConstructionSite - origAngleConstructionSite));
            }
        }
    }
}

void ConstructorGpuTests::_ResultChecker::checkCellAttributes(
    TokenDescription const& token,
    CellDescription const& cell) const
{
    EXPECT_TRUE(isCompatible(_parameters.cellFunctionConstructorOffspringCellEnergy, static_cast<float>(*cell.energy)));

    auto const expectedMaxConnections = token.data->at(Enums::Constr::IN_CELL_MAX_CONNECTIONS);
    auto const expectedBranchNumber = token.data->at(Enums::Constr::IN_CELL_BRANCH_NO);
    auto const expectedCellFunctionType = token.data->at(Enums::Constr::IN_CELL_FUNCTION);

    auto const expectedStaticDataLength = token.data->at(Enums::Constr::IN_CELL_FUNCTION_DATA);
    auto const expectedStaticData = token.data->mid(Enums::Constr::IN_CELL_FUNCTION_DATA + 1, expectedStaticDataLength);
    auto const mutableDataIndex = Enums::Constr::IN_CELL_FUNCTION_DATA + 1 + expectedStaticDataLength;
    auto const expectedMutableDataLength = token.data->at(mutableDataIndex);
    auto const expectedMutableData = token.data->mid(mutableDataIndex + 1, expectedMutableDataLength);

    EXPECT_EQ(expectedBranchNumber, *cell.tokenBranchNumber);
    EXPECT_EQ(expectedCellFunctionType, cell.cellFeature->type);
    EXPECT_EQ(expectedStaticData, cell.cellFeature->constData);
    EXPECT_EQ(expectedMutableData, cell.cellFeature->volatileData);

    auto const decreaseMaxConnectionIfReduced = isReduced(token) ? -1 : 0;
    auto const isAutomaticMaxConnection = 0 == expectedMaxConnections;
    if (isAutomaticMaxConnection) {
        EXPECT_EQ(
            std::max(static_cast<int>(cell.connectingCells->size()), 2) + decreaseMaxConnectionIfReduced,
            *cell.maxConnections);
    } else {
        EXPECT_EQ(expectedMaxConnections, *cell.maxConnections);
    }

    EXPECT_EQ(!isFinished(token), cell.tokenBlocked);
}

void ConstructorGpuTests::_ResultChecker::checkCellConnections(TestResult const& testResult) const
{
    auto const& token = testResult.token;
    auto const constructedCell = *testResult.getConstructedCell();
    EXPECT_EQ(!isSeparated(token), constructedCell.isConnectedTo(testResult.constructorCell.id));
    EXPECT_EQ(!isSeparated(token), testResult.constructorCell.isConnectedTo(constructedCell.id));

    auto const increaseMaxConnectionIfNewConstructionSite = 0 == testResult.origConstructionSite.size() ? 1 : 0;
    if (*testResult.origConstructorCell.maxConnections == testResult.origConstructorCell.connectingCells->size()) {
        auto const decreaseMaxConnectionIfReduced = isReduced(token) ? -1 : 0;
        EXPECT_EQ(
            *testResult.origConstructorCell.maxConnections + increaseMaxConnectionIfNewConstructionSite
                + decreaseMaxConnectionIfReduced,
            *testResult.constructorCell.maxConnections);
    }
}

void ConstructorGpuTests::_ResultChecker::checkConstructedToken(
    TestResult const& testResult,
    Expectations const& expectations) const
{
    if (expectations._constructedToken) {
        auto const actualTokens = testResult.getConstructedCell()->tokens;
        EXPECT_EQ(1, actualTokens->size());
        EXPECT_TRUE(isCompatible(*expectations._constructedToken, actualTokens->at(0)));
    }
}

bool ConstructorGpuTests::isFinished(TokenDescription const& token)
{
    auto const option = token.data->at(Enums::Constr::IN_OPTION);
    return Enums::ConstrInOption::FINISH_NO_SEP == option || Enums::ConstrInOption::FINISH_WITH_SEP == option
        || Enums::ConstrInOption::FINISH_WITH_SEP_RED == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option;
}

bool ConstructorGpuTests::isReduced(TokenDescription const& token)
{
    auto const option = token.data->at(Enums::Constr::IN_OPTION);
    return Enums::ConstrInOption::FINISH_WITH_SEP_RED == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option;
}

bool ConstructorGpuTests::isSeparated(TokenDescription const& token)
{
    auto const option = token.data->at(Enums::Constr::IN_OPTION);
    return Enums::ConstrInOption::FINISH_WITH_SEP == option || Enums::ConstrInOption::FINISH_WITH_SEP_RED == option
        || Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED == option;
}

float ConstructorGpuTests::getOffspringDistance(WithSeparation value) const
{
    return value == WithSeparation::Yes ? _parameters.cellFunctionConstructorOffspringCellDistance * 2
                                        : _parameters.cellFunctionConstructorOffspringCellDistance;
}

QVector2D ConstructorGpuTests::constructorPositionForHorizontalClusterAfterCreation(
    vector<QVector2D> constructionSite,
    QVector2D constructor,
    vector<QVector2D> remainingCells,
    float distanceBetweenOffspringToConstructionSite,
    WithSeparation withSeparation) const
{
    auto origCenter = std::accumulate(constructionSite.begin(), constructionSite.end(), constructor);
    origCenter = std::accumulate(remainingCells.begin(), remainingCells.end(), origCenter);
    origCenter /= constructionSite.size() + 1 + remainingCells.size();

    auto offspringCellPos = constructor + QVector2D{getOffspringDistance(withSeparation), 0};
    for (auto& pos : constructionSite) {
        pos += QVector2D{distanceBetweenOffspringToConstructionSite, 0};
    }

    auto center = std::accumulate(constructionSite.begin(), constructionSite.end(), constructor + offspringCellPos);
    center = std::accumulate(remainingCells.begin(), remainingCells.end(), center);
    center /= constructionSite.size() + 2 + remainingCells.size();

    auto const centerDisplacement = center - origCenter;
    return constructor - centerDisplacement;
}

TEST_F(ConstructorGpuTests, testDoNothing)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::DO_NOTHING));
    auto result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_standardParameters)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_nonStandardParameters1)
{
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::SAFE)
                                                      .cellBranchNumber(2)
                                                      .maxConnections(3)
                                                      .cellFunctionType(Enums::CellFunction::SCANNER));
    auto result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_nonStandardParameters2)
{
    auto const basicFacade = ServiceLocator::getInstance().getService<ModelBasicBuilderFacade>();
    auto const compiler =
        basicFacade->buildCellComputerCompiler(_context->getSymbolTable(), _context->getSimulationParameters());

    std::stringstream stream;
    stream << "mov [1], 3";
    for (int i = 0; i < _parameters.cellFunctionComputerMaxInstructions - 1; ++i) {
        stream << "\nmov [1], 3";
    }

    CompilationResult compiledProgram = compiler->compileSourceCode(stream.str());
    CHECK(compiledProgram.compilationOk);

    auto const token =
        createTokenForConstruction(TokenForConstructionParameters()
                                       .constructionInput(Enums::ConstrIn::SAFE)
                                       .cellBranchNumber(1)
                                       .maxConnections(2)
                                       .cellFunctionType(Enums::CellFunction::COMPUTER)
                                       .staticData(compiledProgram.compilation)
                                       .mutableData(QByteArray(_parameters.cellFunctionComputerCellMemorySize, 1)));
    auto result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_ignoreDistanceOnFirstConstructedCell1)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).distance(getOffspringDistance() / 2));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));

    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_ignoreDistanceOnFirstConstructedCell2)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).distance(getOffspringDistance() * 2));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));

    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_rightHandSide)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).angle(90.0f));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));

    auto const expectedCellPos = QVector2D{0, getOffspringDistance()};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_leftHandSide)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).angle(-90.0f));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));

    auto const expectedCellPos = QVector2D{0, -getOffspringDistance()};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_errorMaxConnectionsReached)
{
    _parameters.cellMaxBonds = 1;
    _context->setSimulationParameters(_parameters);

    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_CONNECTION));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_errorNoEnergy)
{
    auto const lowTokenEnergy = _parameters.tokenMinEnergy + _parameters.cellFunctionConstructorOffspringCellEnergy / 2;
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).energy(lowTokenEnergy));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_NO_ENERGY));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_otherClusterRightObstacle_safeMode)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(getOffspringDistance()));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_otherClusterRightObstacle_unsafeMode)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::UNSAFE));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(getOffspringDistance()));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_otherClusterRightObstacle_bruteforceMode)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::BRUTEFORCE));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(getOffspringDistance()));

    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations()
            .tokenOutput(Enums::ConstrOut::SUCCESS)
            .relPosOfFirstCellOfConstructionSite(expectedCellPos)
            .destruction(true));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_otherClusterLeftObstacle_safeMode)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            -1));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_otherClusterLeftObstacle_unsafeMode)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::UNSAFE));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            -1));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_otherClusterLeftObstacle_bruteforceMode)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::BRUTEFORCE));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            -1));

    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations()
            .tokenOutput(Enums::ConstrOut::SUCCESS)
            .relPosOfFirstCellOfConstructionSite(expectedCellPos)
            .destruction(true));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_sameClusterObstacle_safeMode)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).angle(90));
    auto const result = runStartConstructionOnWedgeClusterTest(token, 180, 0);

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_sameClusterObstacle_unsafeMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::UNSAFE).angle(90));
    auto const result = runStartConstructionOnWedgeClusterTest(token, 180, 0);

    auto const expectedCellPos = QVector2D{0, getOffspringDistance()};
    _resultChecker->check(
        result,
        Expectations()
            .tokenOutput(Enums::ConstrOut::SUCCESS)
            .relPosOfFirstCellOfConstructionSite(expectedCellPos)
            .destruction(true));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_sameClusterObstacle_bruteforceMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::BRUTEFORCE).angle(90));
    auto const result = runStartConstructionOnWedgeClusterTest(token, 180, 0);

    auto const expectedCellPos = QVector2D{0, getOffspringDistance()};
    _resultChecker->check(
        result,
        Expectations()
            .tokenOutput(Enums::ConstrOut::SUCCESS)
            .relPosOfFirstCellOfConstructionSite(expectedCellPos)
            .destruction(true));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnWedgeCluster_rightHandSide)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runStartConstructionOnWedgeClusterTest(token, 90, 0);

    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnWedgeCluster_leftHandSide)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runStartConstructionOnWedgeClusterTest(token, 270, 0);

    auto const expectedCellPos = QVector2D{-getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnWedgeCluster_diagonal)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runStartConstructionOnWedgeClusterTest(token, 90, 45);

    auto const expectedCellPos = QVector2D{getOffspringDistance() / sqrtf(2), getOffspringDistance() / sqrtf(2)};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnTiangleCluster)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runStartConstructionOnTriangleClusterTest(token);

    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_withEmptyToken)
{
    auto const cellBranchNumber = 1;
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::SAFE)
                                                      .constructionOption(Enums::ConstrInOption::CREATE_EMPTY_TOKEN)
                                                      .cellBranchNumber(cellBranchNumber));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    QByteArray expectedTokenMemory(_parameters.tokenMemorySize, 0);
    expectedTokenMemory[0] = cellBranchNumber;
    auto const expectedToken = TokenDescription()
                                   .setEnergy(_parameters.cellFunctionConstructorOffspringTokenEnergy)
                                   .setData(expectedTokenMemory);
    _resultChecker->check(
        result,
        Expectations()
            .tokenOutput(Enums::ConstrOut::SUCCESS)
            .relPosOfFirstCellOfConstructionSite(expectedCellPos)
            .constructedToken(expectedToken));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_withDuplicatedToken)
{
    auto const cellBranchNumber = 1;
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::SAFE)
                                                      .constructionOption(Enums::ConstrInOption::CREATE_DUP_TOKEN)
                                                      .cellBranchNumber(cellBranchNumber));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    auto expectedTokenMemory = *token.data;
    expectedTokenMemory[0] = cellBranchNumber;
    auto const expectedToken = TokenDescription()
                                   .setEnergy(_parameters.cellFunctionConstructorOffspringTokenEnergy)
                                   .setData(expectedTokenMemory);
    _resultChecker->check(
        result,
        Expectations()
            .tokenOutput(Enums::ConstrOut::SUCCESS)
            .relPosOfFirstCellOfConstructionSite(expectedCellPos)
            .constructedToken(expectedToken));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_finishWithoutSeparation)
{
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::SAFE)
                                                      .constructionOption(Enums::ConstrInOption::FINISH_NO_SEP));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    auto const expectedCellPos = QVector2D{getOffspringDistance(), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_finishWithSeparation)
{
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::SAFE)
                                                      .constructionOption(Enums::ConstrInOption::FINISH_WITH_SEP));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    auto const expectedCellPos = QVector2D{getOffspringDistance(WithSeparation::Yes), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(
    ConstructorGpuTests,
    testConstructFirstCellOnHorizontalCluster_finishWithSeparation_otherClusterRightObstacle_safeMode)
{
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::SAFE)
                                                      .constructionOption(Enums::ConstrInOption::FINISH_WITH_SEP));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            getOffspringDistance(WithSeparation::Yes)));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(
    ConstructorGpuTests,
    testConstructFirstCellOnHorizontalCluster_finishWithSeparation_otherClusterRightObstacle_unsafeMode)
{
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::UNSAFE)
                                                      .constructionOption(Enums::ConstrInOption::FINISH_WITH_SEP));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            getOffspringDistance(WithSeparation::Yes)));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(
    ConstructorGpuTests,
    testConstructFirstCellOnHorizontalCluster_finishWithSeparation_otherClusterRightObstacle_bruteforceMode)
{
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::BRUTEFORCE)
                                                      .constructionOption(Enums::ConstrInOption::FINISH_WITH_SEP));
    auto const result = runStartConstructionOnHorizontalClusterTest(
        StartConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            getOffspringDistance(WithSeparation::Yes)));

    auto const expectedCellPos = QVector2D{getOffspringDistance(WithSeparation::Yes), 0};
    _resultChecker->check(
        result,
        Expectations()
            .tokenOutput(Enums::ConstrOut::SUCCESS)
            .relPosOfFirstCellOfConstructionSite(expectedCellPos)
            .destruction(true));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_finishWithSeparationAndReduction)
{
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::SAFE)
                                                      .constructionOption(Enums::ConstrInOption::FINISH_WITH_SEP_RED));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));
    auto const expectedCellPos = QVector2D{getOffspringDistance(WithSeparation::Yes), 0};
    _resultChecker->check(
        result,
        Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).relPosOfFirstCellOfConstructionSite(expectedCellPos));
}

TEST_F(ConstructorGpuTests, testConstructFirstCellOnHorizontalCluster_finishWithTokenAndSeparationAndReduction)
{
    auto const cellBranchNumber = 1;
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters()
                                       .constructionInput(Enums::ConstrIn::SAFE)
                                       .constructionOption(Enums::ConstrInOption::FINISH_WITH_TOKEN_SEP_RED)
                                       .cellBranchNumber(cellBranchNumber));
    auto const result =
        runStartConstructionOnHorizontalClusterTest(StartConstructionOnHorizontalClusterTestParameters().token(token));

    auto const expectedCellPos = QVector2D{getOffspringDistance(WithSeparation::Yes), 0};
    QByteArray expectedTokenMemory(_parameters.tokenMemorySize, 0);
    expectedTokenMemory[0] = cellBranchNumber;
    auto const expectedToken = TokenDescription()
                                   .setEnergy(_parameters.cellFunctionConstructorOffspringTokenEnergy)
                                   .setData(expectedTokenMemory);
    _resultChecker->check(
        result,
        Expectations()
            .tokenOutput(Enums::ConstrOut::SUCCESS)
            .relPosOfFirstCellOfConstructionSite(expectedCellPos)
            .constructedToken(expectedToken));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_standardParameters)
{
    auto const token =
        createTokenForConstruction(TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));
    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_minDistance)
{
    auto const minDistance = _parameters.cellMinDistance;
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).distance(minDistance * 1.1f));
    auto result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));
    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_maxDistance)
{
    auto const maxDistance = _parameters.cellMaxDistance;
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).distance(maxDistance * 0.9f));
    auto result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));
    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS));
}


TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_errorTooLowDistance)
{
    auto const minDistance = _parameters.cellMinDistance;
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).distance(minDistance * 0.9f));
    auto result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));
    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_DIST));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_errorTooLargeDistance)
{
    auto const maxDistance = _parameters.cellMaxDistance;
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).distance(maxDistance * 1.1f));
    auto result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));
    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_DIST));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_rightHandSide)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).angle(30.0f));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_leftHandSide)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).angle(-30.0f));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_errorNoEnergy)
{
    auto const lowTokenEnergy = _parameters.tokenMinEnergy + _parameters.cellFunctionConstructorOffspringCellEnergy / 2;
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE).energy(lowTokenEnergy));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_NO_ENERGY));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_otherClusterRightObstacle_safeMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            1 + getOffspringDistance()));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_otherClusterRightObstacle_unsafeMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::UNSAFE));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            1 + getOffspringDistance()));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_otherClusterRightObstacle_bruteforceMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::BRUTEFORCE));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(
            1 + getOffspringDistance()));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).destruction(true));
}


TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_otherClusterLeftObstacle_safeMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(-1));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_otherClusterLeftObstacle_unsafeMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::UNSAFE));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(-1));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_otherClusterLeftObstacle_bruteforceMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::BRUTEFORCE));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token).horizontalObstacleAt(-1));

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).destruction(true));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnSelfTouchingCluster_sameClusterObstacle_safeMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::SAFE));
    auto const result = runContinueConstructionOnSelfTouchingClusterTest(token, 4);

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::ERROR_OBSTACLE));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnSelfTouchingCluster_sameClusterObstacle_unsafeMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::UNSAFE));
    auto const result = runContinueConstructionOnSelfTouchingClusterTest(token, 4);

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).destruction(true));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnSelfTouchingCluster_sameClusterObstacle_bruteforceMode)
{
    auto const token = createTokenForConstruction(
        TokenForConstructionParameters().constructionInput(Enums::ConstrIn::BRUTEFORCE));
    auto const result = runContinueConstructionOnSelfTouchingClusterTest(token, 4);

    _resultChecker->check(result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).destruction(true));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_withEmptyToken)
{
    auto const cellBranchNumber = 1;
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
        .constructionInput(Enums::ConstrIn::SAFE)
        .constructionOption(Enums::ConstrInOption::CREATE_EMPTY_TOKEN)
        .cellBranchNumber(cellBranchNumber));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));
    QByteArray expectedTokenMemory(_parameters.tokenMemorySize, 0);
    expectedTokenMemory[0] = cellBranchNumber;
    auto const expectedToken = TokenDescription()
        .setEnergy(_parameters.cellFunctionConstructorOffspringTokenEnergy)
        .setData(expectedTokenMemory);
    _resultChecker->check(
        result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).constructedToken(expectedToken));
}

TEST_F(ConstructorGpuTests, testConstructSecondCellOnHorizontalCluster_withDuplicatedToken)
{
    auto const cellBranchNumber = 1;
    auto const token = createTokenForConstruction(TokenForConstructionParameters()
                                                      .constructionInput(Enums::ConstrIn::SAFE)
                                                      .constructionOption(Enums::ConstrInOption::CREATE_DUP_TOKEN)
                                                      .cellBranchNumber(cellBranchNumber));
    auto const result = runContinueConstructionOnHorizontalClusterTest(
        ContinueConstructionOnHorizontalClusterTestParameters().token(token));
    auto expectedTokenMemory = *token.data;
    expectedTokenMemory[0] = cellBranchNumber;
    auto const expectedToken = TokenDescription()
                                   .setEnergy(_parameters.cellFunctionConstructorOffspringTokenEnergy)
                                   .setData(expectedTokenMemory);
    _resultChecker->check(
        result, Expectations().tokenOutput(Enums::ConstrOut::SUCCESS).constructedToken(expectedToken));
}

//TODO:
//Token auf ConstructionSite vorhanden
//only Rotation