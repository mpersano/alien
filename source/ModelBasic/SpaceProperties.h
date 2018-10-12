#pragma once

#include "Definitions.h"

class MODELBASIC_EXPORT SpaceProperties
	: public QObject
{
	Q_OBJECT
public:
	SpaceProperties(QObject* parent = nullptr);
	virtual ~SpaceProperties() = default;

	virtual IntVector2D getSize() const = 0;
	virtual IntVector2D convertToIntVector(QVector2D const &pos) const = 0;
	virtual IntVector2D correctPositionAndConvertToIntVector(QVector2D const &pos) const = 0;
	virtual void correctPosition(IntVector2D &pos) const = 0;
	virtual void correctPosition(QVector2D& pos) const = 0;
	virtual void correctDisplacement(QVector2D &displacement) const = 0;
};