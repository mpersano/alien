#pragma once

#include <QGraphicsItem>

#include "EngineInterface/Definitions.h"
#include "Gui/Definitions.h"

class AbstractItem
	: public QGraphicsItem
{
public:
	AbstractItem(QGraphicsItem *parent = nullptr) : QGraphicsItem(parent) {}
	virtual ~AbstractItem() = default;

	virtual void moveBy(QVector2D const &delta);
};
