// 2dsim08/mainwindow.cpp V202506070700 - Alpha-Led Multi-Herd System with Housekeeping
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

// === Creature Update Task (Alpha-Led Herding System) ===
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
        QString startMsg = QString("[Thread %1] Alpha Herd Task %2 processing creatures [%3-%4)")
                          .arg((quintptr)QThread::currentThreadId())
                          .arg(mTaskId)
                          .arg(mStartIndex)
                          .arg(mEndIndex);
        appendToOutput(startMsg);

        // Process creatures - ALPHA-LED HERDING BEHAVIOR
        for (int i = mStartIndex; i < mEndIndex && i < mCreatures->size(); i++) {
            SimpleCreature* creature = (*mCreatures)[i];
            if (creature && creature->exists) {

                if (creature->isAlpha) {
                    // === ALPHA BEHAVIOR ===
                    switch (creature->state) {
                        case STATE_ALPHA_TRAVELING:
                            {
                                // Move toward alpha destination
                                qreal dx = creature->alphaTargetX - creature->posX;
                                qreal dy = creature->alphaTargetY - creature->posY;
                                qreal distance = sqrt(dx * dx + dy * dy);

                                if (distance > creature->speed) {
                                    // Keep moving toward destination
                                    qreal moveX = (dx / distance) * creature->speed;
                                    qreal moveY = (dy / distance) * creature->speed;
                                    creature->newX = creature->posX + moveX;
                                    creature->newY = creature->posY + moveY;
                                } else {
                                    // Reached destination, start resting
                                    creature->newX = creature->alphaTargetX;
                                    creature->newY = creature->alphaTargetY;
                                    creature->state = STATE_ALPHA_RESTING;
                                    creature->alphaRestingTime = MainWindow::ALPHA_MIN_REST_DURATION +
                                        QRandomGenerator::global()->bounded(MainWindow::ALPHA_MAX_REST_DURATION - MainWindow::ALPHA_MIN_REST_DURATION);
                                }
                            }
                            break;

                    case STATE_ALPHA_RESTING:
                        // Stay put and count down resting time
                        creature->newX = creature->posX;
                        creature->newY = creature->posY;
                        creature->alphaRestingTime--;

                        if (creature->alphaRestingTime <= 0) {
                            // Pick small random offset from current position for normal wandering
                            qreal offsetX = QRandomGenerator::global()->bounded(MainWindow::ALPHA_NORMAL_WANDER_DISTANCE * 2 + 1) - MainWindow::ALPHA_NORMAL_WANDER_DISTANCE;
                            qreal offsetY = QRandomGenerator::global()->bounded(MainWindow::ALPHA_NORMAL_WANDER_DISTANCE * 2 + 1) - MainWindow::ALPHA_NORMAL_WANDER_DISTANCE;

                            qreal targetX = creature->posX + offsetX;
                            qreal targetY = creature->posY + offsetY;

                            // Keep target within world bounds
                            targetX = qMax(0.0, qMin(static_cast<qreal>(MainWindow::WORLD_SCENE_WIDTH), targetX));
                            targetY = qMax(0.0, qMin(static_cast<qreal>(MainWindow::WORLD_SCENE_HEIGHT), targetY));

                            creature->alphaTargetX = targetX;
                            creature->alphaTargetY = targetY;
                            creature->state = STATE_ALPHA_TRAVELING;
                        }
                        break;

                        default:
                            // Default alpha state - pick initial destination
                            creature->alphaTargetX = QRandomGenerator::global()->bounded(MainWindow::WORLD_SCENE_WIDTH);
                            creature->alphaTargetY = QRandomGenerator::global()->bounded(MainWindow::WORLD_SCENE_HEIGHT);
                            creature->state = STATE_ALPHA_TRAVELING;
                            break;
                    }
                } else {
                    // === HERD MEMBER BEHAVIOR ===
                    // Simple behavior: Rest -> Pick position around alpha -> Move to position -> Rest
                    switch (creature->state) {
                        case STATE_SEEKING_HERD:
                        case STATE_MOVING_TO_HERD:
                        case STATE_FINDING_SPACE:
                            // Simplify: all these states now just go to resting
                            creature->state = STATE_RESTING;
                            creature->restingTimeLeft = MainWindow::CREATURE_MIN_REST_TICKS +
                                QRandomGenerator::global()->bounded(MainWindow::CREATURE_MAX_REST_TICKS - MainWindow::CREATURE_MIN_REST_TICKS);
                            creature->newX = creature->posX;
                            creature->newY = creature->posY;
                            break;

                        case STATE_RESTING:
                            // Stay put and count down resting time
                            creature->newX = creature->posX;
                            creature->newY = creature->posY;
                            creature->restingTimeLeft--;

                            if (creature->restingTimeLeft <= 0) {
                                // Done resting, pick random position around alpha
                                if (creature->myAlpha) {
                                    // Pick random point within HERD_MAX_DIAMETER of alpha
                                    qreal offsetX = QRandomGenerator::global()->bounded(MainWindow::HERD_GROUP_FOOTPRINT_SIZE * 2 + 1) - MainWindow::HERD_GROUP_FOOTPRINT_SIZE; // -300 to +300
                                    qreal offsetY = QRandomGenerator::global()->bounded(MainWindow::HERD_GROUP_FOOTPRINT_SIZE * 2 + 1) - MainWindow::HERD_GROUP_FOOTPRINT_SIZE; // -300 to +300

                                    qreal targetX = creature->myAlpha->posX + offsetX;
                                    qreal targetY = creature->myAlpha->posY + offsetY;

                                    // Keep target within world bounds
                                    targetX = qMax(0.0, qMin(static_cast<qreal>(MainWindow::WORLD_SCENE_WIDTH), targetX));
                                    targetY = qMax(0.0, qMin(static_cast<qreal>(MainWindow::WORLD_SCENE_HEIGHT), targetY));

                                    creature->wanderTargetX = targetX;
                                    creature->wanderTargetY = targetY;
                                    creature->state = STATE_WANDERING;
                                } else {
                                    // No alpha, just pick random point nearby
                                    creature->wanderTargetX = creature->posX + (QRandomGenerator::global()->bounded(2001) - 1000); // -1000 to +1000
                                    creature->wanderTargetY = creature->posY + (QRandomGenerator::global()->bounded(2001) - 1000);
                                    creature->state = STATE_WANDERING;
                                }
                            }
                            break;

                        case STATE_WANDERING:
                            // Move toward wander target (position around alpha)
                            {
                                qreal dx = creature->wanderTargetX - creature->posX;
                                qreal dy = creature->wanderTargetY - creature->posY;
                                qreal distance = sqrt(dx * dx + dy * dy);

                                if (distance > creature->speed) {
                                    // Keep moving toward target position
                                    qreal moveX = (dx / distance) * creature->speed;
                                    qreal moveY = (dy / distance) * creature->speed;
                                    creature->newX = creature->posX + moveX;
                                    creature->newY = creature->posY + moveY;
                                } else {
                                    // Reached target position, start resting again
                                    creature->newX = creature->wanderTargetX;
                                    creature->newY = creature->wanderTargetY;
                                    creature->state = STATE_RESTING;
                                    creature->restingTimeLeft = MainWindow::CREATURE_MIN_REST_TICKS +
                                        QRandomGenerator::global()->bounded(MainWindow::CREATURE_MAX_REST_TICKS - MainWindow::CREATURE_MIN_REST_TICKS);
                                }
                            }
                            break;

                        default:
                            creature->state = STATE_RESTING;
                            break;
                    }
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

        QString endMsg = QString("[Thread %1] Alpha Herd Task %2 completed")
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
    , mHousekeepingTickCounter(0)
    , mHousekeepingCreatureIndex(0)
{
    setWindowTitle("2dsim08 - Alpha-Led Multi-Herd System");
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

    appendOutput(QString("=== ALPHA-LED MULTI-HERD SIMULATION INITIALIZED ==="));
    appendOutput(QString("Thread pool: %1 cores (of %2 total)").arg(usableCores).arg(totalCores));
    appendOutput(QString("Creatures: %1 (with %2 alpha leaders)").arg(STARTING_CREATURE_COUNT).arg(STARTING_CREATURE_COUNT / ALPHA_RATIO));
    appendOutput(QString("Terrain: %1x%2, World size: %3x%4").arg(NUM_TERRAIN_COLS).arg(NUM_TERRAIN_ROWS).arg(WORLD_SCENE_WIDTH).arg(WORLD_SCENE_HEIGHT));
    appendOutput("Use mouse wheel to zoom, WASD to pan. Click Start to begin!");
    appendOutput("=== Each herd has its own unique color! ===");
    appendOutput("Black ring alphas lead white ring herds around the world");

    // Start in fullscreen
    showMaximized();
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

    statusLabel = new QLabel("Alpha-Led Multi-Herd Simulation Ready. Click Start to watch herds form!");
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("QLabel { background-color: lightblue; padding: 5px; }");

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("Start Simulation");
    clearButton = new QPushButton("Clear Output");
    debugToggleButton = new QPushButton("Debug: OFF");  // Changed from "Debug: ON"

    startButton->setStyleSheet("QPushButton { background-color: lightgreen; padding: 5px; }");
    clearButton->setStyleSheet("QPushButton { background-color: lightyellow; padding: 5px; }");
    debugToggleButton->setStyleSheet("QPushButton { background-color: lightgray; padding: 5px; }");  // Changed from lightcyan

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
    appendOutput("Creating alpha-led multi-herd system...");

    // Create alpha creatures first
    QVector<SimpleCreature*> alphas;
    int numAlphas = qMax(1, STARTING_CREATURE_COUNT / ALPHA_RATIO);

    for (int i = 0; i < numAlphas; i++) {
        qreal x = QRandomGenerator::global()->bounded(WORLD_SCENE_WIDTH);
        qreal y = QRandomGenerator::global()->bounded(WORLD_SCENE_HEIGHT);
        SimpleCreature* alpha = createCreature(x, y, true); // true = isAlpha
        alphas.push_back(alpha);
        mCreatures.push_back(alpha);
    }

    // Create regular herd members and assign them to alphas
    for (int i = numAlphas; i < STARTING_CREATURE_COUNT; i++) {
        qreal x = QRandomGenerator::global()->bounded(WORLD_SCENE_WIDTH);
        qreal y = QRandomGenerator::global()->bounded(WORLD_SCENE_HEIGHT);
        SimpleCreature* member = createCreature(x, y, false); // false = not alpha

        // Assign to nearest alpha
        assignCreatureToNearestAlpha(member, alphas);
        mCreatures.push_back(member);
    }

    appendOutput(QString("Created %1 alphas (black rings) leading %2 total creatures").arg(numAlphas).arg(mCreatures.size()));
    appendOutput(QString("Each of %1 herds has its own unique color!").arg(numAlphas));
    printCreatureSample("Alpha and herd sample:");
}

void MainWindow::setupEventLoop() {
    // Setup event loop timer (like 2dsim07)
    connect(&mEventLoopTimer, &QTimer::timeout, this, &MainWindow::eventLoopTick);
    mEventLoopTimer.setInterval(20); // 50 FPS
    appendOutput("Event loop configured (20ms interval - 50 FPS).");
}

void MainWindow::runSimulation() {
    if (!mSimulationRunning) {
        mSimulationRunning = true;
        startButton->setText("Stop Simulation");
        statusLabel->setText("Simulation RUNNING - Watch alphas lead their colored herds around the world!");
        mEventLoopTimer.start();
        appendOutput("=== ALPHA-LED MULTI-HERD SIMULATION STARTED ===");
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

    // Run housekeeping periodically
    mHousekeepingTickCounter++;
    if (mHousekeepingTickCounter >= MainWindow::HOUSEKEEPING_INTERVAL) {
        runHousekeeping();
        mHousekeepingTickCounter = 0; // Reset counter
    }

    // Handle orphan assignment in main thread (needs access to creature vector)
    for (auto* creature : mCreatures) {
        if (creature && creature->exists && !creature->isAlpha && !creature->myAlpha) {
            // This creature needs an alpha - assign to nearest one
            SimpleCreature* nearestAlpha = nullptr;
            qreal nearestDistance = std::numeric_limits<qreal>::max();

            for (auto* potential : mCreatures) {
                if (potential && potential->isAlpha && potential->exists) {
                    qreal distance = distanceBetween(creature->posX, creature->posY, potential->posX, potential->posY);
                    if (distance < nearestDistance) {
                        nearestDistance = distance;
                        nearestAlpha = potential;
                    }
                }
            }

            if (nearestAlpha) {
                creature->myAlpha = nearestAlpha;
                creature->color = generateHerdColor(nearestAlpha->uniqueID);
                creature->graphicsItem->setBrush(QBrush(creature->color));
                creature->graphicsItem->setPen(QPen(Qt::white, CREATURE_RING_WIDTH));
            }
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

            // Keep the assigned herd color (don't randomize!)
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
SimpleCreature* MainWindow::createCreature(qreal x, qreal y, bool isAlpha) {
    SimpleCreature* creature = new SimpleCreature;

    // Initialize creature data
    creature->posX = x;
    creature->posY = y;
    creature->newX = x;
    creature->newY = y;

    // Set speeds based on creature type
    if (isAlpha) {
        creature->speed = ALPHA_SPEED_SLOW;  // Alphas move slowly
    } else {
        creature->speed = CREATURE_SPEED_NORMAL;  // Creatures move at normal speed
    }
    creature->originalSpeed = creature->speed;

    creature->size = DEFAULT_CREATURE_SIZE + QRandomGenerator::global()->bounded(50);

    // Alpha system
    creature->isAlpha = isAlpha;
    creature->myAlpha = nullptr;

    // *** FIX: Initialize alpha targets using small box logic ***
    if (isAlpha) {
        // Give alphas small local destinations using the same box logic as normal wandering
        qreal offsetX = QRandomGenerator::global()->bounded(ALPHA_NORMAL_WANDER_DISTANCE * 2 + 1) - ALPHA_NORMAL_WANDER_DISTANCE;
        qreal offsetY = QRandomGenerator::global()->bounded(ALPHA_NORMAL_WANDER_DISTANCE * 2 + 1) - ALPHA_NORMAL_WANDER_DISTANCE;

        qreal targetX = x + offsetX;  // Use spawn position + small offset
        qreal targetY = y + offsetY;

        // Keep target within world bounds
        targetX = qMax(0.0, qMin(static_cast<qreal>(WORLD_SCENE_WIDTH), targetX));
        targetY = qMax(0.0, qMin(static_cast<qreal>(WORLD_SCENE_HEIGHT), targetY));

        creature->alphaTargetX = targetX;
        creature->alphaTargetY = targetY;
    } else {
        creature->alphaTargetX = 0;  // Non-alphas don't use these
        creature->alphaTargetY = 0;
    }

    creature->alphaRestingTime = 0;

    // Herding system
    creature->herdTarget = nullptr;
    creature->hasHerdTarget = false;
    creature->restingTimeLeft = 0;
    creature->herdingRange = creature->size * 4.0;     // Seek herds within 4 diameters

    // Dynamic elbow room: randomize each creature's personal space preference
    creature->elbowRoomRange = QRandomGenerator::global()->bounded(static_cast<int>(ELBOW_ROOM_FACTOR * 100)) / 100.0; // 0.0 to ELBOW_ROOM_FACTOR

    // Wandering system
    creature->wanderTargetX = 0;
    creature->wanderTargetY = 0;

    creature->exists = true;
    creature->uniqueID = getUniqueID();

    // Set initial state and color
    if (isAlpha) {
        creature->state = STATE_ALPHA_TRAVELING;
        // Alphas get the same herd color as their members, but with a black ring
        creature->color = generateHerdColor(creature->uniqueID); // Same color as herd
    } else {
        creature->state = STATE_RESTING;  // Start followers in resting state
        // Herd members get a bright random color (will be overridden when assigned to alpha)
        creature->color = getRandomBrightColor();
        creature->restingTimeLeft = MainWindow::CREATURE_MIN_REST_TICKS +
            QRandomGenerator::global()->bounded(MainWindow::CREATURE_MAX_REST_TICKS - MainWindow::CREATURE_MIN_REST_TICKS);
    }

    // Create graphics with ring indicator
    creature->graphicsItem = new QGraphicsEllipseItem(0, 0, creature->size, creature->size);
    creature->graphicsItem->setPos(x, y);
    creature->graphicsItem->setBrush(QBrush(creature->color));

    // Set ring color and Z-value based on alpha status
    if (isAlpha) {
        creature->graphicsItem->setPen(QPen(Qt::black, CREATURE_RING_WIDTH)); // Black ring for alphas
        creature->graphicsItem->setZValue(20); // Alphas always on top
    } else {
        creature->graphicsItem->setPen(QPen(Qt::white, CREATURE_RING_WIDTH)); // White ring for regular creatures
        creature->graphicsItem->setZValue(10); // Regular creatures below alphas
    }

    mWorldScene->addItem(creature->graphicsItem);

    return creature;
}

void MainWindow::findHerdTarget(SimpleCreature* creature) {
    // This function is no longer used in the simplified system
    // Keeping it for compatibility but it does nothing
}

void MainWindow::assignCreatureToNearestAlpha(SimpleCreature* creature, const QVector<SimpleCreature*>& alphas) {
    if (!creature || creature->isAlpha || alphas.empty()) return; // Don't assign alphas to other alphas!

    // Find nearest alpha
    SimpleCreature* nearestAlpha = nullptr;
    qreal nearestDistance = std::numeric_limits<qreal>::max();

    for (auto* alpha : alphas) {
        if (alpha && alpha->isAlpha) {
            qreal distance = distanceBetween(creature->posX, creature->posY, alpha->posX, alpha->posY);
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestAlpha = alpha;
            }
        }
    }

    if (nearestAlpha) {
        creature->myAlpha = nearestAlpha;
        // Give this creature the same color as its alpha's herd
        creature->color = generateHerdColor(nearestAlpha->uniqueID);
        // Keep the white ring for regular herd members (as per your preference)
        creature->graphicsItem->setPen(QPen(Qt::white, CREATURE_RING_WIDTH));
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
    int sampleSize = qMin(8, mCreatures.size()); // Show more samples to see alphas and herds
    for (int i = 0; i < sampleSize; i++) {
        SimpleCreature* creature = mCreatures[i];
        QString stateStr;
        switch(creature->state) {
            case STATE_SEEKING_HERD: stateStr = "seeking"; break;
            case STATE_MOVING_TO_HERD: stateStr = "moving"; break;
            case STATE_FINDING_SPACE: stateStr = "spacing"; break;
            case STATE_RESTING: stateStr = "resting"; break;
            case STATE_WANDERING: stateStr = "wandering"; break;
            case STATE_ALPHA_TRAVELING: stateStr = "alpha_travel"; break;
            case STATE_ALPHA_RESTING: stateStr = "alpha_rest"; break;
        }
        QString typeStr = creature->isAlpha ? "ALPHA" : "member";
        QString alphaInfo = creature->myAlpha ? QString("alpha%1").arg(creature->myAlpha->uniqueID) : "none";

        appendOutput(QString("  %1 %2: pos(%3,%4) speed=%5 state=%6 follows=%7")
                    .arg(typeStr)
                    .arg(creature->uniqueID)
                    .arg(creature->posX, 0, 'f', 1)
                    .arg(creature->posY, 0, 'f', 1)
                    .arg(creature->speed, 0, 'f', 1)
                    .arg(stateStr)
                    .arg(alphaInfo));
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

QColor MainWindow::getRandomBrightColor() {
    // Generate bright, saturated colors for better visibility
    int colorChoice = QRandomGenerator::global()->bounded(12);
    switch (colorChoice) {
        case 0: return QColor(255, 100, 100);  // Bright red
        case 1: return QColor(100, 255, 100);  // Bright green
        case 2: return QColor(100, 100, 255);  // Bright blue
        case 3: return QColor(255, 255, 100);  // Bright yellow
        case 4: return QColor(255, 100, 255);  // Bright magenta
        case 5: return QColor(100, 255, 255);  // Bright cyan
        case 6: return QColor(255, 165, 0);    // Orange
        case 7: return QColor(255, 20, 147);   // Deep pink
        case 8: return QColor(50, 205, 50);    // Lime green
        case 9: return QColor(138, 43, 226);   // Blue violet
        case 10: return QColor(255, 140, 0);   // Dark orange
        case 11: return QColor(30, 144, 255);  // Dodger blue
        default: return QColor(255, 100, 100); // Default bright red
    }
}

QColor MainWindow::generateHerdColor(int alphaID) {
    // Generate consistent herd colors based on alpha ID
    // Use the alpha ID as a seed for consistent color generation
    QRandomGenerator generator(alphaID);

    // Generate bright, saturated colors for each herd
    int hue = generator.bounded(360);  // 0-359 degrees on color wheel
    int saturation = 200 + generator.bounded(56); // 200-255 (high saturation)
    int value = 200 + generator.bounded(56);      // 200-255 (high brightness)

    return QColor::fromHsv(hue, saturation, value);
}

int MainWindow::getUniqueID() {
    static int nextID = 1;
    return nextID++;
}

// === Housekeeping Methods ===
void MainWindow::runHousekeeping() {
    if (mCreatures.empty()) return;

    appendOutput(QString("=== HOUSEKEEPING: Processing creatures starting at index %1 ===").arg(mHousekeepingCreatureIndex));

    int orphansFound = 0;
    int orphansRehomed = 0;

    // Process a chunk of creatures
    int processed = 0;

    while (processed < MainWindow::HOUSEKEEPING_CREATURES_PER_INTERVAL && mHousekeepingCreatureIndex < mCreatures.size()) {
        SimpleCreature* creature = mCreatures[mHousekeepingCreatureIndex];

        if (creature && creature->exists && !creature->isAlpha) {
            // Check if this creature is orphaned (no alpha assigned)
            if (!creature->myAlpha) {
                orphansFound++;

                // Find a herd that's not full (has fewer than HERD_MAX_SIZE members)
                QVector<SimpleCreature*> availableAlphas;

                for (auto* alpha : mCreatures) {
                    if (alpha && alpha->isAlpha && alpha->exists) {
                        // Count current herd size for this alpha
                        int herdSize = 0;
                        for (auto* member : mCreatures) {
                            if (member && member->exists && member->myAlpha == alpha) {
                                herdSize++;
                            }
                        }

                        if (herdSize < MainWindow::HERD_MAX_SIZE) {
                            availableAlphas.push_back(alpha);
                        }
                    }
                }

                // Assign orphan to a random available alpha
                if (!availableAlphas.empty()) {
                    int randomIndex = QRandomGenerator::global()->bounded(availableAlphas.size());
                    SimpleCreature* newAlpha = availableAlphas[randomIndex];

                    // Assign to herd
                    creature->myAlpha = newAlpha;
                    creature->color = generateHerdColor(newAlpha->uniqueID);
                    creature->graphicsItem->setBrush(QBrush(creature->color));
                    // Keep the white ring for regular herd members (as per your preference)
                    creature->graphicsItem->setPen(QPen(Qt::white, MainWindow::CREATURE_RING_WIDTH));

                    // Reset creature state to resting
                    creature->state = STATE_RESTING;
                    creature->restingTimeLeft = MainWindow::CREATURE_MIN_REST_TICKS +
                        QRandomGenerator::global()->bounded(MainWindow::CREATURE_MAX_REST_TICKS - MainWindow::CREATURE_MIN_REST_TICKS);
                    creature->herdTarget = nullptr;
                    creature->hasHerdTarget = false;

                    orphansRehomed++;
                }
            }
        }

        mHousekeepingCreatureIndex++;
        processed++;
    }

    // Reset index when we've processed all creatures
    if (mHousekeepingCreatureIndex >= mCreatures.size()) {
        mHousekeepingCreatureIndex = 0;
    }

    if (orphansFound > 0) {
        appendOutput(QString("Housekeeping: Found %1 orphans, rehomed %2").arg(orphansFound).arg(orphansRehomed));
    }
}
