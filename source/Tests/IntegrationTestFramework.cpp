#include <QEventLoop>
#include <QMatrix4x4>

#include "Base/ServiceLocator.h"
#include "Base/GlobalFactory.h"
#include "Base/NumberGenerator.h"
#include "EngineInterface/EngineInterfaceBuilderFacade.h"
#include "EngineInterface/EngineInterfaceBuilderFacade.h"
#include "EngineInterface/SimulationParameters.h"
#include "EngineInterface/SimulationAccess.h"
#include "EngineGpu/EngineGpuBuilderFacade.h"

#include "Predicates.h"
#include "IntegrationTestFramework.h"

IntegrationTestFramework::IntegrationTestFramework(IntVector2D const& universeSize)
	: _universeSize(universeSize)
{
	GlobalFactory* factory = ServiceLocator::getInstance().getService<GlobalFactory>();
	_basicFacade = ServiceLocator::getInstance().getService<EngineInterfaceBuilderFacade>();
	_gpuFacade = ServiceLocator::getInstance().getService<EngineGpuBuilderFacade>();
	_symbols = _basicFacade->getDefaultSymbolTable();
	_parameters = _basicFacade->getDefaultSimulationParameters();
}

IntegrationTestFramework::~IntegrationTestFramework()
{
}

ClusterDescription IntegrationTestFramework::createSingleCellClusterWithCompleteData(uint64_t clusterId /*= 0*/, uint64_t cellId /*= 0*/) const
{
	QByteArray code("123123123");
	QByteArray cellMemory(_parameters.cellFunctionComputerCellMemorySize, 0);
	QByteArray tokenMemory(_parameters.tokenMemorySize, 0);
	cellMemory[1] = 'a';
	cellMemory[2] = 'b';
	tokenMemory[0] = 't';
	tokenMemory[3] = 's';
	CellMetadata cellMetadata;
	cellMetadata.color = 2;
	cellMetadata.name = "name1";
	cellMetadata.computerSourcecode = "code";
	cellMetadata.description = "desc";
	ClusterMetadata clusterMetadata;
    clusterMetadata.name = "name2";

    return ClusterDescription()
        .addCell(CellDescription()
                     .setCellFeature(CellFeatureDescription()
                                         .setType(Enums::CellFunction::COMPUTER)
                                         .setConstData(code)
                                         .setVolatileData(cellMemory))
                     .setId(cellId)
                     .setPos({1, 2})
                     .setEnergy(_parameters.cellMinEnergy * 2)
                     .setFlagTokenBlocked(true)
                     .setMaxConnections(3)
                     .setMetadata(cellMetadata)
                     .setTokenBranchNumber(2)
                     .setTokenUsages(3)
                     .setTokens({TokenDescription().setData(tokenMemory).setEnergy(89)}))
        .setId(clusterId)
        .setPos({1, 2})
        .setVel({-1, 1})
        .setAngle(23)
        .setAngularVel(1.2)
        .setMetadata(clusterMetadata);
}

TokenDescription IntegrationTestFramework::createSimpleToken() const
{
	auto tokenEnergy = _parameters.tokenMinEnergy * 2.0;
	return TokenDescription().setEnergy(tokenEnergy).setData(QByteArray(_parameters.tokenMemorySize, 0));
}

ClusterDescription IntegrationTestFramework::createRectangularCluster(IntVector2D const & size, boost::optional<QVector2D> const & centerPos, 
	boost::optional<QVector2D> const & centerVel, Boundary boundary) const
{
    QVector2D pos = centerPos
        ? *centerPos
        : QVector2D(
            _numberGen->getRandomReal(0, _universeSize.x - 1), _numberGen->getRandomReal(0, _universeSize.y - 1));
    QVector2D vel = centerVel ? *centerVel : QVector2D(_numberGen->getRandomReal(-1, 1), _numberGen->getRandomReal(-1, 1));

	ClusterDescription cluster;
	cluster.setId(_numberGen->getId()).setPos(pos).setVel(vel).setAngle(0).setAngularVel(0);

	for (int y = 0; y < size.y; ++y) {
		for (int x = 0; x < size.x; ++x) {
			QVector2D relPos(-static_cast<float>(size.x - 1) / 2.0 + x, -static_cast<float>(size.y - 1) / 2.0 + y);
			int maxConnections = 4;
            if (Boundary::NonSticky == boundary) {
                if (x == 0 || x == size.x - 1) {
                    --maxConnections;
                }
                if (y == 0 || y == size.y - 1) {
                    --maxConnections;
                }
            }
			cluster.addCell(
				CellDescription().setEnergy(_parameters.cellFunctionConstructorOffspringCellEnergy)
				.setPos(pos + relPos)
				.setMaxConnections(maxConnections).setId(_numberGen->getId()).setCellFeature(CellFeatureDescription())
			);
		}
	}

	for (int x = 0; x < size.x; ++x) {
		for (int y = 0; y < size.y; ++y) {
			list<uint64_t> connectingCells;
			if (x > 0) {
				connectingCells.emplace_back(cluster.cells->at((x - 1) + y * size.x).id);
			}
			if (x < size.x - 1) {
				connectingCells.emplace_back(cluster.cells->at((x + 1) + y * size.x).id);
			}
			if (y > 0) {
				connectingCells.emplace_back(cluster.cells->at(x + (y - 1) * size.x).id);
			}
			if (y < size.y - 1) {
				connectingCells.emplace_back(cluster.cells->at(x + (y + 1) * size.x).id);
			}

			cluster.cells->at(x + y * size.x).setConnectingCells(connectingCells);
		}
	}

	return cluster;
}

ClusterDescription IntegrationTestFramework::createLineCluster(int numCells, boost::optional<QVector2D> const & centerPos,
	boost::optional<QVector2D> const & centerVel, boost::optional<double> const & optAngle, boost::optional<double> const & optAngularVel) const
{
    QVector2D pos = centerPos
        ? *centerPos
        : QVector2D(
            _numberGen->getRandomReal(0, _universeSize.x - 1), _numberGen->getRandomReal(0, _universeSize.y - 1));
    QVector2D vel = centerVel ? *centerVel : QVector2D(_numberGen->getRandomReal(-1, 1), _numberGen->getRandomReal(-1, 1));
	double angle = optAngle ? *optAngle : _numberGen->getRandomReal(0, 359);
	double angularVel = optAngularVel.get_value_or(_numberGen->getRandomReal(-1, 1));

	ClusterDescription cluster;
	cluster.setId(_numberGen->getId()).setPos(pos).setVel(vel).setAngle(0).setAngularVel(angularVel);

	QMatrix4x4 transform;
	transform.setToIdentity();
	transform.rotate(angle, 0.0, 0.0, 1.0);

	for (int j = 0; j < numCells; ++j) {
		QVector2D relPosUnrotated(-static_cast<float>(numCells - 1) / 2.0 + j, 0);
		QVector2D relPos = transform.map(QVector3D(relPosUnrotated)).toVector2D();
		cluster.addCell(
			CellDescription().setEnergy(_parameters.cellFunctionConstructorOffspringCellEnergy)
			.setPos(pos + relPos)
			.setMaxConnections(2).setId(_numberGen->getId()).setCellFeature(CellFeatureDescription())
		);
	}
	for (int j = 0; j < numCells; ++j) {
		list<uint64_t> connectingCells;
		if (j > 0) {
			connectingCells.emplace_back(cluster.cells->at(j - 1).id);
		}
		if (j < numCells - 1) {
			connectingCells.emplace_back(cluster.cells->at(j + 1).id);
		}
		cluster.cells->at(j).setConnectingCells(connectingCells);
	}
	return cluster;
}

ClusterDescription IntegrationTestFramework::createHorizontalCluster(
    int numCells,
    boost::optional<QVector2D> const& centerPos,
    boost::optional<QVector2D> const& centerVel,
    boost::optional<double> const& optAngularVel,
    Boundary boundary /*= Boundary::NonSticky*/) const
{
    QVector2D pos = centerPos
        ? *centerPos
        : QVector2D(
            _numberGen->getRandomReal(0, _universeSize.x - 1), _numberGen->getRandomReal(0, _universeSize.y - 1));
    QVector2D vel = centerVel ? *centerVel : QVector2D(_numberGen->getRandomReal(-1, 1), _numberGen->getRandomReal(-1, 1));
	double angularVel = optAngularVel.get_value_or(_numberGen->getRandomReal(-1, 1));

	ClusterDescription cluster;
	cluster.setId(_numberGen->getId()).setPos(pos).setVel(vel).setAngle(0).setAngularVel(angularVel);
	for (int j = 0; j < numCells; ++j) {
        int maxConnection = 2;
        if (boundary == Boundary::NonSticky) {
            if (0 == j || numCells - 1 == j) {
                maxConnection = 1;
            }
        }
		cluster.addCell(
			CellDescription().setEnergy(_parameters.cellFunctionConstructorOffspringCellEnergy)
			.setPos(pos + QVector2D(-static_cast<float>(numCells - 1) / 2.0 + j, 0))
			.setMaxConnections(maxConnection).setId(_numberGen->getId()).setCellFeature(CellFeatureDescription())
		);
	}
	for (int j = 0; j < numCells; ++j) {
		list<uint64_t> connectingCells;
		if (j > 0) {
			connectingCells.emplace_back(cluster.cells->at(j - 1).id);
		}
		if (j < numCells - 1) {
			connectingCells.emplace_back(cluster.cells->at(j + 1).id);
		}
		cluster.cells->at(j).setConnectingCells(connectingCells);
	}
	return cluster;
}

ClusterDescription IntegrationTestFramework::createVerticalCluster(int numCells, boost::optional<QVector2D> const & centerPos, boost::optional<QVector2D> const & centerVel) const
{
    QVector2D pos = centerPos
        ? *centerPos
        : QVector2D(
            _numberGen->getRandomReal(0, _universeSize.x - 1), _numberGen->getRandomReal(0, _universeSize.y - 1));
    QVector2D vel = centerVel ? *centerVel : QVector2D(_numberGen->getRandomReal(-1, 1), _numberGen->getRandomReal(-1, 1));

	ClusterDescription cluster;
	cluster.setId(_numberGen->getId()).setPos(pos).setVel(vel).setAngle(0).setAngularVel(0);
	for (int j = 0; j < numCells; ++j) {
		cluster.addCell(
			CellDescription().setEnergy(_parameters.cellFunctionConstructorOffspringCellEnergy)
			.setPos(pos + QVector2D(0, -static_cast<float>(numCells - 1) / 2.0 + j))
			.setMaxConnections(2).setId(_numberGen->getId()).setCellFeature(CellFeatureDescription())
		);
	}
	for (int j = 0; j < numCells; ++j) {
		list<uint64_t> connectingCells;
		if (j > 0) {
			connectingCells.emplace_back(cluster.cells->at(j - 1).id);
		}
		if (j < numCells - 1) {
			connectingCells.emplace_back(cluster.cells->at(j + 1).id);
		}
		cluster.cells->at(j).setConnectingCells(connectingCells);
	}
	return cluster;
}

ClusterDescription IntegrationTestFramework::createSingleCellCluster(uint64_t clusterId, uint64_t cellId) const
{
    return ClusterDescription()
        .addCell(CellDescription().setId(cellId).setPos({1, 2}).setEnergy(_parameters.cellMinEnergy * 2).setMaxConnections(3))
        .setId(clusterId)
        .setPos({1, 2})
        .setVel({0, 0})
        .setAngle(23)
        .setAngularVel(1.2);
}

ParticleDescription IntegrationTestFramework::createParticle(
    boost::optional<QVector2D> const& optPos,
    boost::optional<QVector2D> const& optVel) const
{
    auto pos = optPos
        ? *optPos
        : QVector2D(
            _numberGen->getRandomReal(0, _universeSize.x - 1), _numberGen->getRandomReal(0, _universeSize.y - 1));
    auto vel = optVel ? *optVel : QVector2D(_numberGen->getRandomReal(-0.5, 0.5), _numberGen->getRandomReal(-0.5, 0.5));
	return ParticleDescription().setEnergy(_parameters.cellMinEnergy / 2).setPos(pos).setVel(vel).setId(_numberGen->getId());
}

QVector2D IntegrationTestFramework::addSmallDisplacement(QVector2D const & value) const
{
    return{ value.x() + 0.04232f, value.y() + 0.04232f };
}


template<>
bool checkCompatibility<double>(double a, double b)
{
    EXPECT_PRED2(predEqual_relative, a, b);
    return predEqual_relative(a, b);
}

template<>
bool checkCompatibility<float>(float a, float b)
{
    EXPECT_PRED2(predEqual_relative, a, b);
    return predEqual_relative(a, b);
}

template<>
bool checkCompatibility<QVector2D>(QVector2D vec1, QVector2D vec2)
{
    auto result = true;
    EXPECT_TRUE(result = checkCompatibility(vec1.x(), vec2.x()));
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(vec1.y(), vec2.y()));
    }
    return result;
}

namespace
{
	void removeZerosAtEnd(QByteArray& data)
	{
		while (true) {
			if (data.isEmpty()) {
				break;
			}
			if (data.at(data.size() - 1) == 0) {
				data.chop(1);
			}
			else {
				break;
			}
		}
	}
}

template<>
bool checkCompatibility<CellFeatureDescription>(CellFeatureDescription feature1, CellFeatureDescription feature2)
{
	removeZerosAtEnd(feature1.volatileData);
    removeZerosAtEnd(feature1.constData);
	removeZerosAtEnd(feature2.volatileData);
    removeZerosAtEnd(feature2.constData);

    auto result = true;
    EXPECT_TRUE(result = checkCompatibility(feature1.getType(), feature2.getType()));
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(feature1.constData, feature2.constData));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(feature1.volatileData, feature2.volatileData));
    }
    return result;
}

template<>
bool checkCompatibility(CellMetadata metadata1, CellMetadata metadata2)
{
    auto result = true;
    EXPECT_TRUE(result = checkCompatibility(metadata1.computerSourcecode, metadata2.computerSourcecode));
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(metadata1.name, metadata2.name));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(metadata1.description, metadata2.description));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(metadata1.color, metadata2.color));
    }
    return result;
}

template<>
bool checkCompatibility(TokenDescription token1, TokenDescription token2)
{
    auto result = true;
    EXPECT_TRUE(result = checkCompatibility(token1.energy, token2.energy));

    //do not compare first byte (overridden branch number)
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(token1.data->mid(1), token2.data->mid(1)));
    }
    return result;
}

template<>
bool checkCompatibility<CellDescription>(CellDescription cell1, CellDescription cell2)
{
    auto result = true;
    EXPECT_TRUE(result = checkCompatibility(cell1.tokenBlocked, cell2.tokenBlocked));
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.pos, cell2.pos));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.energy, cell2.energy));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.maxConnections, cell2.maxConnections));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.connectingCells, cell2.connectingCells));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.tokenBranchNumber, cell2.tokenBranchNumber));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.metadata, cell2.metadata));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.cellFeature, cell2.cellFeature));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.tokens, cell2.tokens));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cell1.tokenUsages, cell2.tokenUsages));
    }
    return result;
}

template<>
bool checkCompatibility<ClusterDescription>(ClusterDescription cluster1, ClusterDescription cluster2)
{
    auto result = true;
    EXPECT_TRUE(result = checkCompatibility(cluster1.pos, cluster2.pos));
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cluster1.vel, cluster2.vel));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cluster1.angle, cluster2.angle));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cluster1.angularVel, cluster2.angularVel));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cluster1.metadata, cluster2.metadata));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(cluster1.cells, cluster2.cells));
    }
    return result;
}

template<>
bool checkCompatibility<ParticleDescription>(ParticleDescription particle1, ParticleDescription particle2)
{
    auto result = true;
    EXPECT_TRUE(result = checkCompatibility(particle1.pos, particle2.pos));
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(particle1.vel, particle2.vel));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(particle1.energy, particle2.energy));
    }
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(particle1.metadata, particle2.metadata));
    }
    return result;
}

namespace
{
	void sortById(DataDescription& data)
	{
		if (data.clusters) {
			std::sort(data.clusters->begin(), data.clusters->end(), [](auto const &cluster1, auto const &cluster2) {
				return cluster1.id <= cluster2.id;
			});
			for (auto& cluster : *data.clusters) {
				std::sort(cluster.cells->begin(), cluster.cells->end(), [](auto const &cell1, auto const &cell2) {
					return cell1.id <= cell2.id;
				});
			}
		}
		if (data.particles) {
			std::sort(data.particles->begin(), data.particles->end(), [](auto const &particle1, auto const &particle2) {
				return particle1.id <= particle2.id;
			});
		}
	}
}

template<>
bool checkCompatibility<DataDescription>(DataDescription data1, DataDescription data2)
{
    sortById(data1);
	sortById(data2);

    auto result = true;
    EXPECT_TRUE(result = checkCompatibility(data1.clusters, data2.clusters));
    if (result) {
        EXPECT_TRUE(result = checkCompatibility(data1.particles, data2.particles));
    }
    return result;
}
