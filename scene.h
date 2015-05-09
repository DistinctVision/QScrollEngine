#ifndef SCENE_H
#define SCENE_H

#include "QScrollEngine/QScrollEngine.h"
#include "QScrollEngine/Tools/PlanarShadows.h"
#include <QSet>
#include <QTouchEvent>

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
    QScrollEngine::PlanarShadows _planarShadows;//При помощи этого объекта создадим плоские тени.
    QScene* _background;//В нашей сцене создадим под-сцену - фон.
    QSet<int> _keys;//какие клвиши в данный момент нажаты.
    bool _mousePressed;//нажата ли левая кнопка мышки
    //Предыдущее и текущее положение мышки
    QPointF _prevMousePos;
    QPointF _currentMousePos;
};

#endif // SCENE_H
