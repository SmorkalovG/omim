#include "qt/qtoglcontextfactory.hpp"

#include "base/assert.hpp"
#include <iostream>
#include <QtCore/QThread>
#include <QtCore/QMutexLocker>

QtOGLContextFactory::QtOGLContextFactory(QOpenGLContext * rootContext)
  : m_rootContext(rootContext)
  , m_drawContext(nullptr)
  , m_uploadContext(nullptr)
  , initLock(new QMutex())
{
    initLock->lock();
    std::cout << "root context = " << rootContext << std::endl;
  m_uploadSurface = createSurface();
  m_drawSurface = createSurface();
}

QtOGLContextFactory::~QtOGLContextFactory()
{
    delete initLock;
  delete m_drawContext;
  delete m_uploadContext;

  m_drawSurface->destroy();
  m_uploadSurface->destroy();

  delete m_drawSurface;
  delete m_uploadSurface;
}

bool QtOGLContextFactory::LockFrame()
{
  if (m_drawContext == nullptr)
    return false;

  m_drawContext->lockFrame();
  return true;
}

QRectF const & QtOGLContextFactory::GetTexRect() const
{
  ASSERT(m_drawContext != nullptr, ());
  return m_drawContext->getTexRect();
}

GLuint QtOGLContextFactory::GetTextureHandle() const
{
  ASSERT(m_drawContext != nullptr, ());
  return m_drawContext->getTextureHandle();
}

void QtOGLContextFactory::UnlockFrame()
{
  ASSERT(m_drawContext != nullptr, ());
  m_drawContext->unlockFrame();
}

void QtOGLContextFactory::onInitFinished()
{
    initLock->unlock();
}

dp::OGLContext * QtOGLContextFactory::getDrawContext()
{
  if (m_drawContext == nullptr)
    m_drawContext = new QtRenderOGLContext(m_rootContext, m_drawSurface);

  std::cout << "draw context = " << m_drawContext << std::endl;
  return m_drawContext;
}

dp::OGLContext * QtOGLContextFactory::getResourcesUploadContext()
{
  if (m_uploadContext == nullptr)
    m_uploadContext = new QtUploadOGLContext(m_rootContext, m_uploadSurface);

  std::cout << "upload context = " << m_uploadContext << std::endl;
  return m_uploadContext;
}

void QtOGLContextFactory::waitForInitialization()
{
    QMutexLocker _guard(initLock);
//    QThread::sleep(2);
}

QOffscreenSurface * QtOGLContextFactory::createSurface()
{
  QSurfaceFormat format = m_rootContext->format();
  QOffscreenSurface * result = new QOffscreenSurface(m_rootContext->screen());
  result->setFormat(format);
  result->create();
  ASSERT(result->isValid(), ());

  return result;
}
