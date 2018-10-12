#pragma once

#include <QThread>

#include "ModelBasic/Definitions.h"
#include "DefinitionsImpl.h"

class ThreadController
	: public QObject
{
	Q_OBJECT
public:
	ThreadController(QObject* parent = nullptr);
	virtual ~ThreadController();

	void init(SpaceProperties *metric);

	virtual WorkerForGpu* getGpuWorker() const;

	void calculate(RunningMode mode);

	Q_SIGNAL void timestepCalculated();

private:
	Q_SIGNAL void runSimulationWithGpu();
	Q_SLOT void timestepCalculatedWithGpu();

	SpaceProperties *_metric = nullptr;

	QThread _thread;
	WorkerForGpu* _worker = nullptr;
	bool _gpuThreadWorking = false;
};