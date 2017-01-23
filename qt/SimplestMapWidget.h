#ifndef SIMPLESTMAPWIDGET_H
#define SIMPLESTMAPWIDGET_H

#include <QWidget>
#include <QtWidgets/QOpenGLWidget>
#include <map/feature_vec_model.hpp>
#include <platform/local_country_file.hpp>

#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/drape_engine.hpp"

#include "qtoglcontext.hpp"
#include "qtoglcontextfactory.hpp"
namespace qt {
class SimplestMapWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit SimplestMapWidget(QWidget *parent = 0);

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int width, int height) override;

private:
    void CreateEngine();
    void onUpdateCurrentCountry(m2::PointF const & pt, int zoomLevel);

    platform::LocalCountryFile readFile(std::string name);

    drape_ptr<QtOGLContextFactory> m_contextFactory;
    qreal m_ratio;
    unique_ptr<gui::Skin> m_skin;
    StringsBundle m_stringsBundle;
    bool m_inited = false;

    location::EMyPositionMode m_initialMyPositionState = location::PendingPosition;

    model::FeaturesFetcher m_model;
    drape_ptr<df::DrapeEngine> m_drapeEngine;
};
}

#endif // SIMPLESTMAPWIDGET_H
