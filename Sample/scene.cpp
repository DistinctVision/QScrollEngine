#include "scene.h"
#include "QScrollEngine/QScrollEngineWidget.h"
#include <cmath>

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
    widget->backgroundColor = QColor(0, 150, 255);//Пусть фон будет голубым.
    setAmbientColor(100, 100, 100);
    widget->camera->setPosition(QVector3D(-8.0f, 1.0f, -1.0f));
    widget->camera->setOrientation(QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -90.0f));

    //Загрузим модель куба.
    //Параметр path - путь до модели.
    //Параметр textureDir - католог с текстурами модели, если его не указать, то текстуры, связанные с моделью не будут загружены.
    //Параметр prefixTextureName - текстуры, которые загрузяться, получат дополнительно к своему имени это префикс.
    //Так сделано для того, чтобы можно было по имени обращаться к конкретной текстуре модели.
    //Здесь используются параметры по умолчанию, а значит текстуры не будут загружены. Не беда - тут у нас их нет.
    QEntity* ship = widget->loadEntity(":/GameData/ship.3DS");
    ship->setParentScene(this);//Помещаем модель в нашу сцену.
    ship->setScale(0.01f);
    ship->setPosition(-3.0f, 0.5f, -1.0f);
    ship->setOrientation(QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -90.0));

    //Формат 3ds - довольно ограниченный, и длина имени текстуры не может быть длиннее 12 символов.
    //А значит длины имени текстуры вполне может не хватить для указания файлового имени.
    //Но можно применить такой трюк - загрузить нужную текстуру заранее, и задать ей имя, которое доджна получить тектура,
    //загружаемая с моделью. Движок найдет ее у себя и использует ее.
    widget->loadTexture("CubeTexture_CUBE.JPG", ":/GameData/cube.jpg");
    QEntity* cube = widget->loadEntity(":/GameData/cube.3DS", "", "CubeTexture_");
    cube->setParentScene(this);//Помещаем модель в нашу сцену.
    cube->setPosition(-1.0f, 0.6f, 0.0f);
    cube->setScale(1.0f);

    //Создадим источник освещения
    QLight* light = new QLight(this);
    light->setRadius(5000.0f);
    light->setPosition(-2000.0f, 500.0f, 0.0f);

    m_planarShadows.setScene(this);//Нужно обязательно указать сцену.
    m_planarShadows.setAlpha(true);//Пусть тени будут иметь возможность быть прозрачными.
    m_planarShadows.setColorShadows(QColor(0, 0, 0, 155));//Пусть будут черными и прозрачными.
    m_planarShadows.setLight(light);
    //Теперь укажем параметры плоскости, на которой будут находится тени.
    m_planarShadows.setPlanePos(QVector3D(0.0f, 0.0f, 0.0f));
    m_planarShadows.setPlaneDir(QVector3D(0.0f, 1.0f, 0.0f));
    //Укажем списко объектов, отбрасывающих тень.
    m_planarShadows.addEntity(ship);
    m_planarShadows.addEntity(cube);

    //Создадим под-сцену - фон.
    m_background = new QScene(widget, -1);//Параметр order равен -1, а значит эта сцена будет рисоваться позади всех.
    m_background->scale = QVector3D(0.0f, 0.0f, 0.0f);//Параметр сцена scale равен 0,
    //а значит сцена будет относительно камеры бесконечно большой.

    //Создадим солнце на сцене, выполняющую роль фона.
    QEntity* sun = new QEntity(m_background);
    sun->addPart(widget, QGLPrimitiv::Primitives::Sphere, QShPtr(new QSh_Color(QColor(255, 255, 0))));
    //Добавим в объект стандарнтую модель сферы,
    //и установим ей шейдер, рисующей ее одним цветом (желтым).
    sun->setPosition(QVector3D(-4.0f, 1.0f, 0.0f));//И пусть на деле солнце всегда будет на расстоянии "вытянутой руки",
    //так как параметр сцены равен нулю, то камера никогда не приблизиться к солнцу.
    sun->setScale(1.0f);

    _initIsoSurface();
}

Scene::~Scene()
{
    //По идее нужно удалить все остальные созданные объекты, на так как они все привязаны к сценам,
    //то при удалении сцен удалятся и привязанные объекты
}

void Scene::_initIsoSurface()
{
    using namespace std;
    using namespace QScrollEngine;

    m_scalarField = [] (const QVector3D& point) -> float {
        GLfloat scale = 6.0f;
        QVector3D spoint = scale * point;
        return (cos(spoint.x()) + cos(spoint.y())) - sin(spoint.z());
    };

    m_isoSurface.setRegion(QVector3D(-1.0f, -1.0f, -1.0f), QVector3D(1.0f, 1.0f, 1.0f));
    m_isoSurface.setCellSize(0.15f);
    m_isoSurface.setEpsilon(0.05f);
    m_isoSurface.setTValue(0.0f);
    m_isoEntity = new QEntity(this);
    QEntity::Part* part = m_isoEntity->addPart(new QMesh(this), QShPtr(new QSh_LightVC()));
    part->mesh()->enableVertexAttribute(QSh::VertexAttributes::TextureCoords);
    part->mesh()->enableVertexAttribute(QSh::VertexAttributes::Normals);
    part->mesh()->enableVertexAttribute(QSh::VertexAttributes::RgbColors);
    m_isoEntity->setPosition(0.0f, 1.0f, -3.0f);

    m_planarShadows.addEntity(m_isoEntity);
}

void Scene::_updateIsoSurface()
{
    using namespace std;
    using namespace QScrollEngine;
    QMesh* mesh = m_isoEntity->part(0)->mesh();
    m_isoSurface.setTValue(std::fmod(m_isoSurface.tValue() + 0.01f, 3.0f));
    m_isoSurface.isoApproximate(mesh, m_scalarField, true);
    mesh->textureCoords().resize(mesh->vertices().size(), QVector2D(0.0f, 0.0f));
    mesh->colors().resize(mesh->vertices().size());
    for (std::size_t i = 0; i < mesh->countVertices(); ++i) {
        QVector3D n = mesh->normals()[i];
        n = QVector3D(1.0f - n.x() * n.x() * n.x(),
                      1.0f - n.y() * n.y() * n.y(),
                      1.0f - n.z() * n.z() * n.z());
        mesh->colors()[i] = QMesh::RgbColor(fabs(n.x()), fabs(n.y()), fabs(n.z()));
    }
    mesh->updateLocalBoundingBox();
    mesh->applyChanges();
}

void Scene::slotKeyPress(int keyCode)
{
    m_keys.insert(keyCode);
}

void Scene::slotKeyRelease(int keyCode)
{
    m_keys.remove(keyCode);
}

void Scene::slotTouchPress(const QTouchEvent::TouchPoint& touchPoint)
{
    m_mousePressed = true;
    m_prevMousePos = m_currentMousePos = touchPoint.pos();
}

void Scene::slotTouchMove(const QTouchEvent::TouchPoint& touchPoint)
{
    m_mousePressed = true;
    m_currentMousePos = touchPoint.pos();
}

void Scene::slotTouchRelease(const QTouchEvent::TouchPoint& touchPoint)
{
    Q_UNUSED(touchPoint);
    m_mousePressed = false;
}

void Scene::slotBeginDrawing()
{
    using namespace QScrollEngine;
    //Слот вызывается перед отрисовкой всех объектов сцены.
    //Нужно понимать, что сцены не обязательно будут рисоваться последовательно.
    QScene::CameraInfo info = cameraInfo();//Информация о положении камеры в сцене
    QCamera3D* camera = parentContext()->camera;
    if (m_keys.contains(Qt::Key_Up))
        camera->setPosition(camera->position() - info.localZ * 0.1f);
    if (m_keys.contains(Qt::Key_Down))
        camera->setPosition(camera->position() + info.localZ * 0.1f);
    if (m_keys.contains(Qt::Key_Left))
        camera->setPosition(camera->position() - info.localX * 0.05f);
    if (m_keys.contains(Qt::Key_Right))
        camera->setPosition(camera->position() + info.localX * 0.05f);
    if (m_mousePressed) {
        float deltaMouseX = m_prevMousePos.x() - m_currentMousePos.x();
        camera->setOrientation(camera->orientation() *
                               QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, deltaMouseX * 0.1f));
        m_prevMousePos = m_currentMousePos;
    }

    _updateIsoSurface();
}

void Scene::slotEndDrawing()
{
    //Слот просто создан для примера и вызывается после отрисовки всех объектов сцены.
}

