#include "scene.h"
#include "QScrollEngine/QScrollEngineWidget.h"

Scene::Scene(QScrollEngine::QScrollEngineWidget* widget):
    QScene(widget, 0) // обязательно нужно указать QScene какому объекту QScrollEngineContext она принадлежит.
    //А QScrollEngineWidget наследуется от QScrollEngineContext.
    //Параметр order равен 0, а значит ее порядок рисования будет стандартной.
{
    using namespace QScrollEngine;

    //Соединяем нужные сигналы и слоты.
    connect(this, SIGNAL(beginDrawing()), this, SLOT(slotBeginDrawing()));
    connect(this, SIGNAL(endDrawing()), this, SLOT(slotEndDrawing()));
    connect(widget, SIGNAL(keyPress(int)), this, SLOT(slotKeyPress(int)));
    connect(widget, SIGNAL(keyRelease(int)), this, SLOT(slotKeyRelease(int)));
    //В виджете оди и тотже сигнал для нажатия мыши и нажатия тачскина.
    //Тачскрин в коде ниже мы не учитываем, и в таких случаях поддержку тачскрина можно отключить
    //при помощи макро - "#define SUPPORT_TOUCHSCREEN 0". Но в данном случае этого делать не будем.
    connect(widget, SIGNAL(touchPressed(QTouchEvent::TouchPoint)), this, SLOT(slotTouchPress(QTouchEvent::TouchPoint)));
    connect(widget, SIGNAL(touchMoved(QTouchEvent::TouchPoint)), this, SLOT(slotTouchMove(QTouchEvent::TouchPoint)));
    connect(widget, SIGNAL(touchReleased(QTouchEvent::TouchPoint)), this, SLOT(slotTouchRelease(QTouchEvent::TouchPoint)));

    widget->setPostEffectUsed(true);//Используем пост-эффект Bloom.
    widget->backgroundColor = QColor(55, 155, 255);//Пусть фон будет голубым.
    setAmbientColor(100, 100, 100);
    widget->camera->setPosition(QVector3D(0.0f, 1.0f, 5.0f));//поставим камеру назад.

    //Загрузим модель куба.
    //Параметр path - путь до модели.
    //Параметр textureDir - католог с текстурами модели, если его не указать, то текстуры, связанные с моделью не будут загружены.
    //Параметр prefixTextureName - текстуры, которые загрузяться, получат дополнительно к своему имени это префикс.
    //Так сделано для того, чтобы можно было по имени обращаться к конкретной текстуре модели.
    //Здесь используются параметры по умолчанию, а значит текстуры не будут загружены. Не беда - тут у нас их нет.
    QEntity* ship = widget->loadEntity(":/GameData/ship.3DS");
    ship->setParentScene(this);//Помещаем модель в нашу сцену.
    ship->setScale(0.01f);
    ship->setPosition(0.0f, 0.5f, -1.0f);
    ship->setOrientation(QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -90.0));

    //Формат 3ds - довольно ограниченный, и длина имени текстуры не может быть длиннее 12 символов.
    //А значит длины имени текстуры вполне может не хватить для указания файлового имени.
    //Но можно применить такой трюк - загрузить нужную текстуру заранее, и задать ей имя, которое доджна получить тектура,
    //загружаемая с моделью. Движок найдет ее у себя и использует ее.
    widget->loadTexture("CubeTexture_CUBE.JPG", ":/GameData/cube.jpg");
    QEntity* cube = widget->loadEntity(":/GameData/cube.3DS", "", "CubeTexture_");
    cube->setParentScene(this);//Помещаем модель в нашу сцену.
    cube->setPosition(0.0f, 0.6f, 0.0f);
    cube->setScale(1.0f);

    //Создадим источник освещения
    QLight* light = new QLight(this);
    light->setRadius(5000.0f);
    light->setPosition(-2000.0f, 500.0f, 0.0f);

    _planarShadows.setScene(this);//Нужно обязательно указать сцену.
    _planarShadows.setAlpha(true);//Пусть тени будут иметь возможность быть прозрачными.
    _planarShadows.setColorShadows(QColor(0, 0, 0, 155));//Пусть будут черными и прозрачными.
    _planarShadows.setLight(light);
    //Теперь укажем параметры плоскости, на которой будут находится тени.
    _planarShadows.setPlanePos(QVector3D(0.0f, 0.0f, 0.0f));
    _planarShadows.setPlaneDir(QVector3D(0.0f, 1.0f, 0.0f));
    //Укажем списко объектов, отбрасывающих тень.
    _planarShadows.addEntity(ship);
    _planarShadows.addEntity(cube);

    //Создадим под-сцену - фон.
    _background = new QScene(widget, -1);//Параметр order равен -1, а значит эта сцена будет рисоваться позади всех.
    _background->scale = QVector3D(0.0f, 0.0f, 0.0f);//Параметр сцена scale равен 0,
    //а значит сцена будет относительно камеры бесконечно большой.

    //Создадим солнце на сцене, выполняющую роль фона.
    QEntity* sun = new QEntity(_background);
    sun->addPart(widget, QGLPrimitiv::Sphere, new QSh_Color(QColor(255, 255, 0)));//Добавим в объект стандарнтую модель сферы,
    //и установим ей шейдер, рисующей ее одним цветом (желтым).
    sun->setPosition(QVector3D(-4.0f, 1.0f, 0.0f));//И пусть на деле солнце всегда будет на расстоянии "вытянутой руки",
    //так как параметр сцены равен нулю, то камера никогда не приблизиться к солнцу.
    sun->setScale(1.0f);
}

Scene::~Scene()
{
    //По идее нужно удалить все остальные созданные объекты, на так как они все привязаны к сценам,
    //то при удалении сцен удалятся и привязанные объекты
}

void Scene::slotKeyPress(int keyCode)
{
    _keys.insert(keyCode);
}

void Scene::slotKeyRelease(int keyCode)
{
    _keys.remove(keyCode);
}

void Scene::slotTouchPress(const QTouchEvent::TouchPoint& touchPoint)
{
    _mousePressed = true;
    _prevMousePos = _currentMousePos = touchPoint.pos();
}

void Scene::slotTouchMove(const QTouchEvent::TouchPoint& touchPoint)
{
    _mousePressed = true;
    _currentMousePos = touchPoint.pos();
}

void Scene::slotTouchRelease(const QTouchEvent::TouchPoint& touchPoint)
{
    Q_UNUSED(touchPoint);
    _mousePressed = false;
}

void Scene::slotBeginDrawing()
{
    using namespace QScrollEngine;
    //Слот вызывается перед отрисовкой всех объектов сцены.
    //Нужно понимать, что сцены не обязательно будут рисоваться последовательно.
    QScene::CameraInfo info = cameraInfo();//Информация о положении камеры в сцене
    QCamera3D* camera = parentContext()->camera;
    if (_keys.contains(Qt::Key_Up))
        camera->setPosition(camera->position() - info.localZ * 0.1f);
    if (_keys.contains(Qt::Key_Down))
        camera->setPosition(camera->position() + info.localZ * 0.1f);
    if (_keys.contains(Qt::Key_Left))
        camera->setPosition(camera->position() - info.localX * 0.05f);
    if (_keys.contains(Qt::Key_Right))
        camera->setPosition(camera->position() + info.localX * 0.05f);
    if (_mousePressed) {
        camera->setOrientation(camera->orientation() *
                               QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, (_prevMousePos.x() - _currentMousePos.x()) * 0.1f));
        _prevMousePos = _currentMousePos;
    }
}

void Scene::slotEndDrawing()
{
    //Слот просто создан для примера и вызывается после отрисовки всех объектов сцены.
}

