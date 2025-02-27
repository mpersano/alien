﻿#pragma once
#include <QObject>

#include "EngineInterface/Definitions.h"

class MainModel : public QObject {
	Q_OBJECT

public:
	MainModel(QObject * parent = nullptr);
	virtual ~MainModel() = default;

	SimulationParameters getSimulationParameters() const;
	void setSimulationParameters(SimulationParameters const& parameters);

    ExecutionParameters getExecutionParameters() const;
    void setExecutionParameters(ExecutionParameters const& parameters);

	SymbolTable* getSymbolMap() const;
	void setSymbolTable(SymbolTable* symbols);

private:
	SimulationParameters _simulationParameters;
    ExecutionParameters _executionParameters;
    SymbolTable* _symbols = nullptr;
};
