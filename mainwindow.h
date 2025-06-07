// 2dsim08/mainwindow.h V57
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QThreadPool>
#include <QMutex>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QRandomGenerator>
#include <QVector>

// === Simple Enums ===
enum TerrainType {
    TERRAIN_NONE = 0,
    TERRAIN_FOLIAGE = 1,
    TERRAIN_SAND = 2,
    TERRAIN_WATER = 3
};

enum CreatureState {
    STATE_SEEKING_HERD,      // Looking for another creature to herd with
    STATE_MOVING_TO_HERD,    // Moving toward herd target
    STATE_FINDING_SPACE,     // Trying to find elbow room (avoiding overlap)
    STATE_RESTING,           // Socially satisfied, resting
    STATE_ALPHA_TRAVELING,   // Alpha moving to chosen destination
    STATE_ALPHA_RESTING      // Alpha resting at destination
};

// === Simple Structs (herding behavior with alphas) ===
struct SimpleCreature {
    // Position and movement
    qreal posX;
    qreal posY;
    qreal newX;
    qreal newY;
    qreal speed;
    qreal originalSpeed;
    qreal size;

    // Alpha system (Scheme 3)
    bool isAlpha;
    SimpleCreature* myAlpha;         // Which alpha do I follow?
    qreal alphaTargetX;              // For alphas: destination X
    qreal alphaTargetY;              // For alphas: destination Y
    int alphaRestingTime;            // For alphas: how long to rest at destination

    // Herding system (within herd only now)
    SimpleCreature* herdTarget;
    bool hasHerdTarget;
    int restingTimeLeft;
    qreal herdingRange;      // How close to get to herd target
    qreal elbowRoomRange;    // Personal space distance

    // Graphics and state
    QGraphicsEllipseItem* graphicsItem;
    QColor color;
    CreatureState state;
    bool exists;
    int uniqueID;
};

struct SimpleTerrain {
    TerrainType type;
    int density;
    QColor color;
    QGraphicsRectItem* graphicsItem;
    bool initialized;
};

// === Custom GraphicsView (from 2dsim07) ===
class CustomGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    CustomGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);
    void zoomAllTheWayOut();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void zoom(int inOrOut);
    void zoomOverMouse(int inOrOut, QPoint mousePos);

    qreal mCurrentScaleFactor;
    qreal mWASDdelta;
    static const int ZOOM_IN = 1;
    static const int ZOOM_OUT = -1;
};

// === Main Window Class ===
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Thread-safe method to append text to the output
    void appendOutput(const QString& text);

    // Public member for global access
    bool mDebugOutputEnabled;

    // === Constants ===
    static const int VECTOR_SIZE = 1000000;
    static const int USE_PCT_CORE = 95;  // Increased from 80 to 95
    static const int WORLD_SCENE_WIDTH = 100000;
    static const int WORLD_SCENE_HEIGHT = 56250;
    static const int NUM_TERRAIN_COLS = 100;
    static const int NUM_TERRAIN_ROWS = 56;
    static const int TERRAIN_SIZE = WORLD_SCENE_WIDTH / 100;
    static const int STARTING_CREATURE_COUNT = 2000;  // Good number for multiple herds
    static const int ALPHA_RATIO = 25;                // 1 alpha per 25 creatures
    static const int DEFAULT_CREATURE_SIZE = 200;
    static const int CREATURES_UPDATED_PER_TICK = 1000;

private slots:
    void runSimulation();
    void clearOutput();
    void toggleDebugOutput();
    void eventLoopTick();

private:
    // === GUI Components ===
    QLabel* statusLabel;
    QPushButton* startButton;
    QPushButton* clearButton;
    QPushButton* debugToggleButton;
    QTextEdit* outputText;

    // === Graphics Components ===
    CustomGraphicsView* mWorldView;
    QGraphicsScene* mWorldScene;

    // === Threading ===
    QThreadPool* m_threadPool;
    QMutex outputMutex;

    // === Game Loop ===
    QTimer mEventLoopTimer;
    bool mSimulationRunning;
    int mCurrentCreatureIndex;

    // === Game Data (Qt containers) ===
    QVector<SimpleCreature*> mCreatures;
    QVector<QVector<SimpleTerrain*>> mTerrain2D;

    // === Metronome ===
    QGraphicsRectItem* mMetronome;
    int mMetronomeRotation;
    bool mMetronomeEnabled;

    // === Setup Methods ===
    void setupGUI();
    void setupGraphics();
    void setupTerrain();
    void setupCreatures();
    void setupEventLoop();

    // === Game Loop Methods ===
    void updateCreaturesParallel();
    void updateGraphics();
    void moveMetronome();

    // === Creature Methods ===
    SimpleCreature* createCreature(qreal x, qreal y);
    void findHerdTarget(SimpleCreature* creature);
    void assignCreatureToNearestAlpha(SimpleCreature* creature);
    void updateAlphaBehavior(SimpleCreature* alpha);
    qreal distanceBetween(qreal x1, qreal y1, qreal x2, qreal y2);

    // === Terrain Methods ===
    SimpleTerrain* createTerrain(int col, int row, TerrainType type);
    void setTerrainColor(SimpleTerrain* terrain);
    TerrainType findTerrainTypeByXY(qreal x, qreal y);

    // === Utility Methods ===
    void printCreatureSample(const QString& label);
    bool isValidCoordinate(qreal x, qreal y);
    QColor getRandomColor();
    int getUniqueID();
};

#endif // MAINWINDOW_H
