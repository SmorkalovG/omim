#include "SimplestMapWidget.h"

#include <iostream>
#include <QOpenGLShaderProgram>
#include <QtCore/QTimer>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContextGroup>
#include <QtGui/QOpenGLFunctions>

using namespace qt;

SimplestMapWidget::SimplestMapWidget(QWidget *parent)
  : QOpenGLWidget(parent)
  , m_contextFactory(nullptr)
  , m_ratio(1.0)
{
    // Init strings bundle.
    // @TODO. There are hardcoded strings below which are defined in strings.txt as well.
    // It's better to use strings form strings.txt intead of hardcoding them here.
    m_stringsBundle.SetDefaultString("placepage_unknown_place", "Unknown Place");
    m_stringsBundle.SetDefaultString("my_places", "My Places");
    m_stringsBundle.SetDefaultString("routes", "Routes");
    m_stringsBundle.SetDefaultString("wifi", "WiFi");

    m_stringsBundle.SetDefaultString("routing_failed_unknown_my_position", "Current location is undefined. Please specify location to create route.");
    m_stringsBundle.SetDefaultString("routing_failed_has_no_routing_file", "Additional data is required to create the route. Download data now?");
    m_stringsBundle.SetDefaultString("routing_failed_start_point_not_found", "Cannot calculate the route. No roads near your starting point.");
    m_stringsBundle.SetDefaultString("routing_failed_dst_point_not_found", "Cannot calculate the route. No roads near your destination.");
    m_stringsBundle.SetDefaultString("routing_failed_cross_mwm_building", "Routes can only be created that are fully contained within a single map.");
    m_stringsBundle.SetDefaultString("routing_failed_route_not_found", "There is no route found between the selected origin and destination.Please select a different start or end point.");
    m_stringsBundle.SetDefaultString("routing_failed_internal_error", "Internal error occurred. Please try to delete and download the map again. If problem persist please contact us at support@maps.me.");

    m_model.InitClassificator();
    m_model.RegisterMap(readFile(std::string("World")));
    m_model.RegisterMap(readFile(std::string("WorldCoasts")));
    std::cout << "map registered" << std::endl;

    QTimer * timer = new QTimer(this);
    VERIFY(connect(timer, SIGNAL(timeout()), this, SLOT(update())), ());
    timer->setSingleShot(false);
    timer->start(30);

    connect(this, &QOpenGLWidget::frameSwapped, [this](){
        if (!m_inited) {
          m_contextFactory->onInitFinished();
        }
        m_inited = true;
    });
}

void SimplestMapWidget::CreateEngine()
{
    double surfaceWidth = m_ratio * width();
    double surfaceHeight = m_ratio * height();
    gui::TWidgetsInitInfo widgetsInitInfo;
    double visualScale = m_ratio;

    m_skin.reset(new gui::Skin(gui::ResolveGuiSkinFile("default"), m_ratio));
    m_skin->Resize(surfaceWidth, surfaceHeight);
    m_skin->ForEach([&widgetsInitInfo](gui::EWidget widget, gui::Position const & pos)
    {
      widgetsInitInfo[widget] = pos;
    });

    widgetsInitInfo[gui::WIDGET_SCALE_LABEL] = gui::Position(dp::LeftBottom);
    auto oglfactory = make_ref(new dp::ThreadSafeFactory(m_contextFactory.get()));

    auto idReadFn = [this](df::MapDataProvider::TReadCallback<FeatureID> const & fn,
                           m2::RectD const & r, int scale) -> void
    {
      m_model.ForEachFeatureID(r, fn, scale);
    };

    auto featureReadFn = [this](df::MapDataProvider::TReadCallback<FeatureType> const & fn,
                                vector<FeatureID> const & ids) -> void
    {
      m_model.ReadFeatures(fn, ids);
    };

    auto isCountryLoadedByNameFn = bind(&model::FeaturesFetcher::IsLoaded, &m_model, _1);
    auto updateCurrentCountryFn = bind(&SimplestMapWidget::onUpdateCurrentCountry, this, _1, _2);

    df::DrapeEngine::Params p(oglfactory,
                              make_ref(&m_stringsBundle),
                              df::Viewport(0, 0, surfaceWidth, surfaceHeight),
                              df::MapDataProvider(idReadFn, featureReadFn, isCountryLoadedByNameFn, updateCurrentCountryFn),
                              visualScale, std::move(widgetsInitInfo),
                              make_pair(m_initialMyPositionState, false),
                              /*allow3dBuildings*/true, /*params.m_isChoosePositionMode*/false,
                              /*params.m_isChoosePositionMode*/false, std::vector<m2::TriangleD>(), /*isFirstLaunch*/true,
                              /*m_routingSession.IsActive()*/false, /*isAutozoomEnabled*/true);

    m_drapeEngine = make_unique_dp<df::DrapeEngine>(std::move(p));

}

void SimplestMapWidget::initializeGL()
{
    ASSERT(m_contextFactory == nullptr, ());
    m_ratio = devicePixelRatio();
    m_contextFactory.reset(new QtOGLContextFactory(context()));

    CreateEngine();
    std::cout << "OpenGLContext::supportsThreadedOpenGL():" << QOpenGLContext::supportsThreadedOpenGL() << std::endl;
}

void SimplestMapWidget::paintGL()
{
  std::cout << "paintGL:" << std::endl;
  static QOpenGLShaderProgram * program = nullptr;
  if (m_contextFactory->LockFrame())
  {
      std::cout << "paintGL: locked" << std::endl;
      if (program == nullptr)
      {
        const char * vertexSrc = "\
          attribute vec2 a_position; \
          attribute vec2 a_texCoord; \
          uniform mat4 u_projection; \
          varying vec2 v_texCoord; \
          \
          void main(void) \
          { \
            gl_Position = u_projection * vec4(a_position, 0.0, 1.0);\
            v_texCoord = a_texCoord; \
          }";

        const char * fragmentSrc = "\
          #ifdef GL_ES \n\
            #ifdef GL_FRAGMENT_PRECISION_HIGH \n\
              #define MAXPREC mediump \n\
            #else \n\
              #define MAXPREC highp \n\
            #endif \n\
            precision MAXPREC float; \n\
          #endif \n\
          uniform sampler2D u_sampler; \
          varying vec2 v_texCoord; \
          \
          void main(void) \
          { \
            gl_FragColor = vec4(texture2D(u_sampler, v_texCoord).rgb, 1.0); \
          }";

        program = new QOpenGLShaderProgram(this);
        program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSrc);
        program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSrc);
        program->link();
        std::cout << "Shader done" << std::endl;
      }

    QOpenGLFunctions* funcs = context()->functions();
    funcs->glActiveTexture(GL_TEXTURE0);
    GLuint image = m_contextFactory->GetTextureHandle();
    funcs->glBindTexture(GL_TEXTURE_2D, image);

    int projectionLocation = program->uniformLocation("u_projection");
    int samplerLocation = program->uniformLocation("u_sampler");

    QMatrix4x4 projection;
    QRect r = rect();
    r.setWidth(m_ratio * r.width());
    r.setHeight(m_ratio * r.height());
    projection.ortho(r);

    program->bind();
    program->setUniformValue(projectionLocation, projection);
    program->setUniformValue(samplerLocation, 0);

    float const w = m_ratio * width();
    float h = m_ratio * height();

    QVector2D positions[4] =
    {
      QVector2D(0.0, 0.0),
      QVector2D(w, 0.0),
      QVector2D(0.0, h),
      QVector2D(w, h)
    };

    QRectF const & texRect = m_contextFactory->GetTexRect();
    QVector2D texCoords[4] =
    {
      QVector2D(texRect.bottomLeft()),
      QVector2D(texRect.bottomRight()),
      QVector2D(texRect.topLeft()),
      QVector2D(texRect.topRight())
    };

    program->enableAttributeArray("a_position");
    program->enableAttributeArray("a_texCoord");
    program->setAttributeArray("a_position", positions, 0);
    program->setAttributeArray("a_texCoord", texCoords, 0);

    funcs->glClearColor(0.0, 1.0, 0.0, 1.0);
    funcs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    funcs->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    funcs->glFinish();
    m_contextFactory->UnlockFrame();
//    std::cout << "unlock frame" << std::endl;
  }
}

void SimplestMapWidget::resizeGL(int width, int height)
{

}

void SimplestMapWidget::onUpdateCurrentCountry(m2::PointF const & pt, int zoomLevel)
{}

platform::LocalCountryFile SimplestMapWidget::readFile(std::string name)
{
    std::cout << "reading file" << std::endl;
    platform::LocalCountryFile res("C:/Users/g.smorkalov/Documents/Projects/build-omim-Desktop_Qt_5_7_0_MinGW_32bit-Debug/out/debug/data/", platform::CountryFile(name), 0);
    std::cout << "sync file" << std::endl;
    res.SyncWithDisk();
    std::cout << "sync file done" << std::endl;
    return res;
}
