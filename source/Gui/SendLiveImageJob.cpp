#include "SendLiveImageJob.h"

#include <iostream>
#include <sstream>
#include <QBuffer>
#include <QImage>

#include "Base/ServiceLocator.h"
#include "Base/LoggingService.h"

#include "EngineInterface/SimulationAccess.h"

#include "Web/WebAccess.h"

SendLiveImageJob::SendLiveImageJob(
    string const& currentSimulationId,
    string const& currentToken,
    string const& taskId,
    IntVector2D const& pos,
    IntVector2D const& size,
    SimulationAccess* simAccess,
    WebAccess* webAccess,
    QObject* parent)
    : Job(taskId, parent)
    , _currentSimulationId(currentSimulationId)
    , _currentToken(currentToken)
    , _pos(pos)
    , _size(size)
    , _simAccess(simAccess)
    , _webAccess(webAccess)
{
    connect(_simAccess, &SimulationAccess::imageReady, this, &SendLiveImageJob::imageFromGpuReceived);
    connect(_webAccess, &WebAccess::sendProcessedTaskReceived, this, &SendLiveImageJob::serverReceivedImage);
}

void SendLiveImageJob::process()
{
    if (!_isReady) {
        return;
    }

    switch (_state)
    {
    case State::Init:
        requestImage();
        break;
    case State::ImageFromGpuRequested:
        sendImageToServer();
        break;
    case State::ImageToServerSent:
        finish();
        break;
    default:
        break;
    }
}

bool SendLiveImageJob::isFinished() const
{
    return State::Finished == _state;
}

bool SendLiveImageJob::isBlocking() const
{
    return true;
}

void SendLiveImageJob::requestImage()
{
    _image = boost::make_shared<QImage>(_size.x, _size.y, QImage::Format_RGB32);
    auto const rect = IntRect{ _pos, IntVector2D{ _pos.x + _size.x, _pos.y + _size.y } };

    auto loggingService = ServiceLocator::getInstance().getService<LoggingService>();

    std::stringstream stream;
    stream << "Web: processing task " << getId() << ": request image with size " << _size.x << " x " << _size.y;
    loggingService->logMessage(Priority::Important, stream.str());

    _simAccess->requirePixelImage(rect, _image, _mutex);

    _state = State::ImageFromGpuRequested;
    _isReady = false;
}

void SendLiveImageJob::sendImageToServer()
{
    delete _buffer;
    _buffer = new QBuffer(&_encodedImageData);
    _buffer->open(QIODevice::ReadWrite);
    _image->save(_buffer, "PNG");
    _buffer->seek(0);

    _webAccess->sendProcessedTask(_currentSimulationId, _currentToken, getId(), _buffer);

    _state = State::ImageToServerSent;
    _isReady = false;
}

void SendLiveImageJob::finish()
{
    _state = State::Finished;
    _isReady = true;
}

void SendLiveImageJob::imageFromGpuReceived()
{
    if (State::ImageFromGpuRequested != _state) {
        return;
    }
    _isReady = true;
}

void SendLiveImageJob::serverReceivedImage(string taskId)
{
    if (State::ImageToServerSent != _state || taskId != getId()) {
        return;
    }

    auto loggingService = ServiceLocator::getInstance().getService<LoggingService>();

    std::stringstream stream;
    stream << "Web: task " << getId() << " processed";
    loggingService->logMessage(Priority::Important, stream.str());

    _isReady = true;
    delete _buffer;
    _buffer = nullptr;
}
