﻿#include <QTimer>

#include <EngineInterface/ZoomLevels.h>

#include "MainController.h"
#include "SimulationConfig.h"
#include "GeneralInfoController.h"
#include "Settings.h"
#include "StringHelper.h"

GeneralInfoController::GeneralInfoController(QObject * parent)
	: QObject(parent)
	, _oneSecondTimer(new QTimer(this))
{
    connect(_oneSecondTimer, &QTimer::timeout, this, &GeneralInfoController::oneSecondTimerTimeout);
}

void GeneralInfoController::init(QLabel * infoLabel, MainController* mainController)
{
	_infoLabel = infoLabel;
	_mainController = mainController;
    _oneSecondTimer->stop();
    _oneSecondTimer->start(1000);
    _rendering = Rendering::OpenGL;

    _infoLabel->setFont(GuiSettings::getGlobalFont());
}

void GeneralInfoController::increaseTimestep()
{
	++_tpsCounting;
	updateInfoLabel();
}

void GeneralInfoController::setZoomFactor(double factor)
{
	_zoomFactor = factor;
	updateInfoLabel();
}

void GeneralInfoController::setRestrictedTPS(boost::optional<int> tps)
{
    _restrictedTPS = tps;
    updateInfoLabel();
}

void GeneralInfoController::setRendering(Rendering value)
{
    _rendering = value;
    updateInfoLabel();
}

void GeneralInfoController::oneSecondTimerTimeout()
{
	_tps = _tpsCounting;
	_tpsCounting = 0;
	updateInfoLabel();
}

void GeneralInfoController::updateInfoLabel()
{
    QString renderModeValueString;
    QString worldSizeValueString;
    QString zoomLevelValueString;
    QString timestepValueString;
    QString tpsValueString;

    if (auto config = _mainController->getSimulationConfig()) {
        renderModeValueString = [&] {
            if (Rendering::OpenGL == _rendering && _zoomFactor < 2.0) {
                return QString("pixel");
            } else if (Rendering::OpenGL == _rendering && _zoomFactor >= Const::ZoomLevelForAutomaticVectorViewSwitch) {
                return QString("vector");
            }
            return QString("item-based");
        }();
        worldSizeValueString = QString("%1 x %2").arg(
            StringHelper::generateFormattedIntString(config->universeSize.x, true),
            StringHelper::generateFormattedIntString(config->universeSize.y, true));
        zoomLevelValueString = QString("%1").arg(toInt(_zoomFactor)) + QString(".%1x").arg(toInt(_zoomFactor * 10) % 10);
        timestepValueString = StringHelper::generateFormattedIntString(_mainController->getTimestep(), true);
        tpsValueString = StringHelper::generateFormattedIntString(_tps, true);
    }

    auto renderModeColorString = [&] {
        if (Rendering::OpenGL == _rendering && _zoomFactor < Const::ZoomLevelForAutomaticVectorViewSwitch) {
            return QString("<font color = #FFB080>");
        } else if (Rendering::OpenGL == _rendering && _zoomFactor >= Const::ZoomLevelForAutomaticVectorViewSwitch) {
            return QString("<font color = #B0FF80>");
        }
        return QString("<font color = #80B0FF>");
    }();
    auto colorTextStart = QString("<font color = %1>").arg(Const::CellEditTextColor1.name());
    auto colorDataStart = QString("<font color = %1>").arg(Const::CellEditDataColor1.name());
    auto colorEnd = QString("</font>");

    QString renderingString = colorTextStart + "Rendering: &nbsp;&nbsp;&nbsp;" + colorEnd + renderModeColorString + "<b>"
        + renderModeValueString + "</b>" + colorEnd;
    QString worldSizeString = colorTextStart + "World size: &nbsp;&nbsp;" + colorEnd + colorDataStart + "<b>"
        + worldSizeValueString + "</b>" + colorEnd;
    QString zoomLevelString = colorTextStart + "Zoom level: &nbsp;&nbsp;" + colorEnd + colorDataStart + "<b>"
        + zoomLevelValueString + "</b>" + colorEnd;
    QString timestepString = colorTextStart +  "Time step: &nbsp;&nbsp;&nbsp;" + colorEnd + colorDataStart + "<b>"
        + timestepValueString + "</b>" + colorEnd;
    QString tpsString = colorTextStart +       "Time steps/s: " + colorEnd + colorDataStart + "<b>"
        + tpsValueString + "</b>" + colorEnd;
    if (_restrictedTPS) {
        tpsString += colorDataStart + "&nbsp;(restricted)" + colorEnd;
    }

    auto separator = QString("<br/>");  
    auto infoString = renderingString + separator + worldSizeString + separator + zoomLevelString + separator
        + timestepString + separator + tpsString;
	_infoLabel->setText(infoString);
}
