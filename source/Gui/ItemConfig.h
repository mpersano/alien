#pragma once

#include <QObject>

#include "EngineInterface/SimulationParameters.h"

class ItemConfig
	: public QObject
{
	Q_OBJECT
public:
	ItemConfig(QObject* parent = nullptr) : QObject(parent) {}
	~ItemConfig() = default;

	void init(SimulationParameters const& parameters);

	bool isShowCellInfo() const;
	void setShowCellInfo(bool value);

	SimulationParameters const& getSimulationParameters() const;

private:
	bool _showCellInfo = true;
	SimulationParameters _parameters;
};

