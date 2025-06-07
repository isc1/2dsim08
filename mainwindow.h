// 2dsim08/mainwindow.h V202506070700 - Alpha-Led Multi-Herd System with Housekeeping
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
    STATE_SEEKING_HERD,      // Looking for another creature in same herd to follow
    STATE_MOVING_TO_HERD,    // Moving toward herd target (same herd member)
    STATE_FINDING_SPACE,     // Trying to find elbow room (avoiding overlap)
    STATE_RESTING,           // Socially satisfied, resting
    STATE_WANDERING,         // Moving to random point near alpha
    STATE_ALPHA_TRAVELING,   // Alpha moving to chosen destination
    STATE_ALPHA_RESTING      // Alpha resting at destination
};

// === Simple Structs (Alpha-Led Multi-Herd System) ===
struct SimpleCreature {
    // Position and movement
    qreal posX;
    qreal posY;
    qreal newX;
    qreal newY;
    qreal speed;
    qreal originalSpeed;
    qreal size;

    // Alpha system
    bool isAlpha;
    SimpleCreature* myAlpha;         // Which alpha do I follow?
    qreal alphaTargetX;              // For alphas: destination X
    qreal alphaTargetY;              // For alphas: destination Y
    int alphaRestingTime;            // For alphas: how long to rest at destination

    // Herding system (within herd only)
    SimpleCreature* herdTarget;      // Random member of same herd to follow
    bool hasHerdTarget;
    int restingTimeLeft;
    qreal herdingRange;      // How close to get to herd target
    qreal elbowRoomRange;    // Personal space distance (dynamically calculated)

    // Wandering system
    qreal wanderTargetX;     // Random wander destination X
    qreal wanderTargetY;     // Random wander destination Y

    // Graphics and state
    QGraphicsEllipseItem* graphicsItem;
    QColor color;            // Herd color (shared among herd members)
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

    // === Housekeeping ===
    static const int HOUSEKEEPING_INTERVAL = 250;
    static const int HOUSEKEEPING_CREATURES_PER_INTERVAL = 100;

    // === World ===
    static const int VECTOR_SIZE = 1000000;
    static const int USE_PCT_CORE = 95;  // Increased from 80 to 95
    static const int WORLD_SCENE_WIDTH = 100000;
    static const int WORLD_SCENE_HEIGHT = 56250;
    static const int NUM_TERRAIN_COLS = 100;
    static const int NUM_TERRAIN_ROWS = 56;
    static const int TERRAIN_SIZE = WORLD_SCENE_WIDTH / 100;
    static const int STARTING_CREATURE_COUNT = 2000;  // Good number for multiple herds
    static const int ALPHA_RATIO = 25;                // 1 alpha per 25 creatures (creates ~80 herds)
    static const int HERD_MIN_SIZE = 10;              // Minimum herd size before splitting
    static const int HERD_MAX_SIZE = 50;              // Maximum herd size before splitting
    static const int HERD_GROUP_FOOTPRINT_SIZE = 1000;   // Distance followers can be from alpha
    static const int DEFAULT_CREATURE_SIZE = 200;
    static const int CREATURES_UPDATED_PER_TICK = 1000;
    static const int CREATURE_RING_WIDTH = 40;        // Ring thickness (visible at normal zoom)

    // Elbow room and behavior constants
    static constexpr qreal ELBOW_ROOM_FACTOR = 2.0;   // 0-10: 0=touching, 10=up to 10x diameter apart
    static const int CREATURE_MIN_REST_TICKS = 100;   // Minimum ticks to rest in place
    static const int CREATURE_MAX_REST_TICKS = 500;   // Maximum ticks to rest in place
    static const int CREATURE_MIN_WANDER_DISTANCE = 500;   // Min distance from alpha to wander
    static const int CREATURE_MAX_WANDER_DISTANCE = 2000;  // Max distance from alpha to wander
    static const int CREATURE_SPEED_SLOW = 40;
    static const int CREATURE_SPEED_NORMAL = 120;
    static const int CREATURE_SPEED_BURST = 400;

    // Alpha behavior constants
    static const int ALPHA_MIN_WANDER_DIST = 3000;    // Min distance for alpha wandering spurts
    static const int ALPHA_MAX_WANDER_DIST = 8000;    // Max distance for alpha wandering spurts
    static const int ALPHA_MIN_REST_DURATION = 20;   // Min ticks for alpha to rest (longer than creatures)
    static const int ALPHA_MAX_REST_DURATION = 200;  // Max ticks for alpha to rest
    static const int ALPHA_SPEED_SLOW = 400;
    static const int ALPHA_SPEED_NORMAL = 800;
    static const int ALPHA_SPEED_BURST = 6000;
    // Alpha wandering distance for normal daily movement
    static const int ALPHA_NORMAL_WANDER_DISTANCE = 2500;  // Creates a 50x50 box around alpha

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

    // === Housekeeping System ===
    int mHousekeepingTickCounter;
    int mHousekeepingCreatureIndex;

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

    // === Housekeeping Methods ===
    void runHousekeeping();

    // === Creature Methods ===
    SimpleCreature* createCreature(qreal x, qreal y, bool isAlpha = false);
    void findHerdTarget(SimpleCreature* creature);
    void assignCreatureToNearestAlpha(SimpleCreature* creature, const QVector<SimpleCreature*>& alphas);
    qreal distanceBetween(qreal x1, qreal y1, qreal x2, qreal y2);

    // === Terrain Methods ===
    SimpleTerrain* createTerrain(int col, int row, TerrainType type);
    void setTerrainColor(SimpleTerrain* terrain);
    TerrainType findTerrainTypeByXY(qreal x, qreal y);

    // === Utility Methods ===
    void printCreatureSample(const QString& label);
    bool isValidCoordinate(qreal x, qreal y);
    QColor getRandomColor();
    QColor getRandomBrightColor();
    QColor generateHerdColor(int alphaID);
    int getUniqueID();
};

#endif // MAINWINDOW_H
