// 2dsim08/mainwindow.cpp V64
#include "mainwindow.h"
#include <QApplication>
#include <QFont>
#include <QBrush>
#include <QPen>
#include <QRunnable>
#include <QThread>
#include <QElapsedTimer>
#include <QMetaObject>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>

// Make mDebugOutputEnabled accessible to the global function
MainWindow* g_mainWindow = nullptr;

// Helper function for thread-safe output
void appendToOutput(const QString& text) {
    if (g_mainWindow && g_mainWindow->mDebugOutputEnabled) {
        g_mainWindow->appendOutput(text);
    }
}

// === Creature Update Task (like 2dsim08 tasks) ===
class CreatureUpdateTask : public QRunnable {
private:
    QVector<SimpleCreature*>* mCreatures;
    int mStartIndex;
    int mEndIndex;
    int mTaskId;

public:
    CreatureUpdateTask(QVector<SimpleCreature*>* creatures, int start, int end, int taskId)
        : mCreatures(creatures), mStartIndex(start), mEndIndex(end), mTaskId(taskId) {
        setAutoDelete(true);
    }

    void run() override {
        QString startMsg = QString("[Thread %1] Creature Task %2 processing creatures [%3-%4)")
                          .arg((quintptr)QThread::currentThreadId())
                          .arg(mTaskId)
                          .arg(mStartIndex)
                          .arg(mEndIndex);
        appendToOutput(startMsg);

        // Process creatures - HERDING BEHAVIOR
        for (int i = mStartIndex; i < mEndIndex && i < mCreatures->size(); i++) {
            SimpleCreature* creature = (*mCreatures)[i];
            if (creature && creature->exists) {

                // === HERDING STATE MACHINE ===
                switch (creature->state) {
                    case STATE_SEEKING_HERD:
                        // Look for another creature to herd with (will be handled in main thread)
                        break;

                    case STATE_MOVING_TO_HERD:
                        if (creature->herdTarget) {
                            // Calculate direction to herd target
                            qreal dx = creature->herdTarget->posX - creature->posX;
                            qreal dy = creature->herdTarget->posY - creature->posY;
                            qreal distance = sqrt(dx * dx + dy * dy);

                            if (distance > 1.0) {
                                // Move toward herd target
                                qreal moveX = (dx / distance) * creature->speed;
                                qreal moveY = (dy / distance) * creature->speed;
                                creature->newX = creature->posX + moveX;
                                creature->newY = creature->posY + moveY;

                                // Check if close enough to herd
                                if (distance <= creature->herdingRange) {
                                    creature->state = STATE_FINDING_SPACE;
                                }
                            }
                        } else {
                            creature->state = STATE_SEEKING_HERD;
                        }
                        break;

                    case STATE_FINDING_SPACE:
                        // Move away from overlapping creatures (simple avoidance)
                        creature->newX = creature->posX + (QRandomGenerator::global()->bounded(21) - 10); // -10 to +10
                        creature->newY = creature->posY + (QRandomGenerator::global()->bounded(21) - 10);

                        // After moving, check if we can rest
                        creature->state = STATE_RESTING;
                        creature->restingTimeLeft = 100 + QRandomGenerator::global()->bounded(200); // 100-300 ticks
                        break;

                    case STATE_RESTING:
                        // Stay put and count down resting time
                        creature->newX = creature->posX;
                        creature->newY = creature->posY;
                        creature->restingTimeLeft--;

                        if (creature->restingTimeLeft <= 0) {
                            creature->state = STATE_SEEKING_HERD;
                            creature->herdTarget = nullptr;
                            creature->hasHerdTarget = false;
                        }
                        break;
                }

                // Keep creatures in bounds
                if (creature->newX < 0) creature->newX = 0;
                if (creature->newX > MainWindow::WORLD_SCENE_WIDTH) creature->newX = MainWindow::WORLD_SCENE_WIDTH;
                if (creature->newY < 0) creature->newY = 0;
                if (creature->newY > MainWindow::WORLD_SCENE_HEIGHT) creature->newY = MainWindow::WORLD_SCENE_HEIGHT;
            }
        }

        // Simulate some processing time based on core utilization (from 2dsim08)
        if (MainWindow::USE_PCT_CORE < 100) {
            int delayMs = (100 - MainWindow::USE_PCT_CORE) * 0.5;  // Reduced delay multiplier
            QThread::msleep(delayMs);
        }

        QString endMsg = QString("[Thread %1] Creature Task %2 completed")
                        .arg((quintptr)QThread::currentThreadId())
                        .arg(mTaskId);
        appendToOutput(endMsg);
    }
};

// === Custom GraphicsView Implementation (from 2dsim07) ===
CustomGraphicsView::CustomGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent), mCurrentScaleFactor(1.0), mWASDdelta(100.0)
{
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignCenter);
}

void CustomGraphicsView::zoomAllTheWayOut() {
    if (scene()) {
        mCurrentScaleFactor = scene()->sceneRect().width() / width();
        scale(mCurrentScaleFactor, mCurrentScaleFactor);
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

void CustomGraphicsView::zoom(int inOrOut) {
    double scaleFactor = std::pow(1.125, inOrOut);
    scale(scaleFactor, scaleFactor);
    mCurrentScaleFactor = transform().m11();

    if (scene() &&
        scene()->sceneRect().width() * transform().m11() < width() &&
        scene()->sceneRect().height() * transform().m22() < height()) {
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

void CustomGraphicsView::zoomOverMouse(int inOrOut, QPoint mousePos) {
    if (inOrOut == ZOOM_OUT && scene()) {
        if (scene()->sceneRect().width() * transform().m11() <= width() &&
            scene()->sceneRect().height() * transform().m22() <= height()) {
            return;
        }
    }

    QPointF targetViewportPos = mousePos;
    QPointF targetScenePos = mapToScene(mousePos);
    centerOn(targetScenePos);

    double scaleFactor = std::pow(1.125, inOrOut);
    scale(scaleFactor, scaleFactor);
    mCurrentScaleFactor = transform().m11();

    QPointF deltaViewportPos = targetViewportPos - QPointF(viewport()->width() / 2.0, viewport()->height() / 2.0);
    QPointF viewportCenter = mapFromScene(targetScenePos) - deltaViewportPos;
    centerOn(mapToScene(viewportCenter.toPoint()));
}

void CustomGraphicsView::mousePressEvent(QMouseEvent *event) {
    QGraphicsView::mousePressEvent(event);
}

void CustomGraphicsView::wheelEvent(QWheelEvent *event) {
    int inOrOut = ZOOM_IN;
    if (event->delta() < 0) {
        inOrOut = ZOOM_OUT;
    }
    zoomOverMouse(inOrOut, event->pos());
}

void CustomGraphicsView::keyPressEvent(QKeyEvent *event) {
    int key = event->key();
    QPoint viewportCenter = QPoint(viewport()->rect().width()/2, viewport()->rect().height()/2);
    QPointF scenePointAtViewCenter = mapToScene(viewportCenter.x(), viewportCenter.y());
    QPointF newCenter = scenePointAtViewCenter;

    switch(key) {
        case Qt::Key_W:
        case Qt::Key_Up:
            newCenter.setY(scenePointAtViewCenter.y() - mWASDdelta);
            centerOn(newCenter);
            break;
        case Qt::Key_A:
        case Qt::Key_Left:
            newCenter.setX(scenePointAtViewCenter.x() - mWASDdelta);
            centerOn(newCenter);
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            newCenter.setY(scenePointAtViewCenter.y() + mWASDdelta);
            centerOn(newCenter);
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            newCenter.setX(scenePointAtViewCenter.x() + mWASDdelta);
            centerOn(newCenter);
            break;
        default:
            break;
    }
}

void CustomGraphicsView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
}

// === MainWindow Implementation ===
MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
    , mDebugOutputEnabled(true)
    , mSimulationRunning(false)
    , mCurrentCreatureIndex(0)
    , mMetronomeRotation(0)
    , mMetronomeEnabled(true)
{
    setWindowTitle("2dsim08 + 2dsim07 Integration - Multithreaded Creature Simulation");
    setMinimumSize(1000, 700);
    g_mainWindow = this;

    // Setup thread pool (from 2dsim08)
    m_threadPool = new QThreadPool(this);
    int totalCores = QThread::idealThreadCount();
    int usableCores = std::max(1, totalCores > 1 ? totalCores - 1 : 1);
    m_threadPool->setMaxThreadCount(usableCores);

    setupGUI();
    setupGraphics();
    setupTerrain();
    setupCreatures();
    setupEventLoop();

    appendOutput(QString("=== HERDING SIMULATION INITIALIZED ==="));
    appendOutput(QString("Thread pool: %1 cores (of %2 total)").arg(usableCores).arg(totalCores));
    appendOutput(QString("Creatures: %1, Terrain: %2x%3").arg(STARTING_CREATURE_COUNT).arg(NUM_TERRAIN_COLS).arg(NUM_TERRAIN_ROWS));
    appendOutput(QString("World size: %1x%2").arg(WORLD_SCENE_WIDTH).arg(WORLD_SCENE_HEIGHT));
    appendOutput("Use mouse wheel to zoom, WASD to pan. Click Start to begin!");
    appendOutput("=== NEW: Alpha-Led Multi-Herd System Active! ===");
    appendOutput("Dark red = alpha leaders, brown = herd members");
    appendOutput("Alphas wander to random destinations, herd members follow within their herd");
}

MainWindow::~MainWindow() {
    g_mainWindow = nullptr;

    // Clean up creatures
    for (auto* creature : mCreatures) {
        if (creature) {
            delete creature->graphicsItem;
            delete creature;
        }
    }

    // Clean up terrain
    for (auto& row : mTerrain2D) {
        for (auto* terrain : row) {
            if (terrain) {
                delete terrain->graphicsItem;
                delete terrain;
            }
        }
    }
}

void MainWindow::appendOutput(const QString& text) {
    // Always allow direct calls to appendOutput (for important messages)
    QMetaObject::invokeMethod(this, [this, text]() {
        outputText->append(text);
        outputText->ensureCursorVisible();
    }, Qt::QueuedConnection);
}

void MainWindow::setupGUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    statusLabel = new QLabel("Integrated Simulation Ready. Click Start to begin creature movement!");
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("QLabel { background-color: lightblue; padding: 5px; }");

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("Start Simulation");
    clearButton = new QPushButton("Clear Output");
    debugToggleButton = new QPushButton("Debug: ON");

    startButton->setStyleSheet("QPushButton { background-color: lightgreen; padding: 5px; }");
    clearButton->setStyleSheet("QPushButton { background-color: lightyellow; padding: 5px; }");
    debugToggleButton->setStyleSheet("QPushButton { background-color: lightcyan; padding: 5px; }");

    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(debugToggleButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(clearButton);

    outputText = new QTextEdit();
    outputText->setReadOnly(true);
    outputText->setFont(QFont("Courier", 8));
    outputText->setMaximumHeight(120);
    outputText->setStyleSheet("QTextEdit { background-color: black; color: lightgreen; }");

    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::runSimulation);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearOutput);
    connect(debugToggleButton, &QPushButton::clicked, this, &MainWindow::toggleDebugOutput);
}

void MainWindow::setupGraphics() {
    // Create main world scene (like 2dsim07)
    mWorldScene = new QGraphicsScene(0, 0, WORLD_SCENE_WIDTH, WORLD_SCENE_HEIGHT);
    mWorldView = new CustomGraphicsView(mWorldScene, this);

    // Add graphics view to main layout
    layout()->addWidget(mWorldView);
    layout()->addWidget(outputText);

    // Setup metronome visual indicator (from 2dsim07)
    if (mMetronomeEnabled) {
        int metronomeSize = WORLD_SCENE_WIDTH / 200;
        mMetronome = new QGraphicsRectItem(20, 20, metronomeSize, metronomeSize);
        QPen pen(Qt::black, 2);
        mMetronome->setPen(pen);
        mMetronome->setBrush(QBrush(Qt::red));
        mMetronome->setZValue(100);
        mMetronome->setTransformOriginPoint(metronomeSize/2.0, metronomeSize/2.0);
        mWorldScene->addItem(mMetronome);
        appendOutput("Metronome visual indicator created.");
    }

    // Fit view to scene after a short delay
    QTimer::singleShot(200, [this]() {
        mWorldView->fitInView(mWorldScene->sceneRect(), Qt::KeepAspectRatio);
    });
}

void MainWindow::setupTerrain() {
    appendOutput("Setting up terrain...");

    // Initialize 2D terrain vector (like 2dsim07)
    mTerrain2D.resize(NUM_TERRAIN_COLS);
    for (int col = 0; col < NUM_TERRAIN_COLS; col++) {
        mTerrain2D[col].resize(NUM_TERRAIN_ROWS);
        for (int row = 0; row < NUM_TERRAIN_ROWS; row++) {
            mTerrain2D[col][row] = createTerrain(col, row, TERRAIN_FOLIAGE);
        }
    }

    // Add some random water and sand patches
    for (int i = 0; i < 50; i++) {
        int col = QRandomGenerator::global()->bounded(NUM_TERRAIN_COLS);
        int row = QRandomGenerator::global()->bounded(NUM_TERRAIN_ROWS);
        TerrainType type = (QRandomGenerator::global()->bounded(2) == 0) ? TERRAIN_WATER : TERRAIN_SAND;

        SimpleTerrain* terrain = mTerrain2D[col][row];
        terrain->type = type;
        setTerrainColor(terrain);
    }

    appendOutput(QString("Terrain created: %1x%2 = %3 squares").arg(NUM_TERRAIN_COLS).arg(NUM_TERRAIN_ROWS).arg(NUM_TERRAIN_COLS * NUM_TERRAIN_ROWS));
}

void MainWindow::setupCreatures() {
    appendOutput("Creating creatures with herding behavior...");

    // Create creatures
    for (int i = 0; i < STARTING_CREATURE_COUNT; i++) {
        qreal x = QRandomGenerator::global()->bounded(WORLD_SCENE_WIDTH);
        qreal y = QRandomGenerator::global()->bounded(WORLD_SCENE_HEIGHT);
        SimpleCreature* creature = createCreature(x, y);
        mCreatures.push_back(creature);
    }

    appendOutput(QString("Created %1 brown creatures with herding AI").arg(mCreatures.size()));
    printCreatureSample("Initial creature sample:");
}

void MainWindow::setupEventLoop() {
    // Setup event loop timer (like 2dsim07)
    connect(&mEventLoopTimer, &QTimer::timeout, this, &MainWindow::eventLoopTick);
    mEventLoopTimer.setInterval(20); // Increased frequency: 50 FPS instead of 20 FPS
    appendOutput("Event loop configured (20ms interval - 50 FPS).");
}

void MainWindow::runSimulation() {
    if (!mSimulationRunning) {
        mSimulationRunning = true;
        startButton->setText("Stop Simulation");
        statusLabel->setText("Simulation RUNNING - creatures moving west to east, using parallel processing");
        mEventLoopTimer.start();
        appendOutput("=== SIMULATION STARTED ===");
    } else {
        mSimulationRunning = false;
        startButton->setText("Start Simulation");
        statusLabel->setText("Simulation STOPPED - click Start to resume");
        mEventLoopTimer.stop();
        appendOutput("=== SIMULATION STOPPED ===");
    }
}

void MainWindow::clearOutput() {
    outputText->clear();
    if (mDebugOutputEnabled) {
        appendOutput("Output cleared. Ready for next run!");
    }
}

void MainWindow::toggleDebugOutput() {
    mDebugOutputEnabled = !mDebugOutputEnabled;

    if (mDebugOutputEnabled) {
        debugToggleButton->setText("Debug: ON");
        debugToggleButton->setStyleSheet("QPushButton { background-color: lightcyan; padding: 5px; }");
        appendOutput("=== DEBUG OUTPUT ENABLED ===");
        appendOutput("Thread activity messages will be shown.");
    } else {
        debugToggleButton->setText("Debug: OFF");
        debugToggleButton->setStyleSheet("QPushButton { background-color: lightgray; padding: 5px; }");
        // This message will show because we haven't disabled output yet
        appendOutput("=== DEBUG OUTPUT DISABLED ===");
        appendOutput("Thread activity messages are now hidden. Simulation continues silently.");
    }
}

void MainWindow::eventLoopTick() {
    if (!mSimulationRunning) return;

    // Update metronome (visual indicator)
    if (mMetronomeEnabled) {
        moveMetronome();
    }

    // Handle herding target selection in main thread (needs access to creature vector)
    for (auto* creature : mCreatures) {
        if (creature && creature->exists && creature->state == STATE_SEEKING_HERD) {
            findHerdTarget(creature);
        }
    }

    // Update creatures using parallel processing
    updateCreaturesParallel();

    // Update graphics in main thread
    updateGraphics();
}

void MainWindow::updateCreaturesParallel() {
    if (mCreatures.empty()) return;

    int numThreads = m_threadPool->maxThreadCount();
    int chunkSize = qMax(1, mCreatures.size() / numThreads);

    // Create tasks for parallel processing (like 2dsim08)
    for (int i = 0; i < numThreads; i++) {
        int start = i * chunkSize;
        int end = (i == numThreads - 1) ? mCreatures.size() : (i + 1) * chunkSize;
        if (start >= mCreatures.size()) break;

        CreatureUpdateTask* task = new CreatureUpdateTask(&mCreatures, start, end, i);
        m_threadPool->start(task);
    }

    // Wait for all tasks to complete
    while (!m_threadPool->waitForDone(1)) {
        QApplication::processEvents();
    }
}

void MainWindow::updateGraphics() {
    // Update creature graphics in main thread (thread-safe)
    int updated = 0;
    for (int i = mCurrentCreatureIndex; i < mCurrentCreatureIndex + CREATURES_UPDATED_PER_TICK && i < mCreatures.size(); i++) {
        SimpleCreature* creature = mCreatures[i];
        if (creature && creature->exists) {
            // Update position
            creature->posX = creature->newX;
            creature->posY = creature->newY;
            creature->graphicsItem->setPos(creature->posX, creature->posY);

            // Update color based on hunger (DON'T override with random colors!)
            creature->graphicsItem->setBrush(QBrush(creature->color));

            // Check for water collision (like 2dsim07)
            TerrainType terrainType = findTerrainTypeByXY(creature->posX, creature->posY);
            if (terrainType == TERRAIN_WATER) {
                creature->newX = QRandomGenerator::global()->bounded(WORLD_SCENE_WIDTH);
                creature->newY = QRandomGenerator::global()->bounded(WORLD_SCENE_HEIGHT);
            }

            updated++;
        }
    }

    mCurrentCreatureIndex += CREATURES_UPDATED_PER_TICK;
    if (mCurrentCreatureIndex >= mCreatures.size()) {
        mCurrentCreatureIndex = 0;
    }

    // Advance scene
    mWorldScene->advance();
}

void MainWindow::moveMetronome() {
    if (!mMetronome) return;

    // Move metronome around (like 2dsim07)
    qreal dx = WORLD_SCENE_WIDTH * 0.002;
    qreal dy = WORLD_SCENE_HEIGHT * -0.001;
    qreal newX = mMetronome->x() + dx;
    qreal newY = mMetronome->y() + dy;

    if (newX < 0) newX = WORLD_SCENE_WIDTH - 100;
    else if (newX > WORLD_SCENE_WIDTH) newX = 100;
    if (newY < 0) newY = WORLD_SCENE_HEIGHT - 100;
    else if (newY > WORLD_SCENE_HEIGHT) newY = 100;

    mMetronomeRotation += 3;
    if (mMetronomeRotation > 360) mMetronomeRotation -= 360;

    mMetronome->setRotation(mMetronomeRotation);
    mMetronome->setPos(newX, newY);

    // Randomly change color
    if (QRandomGenerator::global()->bounded(100) > 95) {
        QColor newColor = getRandomColor();
        mMetronome->setBrush(QBrush(newColor));
    }
}

// === Creature Methods ===
SimpleCreature* MainWindow::createCreature(qreal x, qreal y) {
    SimpleCreature* creature = new SimpleCreature;

    // Initialize creature data
    creature->posX = x;
    creature->posY = y;
    creature->newX = x;
    creature->newY = y;
    creature->speed = 30 + QRandomGenerator::global()->bounded(40); // 30-70 speed
    creature->originalSpeed = creature->speed;
    creature->size = DEFAULT_CREATURE_SIZE + QRandomGenerator::global()->bounded(50);

    // Herding system
    creature->herdTarget = nullptr;
    creature->hasHerdTarget = false;
    creature->restingTimeLeft = 0;
    creature->herdingRange = creature->size * 2.5;     // Get within 2.5 diameters
    creature->elbowRoomRange = creature->size * 1.2;   // Personal space

    // Start seeking a herd
    creature->state = STATE_SEEKING_HERD;
    creature->color = QColor(139, 69, 19);  // Saddle brown color
    creature->exists = true;
    creature->uniqueID = getUniqueID();

    // Create graphics
    creature->graphicsItem = new QGraphicsEllipseItem(0, 0, creature->size, creature->size);
    creature->graphicsItem->setPos(x, y);
    creature->graphicsItem->setBrush(QBrush(creature->color));
    creature->graphicsItem->setPen(QPen(Qt::transparent));
    creature->graphicsItem->setZValue(10);

    mWorldScene->addItem(creature->graphicsItem);

    return creature;
}

void MainWindow::findHerdTarget(SimpleCreature* creature) {
    if (!creature || creature->hasHerdTarget) return;

    // Find a random creature to herd with
    if (mCreatures.size() > 1) {
        int attempts = 10;  // Don't search forever
        while (attempts > 0) {
            int randomIndex = QRandomGenerator::global()->bounded(mCreatures.size());
            SimpleCreature* potential = mCreatures[randomIndex];

            if (potential && potential != creature && potential->exists) {
                // Check if target is not too far away (reasonable herding distance)
                qreal distance = distanceBetween(creature->posX, creature->posY,
                                               potential->posX, potential->posY);
                if (distance < WORLD_SCENE_WIDTH * 0.3) {  // Within 30% of world width
                    creature->herdTarget = potential;
                    creature->hasHerdTarget = true;
                    creature->state = STATE_MOVING_TO_HERD;
                    break;
                }
            }
            attempts--;
        }
    }
}

qreal MainWindow::distanceBetween(qreal x1, qreal y1, qreal x2, qreal y2) {
    qreal dx = x2 - x1;
    qreal dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

// === Terrain Methods ===
SimpleTerrain* MainWindow::createTerrain(int col, int row, TerrainType type) {
    SimpleTerrain* terrain = new SimpleTerrain;

    terrain->type = type;
    terrain->density = 1;
    terrain->initialized = true;

    // Create graphics
    qreal x = col * TERRAIN_SIZE;
    qreal y = row * TERRAIN_SIZE;
    terrain->graphicsItem = new QGraphicsRectItem(x, y, TERRAIN_SIZE, TERRAIN_SIZE);
    terrain->graphicsItem->setZValue(0);

    setTerrainColor(terrain);
    mWorldScene->addItem(terrain->graphicsItem);

    return terrain;
}

void MainWindow::setTerrainColor(SimpleTerrain* terrain) {
    if (!terrain) return;

    switch (terrain->type) {
        case TERRAIN_FOLIAGE:
            terrain->color = QColor(180, 230, 180); // Light green
            break;
        case TERRAIN_SAND:
            terrain->color = QColor(180, 153, 102); // Sandy brown
            break;
        case TERRAIN_WATER:
            terrain->color = QColor(51, 153, 255);  // Blue
            break;
        default:
            terrain->color = QColor(100, 100, 100); // Gray
            break;
    }

    terrain->graphicsItem->setBrush(QBrush(terrain->color));
    terrain->graphicsItem->setPen(QPen(Qt::transparent));
}

TerrainType MainWindow::findTerrainTypeByXY(qreal x, qreal y) {
    int col = static_cast<int>(x / TERRAIN_SIZE);
    int row = static_cast<int>(y / TERRAIN_SIZE);

    if (col < 0 || col >= NUM_TERRAIN_COLS || row < 0 || row >= NUM_TERRAIN_ROWS) {
        return TERRAIN_FOLIAGE; // Default
    }

    return mTerrain2D[col][row]->type;
}

// === Utility Methods ===
void MainWindow::printCreatureSample(const QString& label) {
    appendOutput(label);
    int sampleSize = qMin(5, mCreatures.size());
    for (int i = 0; i < sampleSize; i++) {
        SimpleCreature* creature = mCreatures[i];
        QString stateStr;
        switch(creature->state) {
            case STATE_SEEKING_HERD: stateStr = "seeking"; break;
            case STATE_MOVING_TO_HERD: stateStr = "moving"; break;
            case STATE_FINDING_SPACE: stateStr = "spacing"; break;
            case STATE_RESTING: stateStr = "resting"; break;
            case STATE_ALPHA_TRAVELING: stateStr = "alpha_travel"; break;
            case STATE_ALPHA_RESTING: stateStr = "alpha_rest"; break;
        }
        appendOutput(QString("  Creature %1: pos(%2,%3) speed=%4 state=%5")
                    .arg(creature->uniqueID)
                    .arg(creature->posX, 0, 'f', 1)
                    .arg(creature->posY, 0, 'f', 1)
                    .arg(creature->speed, 0, 'f', 1)
                    .arg(stateStr));
    }
}

bool MainWindow::isValidCoordinate(qreal x, qreal y) {
    return x >= 0 && x < WORLD_SCENE_WIDTH && y >= 0 && y < WORLD_SCENE_HEIGHT;
}

QColor MainWindow::getRandomColor() {
    int r = QRandomGenerator::global()->bounded(256);
    int g = QRandomGenerator::global()->bounded(256);
    int b = QRandomGenerator::global()->bounded(256);
    return QColor(r, g, b);
}

int MainWindow::getUniqueID() {
    static int nextID = 1;
    return nextID++;
}
