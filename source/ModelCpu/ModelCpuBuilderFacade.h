#pragma once

#include "Definitions.h"

class ModelCpuBuilderFacade
{
public:
	virtual ~ModelCpuBuilderFacade() = default;

	struct Config {
		IntVector2D universeSize;
		SymbolTable* symbolTable;
		SimulationParameters* parameters;
	};
	virtual SimulationControllerCpu* buildSimulationController(Config const& config
		, ModelCpuData const& specificData
		, uint timestepAtBeginning) const = 0;
	virtual SimulationAccessCpu* buildSimulationAccess() const = 0;
	virtual SimulationMonitor* buildSimulationMonitor() const = 0;
};