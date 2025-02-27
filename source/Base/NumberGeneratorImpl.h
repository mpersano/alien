#pragma once

#include "NumberGenerator.h"

class NumberGeneratorImpl
	: public NumberGenerator
{
public:
	NumberGeneratorImpl(QObject* parent = nullptr);
	virtual ~NumberGeneratorImpl() = default;

	virtual void init(uint32_t arraySize, uint16_t threadId) override;

	virtual uint32_t getRandomInt() override;
	virtual uint32_t getRandomInt(uint32_t range) override;
    virtual uint32_t getRandomInt(uint32_t min, uint32_t max) override;
	virtual double getRandomReal() override;
    virtual double getRandomReal(double min, double max) override;
	virtual QByteArray getRandomArray(int length) override;

	virtual uint64_t getId() override;

private:
    uint32_t getLargeRandomInt(uint32_t range);
    quint32 getNumberFromArray();

	int _index = 0;
	vector<uint32_t> _arrayOfRandomNumbers;
	uint64_t _runningNumber = 0;
	uint64_t _threadId = 0;
};

