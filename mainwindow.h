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
    STATE_MOVING,
    STATE_RESTING,
    STATE_EATING
};

// === Simple Structs (like 2dsim07 style) ===
struct SimpleCreature {
    qreal posX;
    qreal posY;
    qreal newX;
    qreal newY;
    qreal speed;
    qreal size;
    qreal health;
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
    static const int USE_PCT_CORE = 80;
    static const int WORLD_SCENE_WIDTH = 100000;
    static const int WORLD_SCENE_HEIGHT = 56250;
    static const int NUM_TERRAIN_COLS = 100;
    static const int NUM_TERRAIN_ROWS = 56;
    static const int TERRAIN_SIZE = WORLD_SCENE_WIDTH / 100;
    static const int STARTING_CREATURE_COUNT = 1000;
    static const int DEFAULT_CREATURE_SIZE = 200;
    static const int CREATURES_UPDATED_PER_TICK = 100;

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

    // === Game Data (Qt containers like 2dsim07) ===
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
    void updateCreaturePosition(SimpleCreature* creature);
    void updateCreatureColor(SimpleCreature* creature);
    void deleteCreature(SimpleCreature* creature);

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
