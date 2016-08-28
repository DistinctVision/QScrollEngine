#ifndef SCENE_H
#define SCENE_H

#include "QScrollEngine/QScrollEngine.h"
#include "QScrollEngine/Tools/QPlanarShadows.h"
#include <QSet>
#include <QTouchEvent>

#include <functional>

class Scene :
        public QScrollEngine::QScene//наследуем от QScene, это один вариантов использования сцены
{
    Q_OBJECT//раз нужны сигналы и слоты, то нужно исполтьзовать эту макро функцию
public:
    Scene(QScrollEngine::QScrollEngineWidget* widget);
    ~Scene();

private slots:
    void slotKeyPress(int keyCode);//слот нажатия на клавишу
    void slotKeyRelease(int keyCode);//отпускания клавиши
    void slotTouchPress(const QTouchEvent::TouchPoint& touchPoint);//нажатие мышки
    void slotTouchMove(const QTouchEvent::TouchPoint& touchPoint);//движения нажатой мышки
    void slotTouchRelease(const QTouchEvent::TouchPoint& touchPoint);//нажатие мышки
    void slotBeginDrawing();//этот слот будет соответствовать началу рисованию сцены
    void slotEndDrawing();//а этот концу рисования

protected:
    QScrollEngine::QPlanarShadows m_planarShadows;//При помощи этого объекта создадим плоские тени.
    QScene* m_background;//В нашей сцене создадим под-сцену - фон.
    QSet<int> m_keys;//какие клвиши в данный момент нажаты.
    bool m_mousePressed;//нажата ли левая кнопка мышки
    //Предыдущее и текущее положение мышки
    QPointF m_prevMousePos;
    QPointF m_currentMousePos;

    std::function<float(const QVector3D& point)> m_scalarField;
    QScrollEngine::QIsoSurface m_isoSurface;
    QScrollEngine::QEntity* m_isoEntity;

    void _initIsoSurface();
    void _updateIsoSurface();
};

#endif // SCENE_H
