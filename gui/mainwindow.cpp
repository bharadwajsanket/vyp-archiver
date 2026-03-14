#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

// ─────────────────────────────────────────────────────────────────────────────
//  Theme stylesheets
// ─────────────────────────────────────────────────────────────────────────────
static const char *DARK_SS = R"(
QMainWindow, QDialog, QWidget {
    background: #0f1117;
    color: #e8eaf0;
    font-family: "SF Mono", "Fira Code", "Consolas", "Courier New", monospace;
    font-size: 12px;
}
QMenuBar {
    background: #0f1117;
    color: #e8eaf0;
    border-bottom: 1px solid #252d3a;
}
QMenuBar::item:selected { background: #1e2738; }
QMenu {
    background: #161b24;
    color: #e8eaf0;
    border: 1px solid #252d3a;
}
QMenu::item:selected { background: #1e2738; }
#toolStrip {
    background: #161b24;
    border-bottom: 1px solid #252d3a;
}
QPushButton#toolBtn {
    background: #1e2738;
    color: #e8eaf0;
    border: 1px solid #252d3a;
    border-radius: 5px;
    padding: 0 14px;
    min-height: 34px;
    font-size: 12px;
}
QPushButton#toolBtn:hover  { background: #263044; border-color: #f5a623; color: #f5a623; }
QPushButton#toolBtn:pressed { background: #0f1117; border-color: #7a510f; }
QPushButton#toolBtn:disabled { color: #3d4757; border-color: #1e2738; background: #141820; }
QPushButton#opBtn {
    background: transparent;
    color: #8a95a8;
    border: 1px solid #252d3a;
    border-radius: 5px;
    padding: 0 10px;
    min-height: 32px;
    min-width: 148px;
    font-size: 11px;
    text-align: left;
}
QPushButton#opBtn:hover  { background: #1e2738; color: #e8eaf0; border-color: #3d4757; }
QPushButton#opBtn:pressed { background: #0f1117; color: #f5a623; border-color: #7a510f; }
QPushButton#opBtn:disabled { color: #272f3d; border-color: #181f2b; }
QListWidget#fileList {
    background: #161b24;
    border: 1px solid #252d3a;
    border-radius: 5px;
    outline: none;
    padding: 2px 0;
}
QListWidget#fileList::item { padding: 8px 14px; border-bottom: 1px solid #1a2030; color: #c8cdd8; }
QListWidget#fileList::item:last-child { border-bottom: none; }
QListWidget#fileList::item:hover { background: #1c2436; color: #e8eaf0; }
QListWidget#fileList::item:selected { background: #1b2e47; color: #f5a623; border-left: 3px solid #f5a623; padding-left: 11px; }
QListWidget#fileList[dragActive="true"] { border: 2px solid #f5a623; background: #1a2030; }
QLabel#archiveLbl { color: #3d4757; font-size: 11px; }
QLabel#countLbl   { color: #3d4757; font-size: 11px; }
QLabel#sectionLbl { color: #3d4757; font-size: 10px; letter-spacing: 1.2px; }
QFrame#statusBar  { background: #0d1016; border-top: 1px solid #1a2030; min-height: 26px; max-height: 26px; }
QLabel#statusLbl  { color: #8a95a8; font-size: 11px; padding: 0 6px; }
QScrollBar:vertical { background: #0f1117; width: 7px; border-radius: 3px; }
QScrollBar::handle:vertical { background: #252d3a; min-height: 28px; border-radius: 3px; }
QScrollBar::handle:vertical:hover { background: #3d4757; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical,  QScrollBar::sub-page:vertical  { background: none; }
QMessageBox { background: #161b24; }
QMessageBox QPushButton { background: #1e2738; color: #e8eaf0; border: 1px solid #252d3a; border-radius: 4px; padding: 4px 16px; min-height: 26px; }
QMessageBox QPushButton:hover { border-color: #f5a623; color: #f5a623; }
)";

static const char *LIGHT_SS = R"(
QMainWindow, QDialog, QWidget {
    background: #f5f5f7;
    color: #1d1d1f;
    font-family: "SF Mono", "Fira Code", "Consolas", "Courier New", monospace;
    font-size: 12px;
}
QMenuBar {
    background: #f5f5f7;
    color: #1d1d1f;
    border-bottom: 1px solid #d1d1d6;
}
QMenuBar::item:selected { background: #e5e5ea; }
QMenu {
    background: #ffffff;
    color: #1d1d1f;
    border: 1px solid #d1d1d6;
}
QMenu::item:selected { background: #e5e5ea; }
#toolStrip {
    background: #ffffff;
    border-bottom: 1px solid #d1d1d6;
}
QPushButton#toolBtn {
    background: #e5e5ea;
    color: #1d1d1f;
    border: 1px solid #c7c7cc;
    border-radius: 5px;
    padding: 0 14px;
    min-height: 34px;
    font-size: 12px;
}
QPushButton#toolBtn:hover  { background: #d1d1d6; border-color: #0071e3; color: #0071e3; }
QPushButton#toolBtn:pressed { background: #c7c7cc; }
QPushButton#toolBtn:disabled { color: #aeaeb2; border-color: #e5e5ea; background: #f2f2f7; }
QPushButton#opBtn {
    background: transparent;
    color: #48484a;
    border: 1px solid #c7c7cc;
    border-radius: 5px;
    padding: 0 10px;
    min-height: 32px;
    min-width: 148px;
    font-size: 11px;
    text-align: left;
}
QPushButton#opBtn:hover  { background: #e5e5ea; color: #1d1d1f; border-color: #aeaeb2; }
QPushButton#opBtn:pressed { background: #d1d1d6; }
QPushButton#opBtn:disabled { color: #c7c7cc; border-color: #e5e5ea; }
QListWidget#fileList {
    background: #ffffff;
    border: 1px solid #d1d1d6;
    border-radius: 5px;
    outline: none;
    padding: 2px 0;
}
QListWidget#fileList::item { padding: 8px 14px; border-bottom: 1px solid #f2f2f7; color: #1d1d1f; }
QListWidget#fileList::item:last-child { border-bottom: none; }
QListWidget#fileList::item:hover { background: #f2f2f7; }
QListWidget#fileList::item:selected { background: #e1f0ff; color: #0071e3; border-left: 3px solid #0071e3; padding-left: 11px; }
QListWidget#fileList[dragActive="true"] { border: 2px solid #0071e3; background: #f0f8ff; }
QLabel#archiveLbl { color: #aeaeb2; font-size: 11px; }
QLabel#countLbl   { color: #aeaeb2; font-size: 11px; }
QLabel#sectionLbl { color: #aeaeb2; font-size: 10px; letter-spacing: 1.2px; }
QFrame#statusBar  { background: #ffffff; border-top: 1px solid #d1d1d6; min-height: 26px; max-height: 26px; }
QLabel#statusLbl  { color: #48484a; font-size: 11px; padding: 0 6px; }
QScrollBar:vertical { background: #f5f5f7; width: 7px; border-radius: 3px; }
QScrollBar::handle:vertical { background: #c7c7cc; min-height: 28px; border-radius: 3px; }
QScrollBar::handle:vertical:hover { background: #aeaeb2; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical,  QScrollBar::sub-page:vertical  { background: none; }
QMessageBox { background: #ffffff; }
QMessageBox QPushButton { background: #e5e5ea; color: #1d1d1f; border: 1px solid #c7c7cc; border-radius: 4px; padding: 4px 16px; min-height: 26px; }
QMessageBox QPushButton:hover { border-color: #0071e3; color: #0071e3; }
)";

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(bool darkMode, QWidget *parent)
    : QMainWindow(parent)
    , m_darkMode(darkMode)
{
    setWindowTitle("VYP Archiver");
    setMinimumSize(760, 500);
    resize(900, 620);
    setAcceptDrops(true);

    buildUi();
    buildMenuBar();
    applyTheme(m_darkMode);
}

// ─────────────────────────────────────────────────────────────────────────────
//  enginePath — ALWAYS resolved from applicationDirPath(), never hardcoded
// ─────────────────────────────────────────────────────────────────────────────
QString MainWindow::enginePath() const
{
    return QCoreApplication::applicationDirPath() + "/vypack";
}

// ─────────────────────────────────────────────────────────────────────────────
//  runEngine — blocking helper; all engine calls go through here
// ─────────────────────────────────────────────────────────────────────────────
QString MainWindow::runEngine(const QStringList &args, bool *ok) const
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(enginePath(), args);

    if (!proc.waitForStarted(3000)) {
        if (ok) *ok = false;
        return QString("Engine not found: %1").arg(enginePath());
    }

    proc.waitForFinished(15000);
    const QString output = QString::fromUtf8(proc.readAll()).trimmed();
    const bool success   = (proc.exitStatus() == QProcess::NormalExit &&
                             proc.exitCode()   == 0);
    if (ok) *ok = success;
    return output;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Button factories
// ─────────────────────────────────────────────────────────────────────────────
QPushButton *MainWindow::makeToolBtn(const QString &label, const QString &tip)
{
    auto *b = new QPushButton(label, this);
    b->setObjectName("toolBtn");
    b->setToolTip(tip);
    b->setFixedHeight(34);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QPushButton *MainWindow::makeOpBtn(const QString &label, const QString &tip)
{
    auto *b = new QPushButton(label, this);
    b->setObjectName("opBtn");
    b->setToolTip(tip);
    b->setFixedHeight(32);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

// ─────────────────────────────────────────────────────────────────────────────
//  buildUi — layout unchanged from previous version
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ──────────────────────────────────────────────────────────────
    auto *toolStrip = new QFrame();
    toolStrip->setObjectName("toolStrip");
    toolStrip->setFixedHeight(54);
    auto *tl = new QHBoxLayout(toolStrip);
    tl->setContentsMargins(12, 9, 12, 9);
    tl->setSpacing(8);

    m_btnCreate    = makeToolBtn("＋  New Archive", "Create a new .vyp archive");
    m_btnOpen      = makeToolBtn("▶  Open Archive", "Open an existing .vyp archive");

    auto *vSep = new QFrame();
    vSep->setFrameShape(QFrame::VLine);
    vSep->setFixedWidth(1);

    m_btnAddFile   = makeToolBtn("⊕  Add File",   "Add files to the open archive");
    m_btnAddFolder = makeToolBtn("⊕  Add Folder", "Recursively add all files in a folder");

    tl->addWidget(m_btnCreate);
    tl->addWidget(m_btnOpen);
    tl->addWidget(vSep);
    tl->addWidget(m_btnAddFile);
    tl->addWidget(m_btnAddFolder);
    tl->addStretch();

    m_archiveLbl = new QLabel("No archive open");
    m_archiveLbl->setObjectName("archiveLbl");
    m_archiveLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    tl->addWidget(m_archiveLbl);

    root->addWidget(toolStrip);

    // ── Body ─────────────────────────────────────────────────────────────────
    auto *body = new QHBoxLayout();
    body->setContentsMargins(12, 12, 12, 8);
    body->setSpacing(12);

    m_fileList = new QListWidget();
    m_fileList->setObjectName("fileList");
    m_fileList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileList->setAlternatingRowColors(false);
    body->addWidget(m_fileList, 1);

    // Sidebar
    auto *sidebar = new QVBoxLayout();
    sidebar->setSpacing(6);
    sidebar->setContentsMargins(0, 0, 0, 0);

    auto *sectionLbl = new QLabel("OPERATIONS");
    sectionLbl->setObjectName("sectionLbl");
    sidebar->addWidget(sectionLbl);
    sidebar->addSpacing(2);

    m_btnExtSel = makeOpBtn("  Extract Selected", "Extract the selected file");
    m_btnExtAll = makeOpBtn("  Extract All",      "Extract all files");
    m_btnDelete = makeOpBtn("  Delete File",      "Remove selected file from archive");

    auto *hSep = new QFrame();
    hSep->setFrameShape(QFrame::HLine);

    m_btnTest = makeOpBtn("  Test Archive", "Verify archive integrity");

    sidebar->addWidget(m_btnExtSel);
    sidebar->addWidget(m_btnExtAll);
    sidebar->addSpacing(2);
    sidebar->addWidget(hSep);
    sidebar->addSpacing(2);
    sidebar->addWidget(m_btnDelete);
    sidebar->addSpacing(8);
    sidebar->addWidget(m_btnTest);
    sidebar->addStretch();

    auto *countLbl = new QLabel("—");
    countLbl->setObjectName("countLbl");
    countLbl->setAlignment(Qt::AlignCenter);
    sidebar->addWidget(countLbl);

    body->addLayout(sidebar);
    root->addLayout(body, 1);

    // ── Status bar ────────────────────────────────────────────────────────────
    m_statusBar = new QFrame();
    m_statusBar->setObjectName("statusBar");
    auto *sl = new QHBoxLayout(m_statusBar);
    sl->setContentsMargins(4, 0, 8, 0);
    sl->setSpacing(0);

    auto *dot = new QLabel("●");
    dot->setStyleSheet("font-size: 8px; padding: 0 6px 0 4px;");
    m_statusLbl = new QLabel("Ready");
    m_statusLbl->setObjectName("statusLbl");
    sl->addWidget(dot);
    sl->addWidget(m_statusLbl, 1);
    root->addWidget(m_statusBar);

    // ── Initial button states ──────────────────────────────────────────────
    for (auto *b : {m_btnAddFile, m_btnAddFolder,
                    m_btnExtSel,  m_btnExtAll,
                    m_btnDelete,  m_btnTest}) {
        b->setEnabled(false);
    }

    // ── Connections ──────────────────────────────────────────────────────────
    connect(m_btnCreate,    &QPushButton::clicked, this, &MainWindow::onCreateArchive);
    connect(m_btnOpen,      &QPushButton::clicked, this, &MainWindow::onOpenArchive);
    connect(m_btnAddFile,   &QPushButton::clicked, this, &MainWindow::onAddFile);
    connect(m_btnAddFolder, &QPushButton::clicked, this, &MainWindow::onAddFolder);
    connect(m_btnExtSel,    &QPushButton::clicked, this, &MainWindow::onExtractSelected);
    connect(m_btnExtAll,    &QPushButton::clicked, this, &MainWindow::onExtractAll);
    connect(m_btnDelete,    &QPushButton::clicked, this, &MainWindow::onDeleteFile);
    connect(m_btnTest,      &QPushButton::clicked, this, &MainWindow::onTestArchive);

    connect(m_fileList, &QListWidget::itemSelectionChanged, this, [this] {
        const bool hasSel = !m_fileList->selectedItems().isEmpty();
        m_btnExtSel->setEnabled(hasSel);
        m_btnDelete->setEnabled(hasSel);
    });
    connect(m_fileList, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onItemDoubleClicked);
}

// ─────────────────────────────────────────────────────────────────────────────
//  buildMenuBar — View → Toggle Theme
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::buildMenuBar()
{
    QMenuBar *mb    = menuBar();
    QMenu    *view  = mb->addMenu("View");
    QAction  *themeAction = view->addAction("Toggle Theme");
    themeAction->setShortcut(QKeySequence("Ctrl+Shift+T"));
    connect(themeAction, &QAction::triggered, this, &MainWindow::onToggleTheme);
}

// ─────────────────────────────────────────────────────────────────────────────
//  applyTheme — switches between dark and light stylesheets
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::applyTheme(bool dark)
{
    m_darkMode = dark;
    setStyleSheet(dark ? DARK_SS : LIGHT_SS);
}

void MainWindow::onToggleTheme()
{
    applyTheme(!m_darkMode);
}

// ─────────────────────────────────────────────────────────────────────────────
//  openArchive — shared helper used by button and .vyp drop
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::openArchive(const QString &path)
{
    m_archivePath = path;
    m_archiveLbl->setText(QFileInfo(path).fileName());
    refreshList();
    setStatus("Opened: " + QFileInfo(path).fileName());
}

// ─────────────────────────────────────────────────────────────────────────────
//  addFiles — shared helper used by buttons and drag-and-drop
//
//  Accepts a list of absolute paths (files or directories).
//  Directories are expanded recursively using QDirIterator.
//  Only file basenames are shown in the status message (not full paths).
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::addFiles(const QStringList &absolutePaths)
{
    if (m_archivePath.isEmpty()) {
        setStatus("Open or create an archive first", true);
        return;
    }
    if (absolutePaths.isEmpty()) return;

    // Expand any directories recursively.
    QStringList filePaths;
    for (const QString &path : absolutePaths) {
        QFileInfo fi(path);
        if (fi.isDir()) {
            QDirIterator it(path,
                            QDir::Files | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);
            while (it.hasNext())
                filePaths << it.next();
        } else if (fi.isFile()) {
            filePaths << path;
        }
        // Non-file, non-dir entries (devices, sockets, …) are silently skipped.
    }

    if (filePaths.isEmpty()) {
        setStatus("No files found to add", true);
        return;
    }

    int         added = 0;
    QStringList skippedNames;

    for (const QString &fp : filePaths) {
        bool ok = false;
        runEngine({"add", fp, m_archivePath}, &ok);
        if (ok)
            ++added;
        else
            skippedNames << QFileInfo(fp).fileName();   // filename only, not full path
    }

    refreshList();

    if (skippedNames.isEmpty()) {
        setStatus(QString("Added %1 file%2")
                  .arg(added).arg(added == 1 ? "" : "s"));
    } else if (added > 0) {
        setStatus(QString("Added %1; skipped: %2")
                  .arg(added).arg(skippedNames.join(", ")), true);
    } else {
        setStatus(QString("Nothing added — skipped: %1")
                  .arg(skippedNames.join(", ")), true);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  refreshList
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::refreshList()
{
    m_fileList->clear();
    if (m_archivePath.isEmpty()) return;

    bool ok = false;
    const QString out = runEngine({"list", m_archivePath}, &ok);
    if (!ok) { setStatus("Failed to read archive", true); return; }

    // Engine output:
    //   Files: N
    //     filename.ext  (N bytes)
    for (const QString &raw : out.split('\n', Qt::SkipEmptyParts)) {
        QString line = raw.trimmed();
        if (line.startsWith("Files:") || line.isEmpty()) continue;

        // Strip trailing " (N bytes)" annotation — display filename only.
        const int paren = line.lastIndexOf('(');
        if (paren > 0) line = line.left(paren).trimmed();

        if (!line.isEmpty())
            m_fileList->addItem(QFileInfo(line).fileName());
    }

    if (auto *lbl = findChild<QLabel *>("countLbl"))
        lbl->setText(QString("Files: %1").arg(m_fileList->count()));

    for (auto *b : {m_btnAddFile, m_btnAddFolder, m_btnExtAll, m_btnTest})
        b->setEnabled(true);
    m_btnExtSel->setEnabled(false);
    m_btnDelete->setEnabled(false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  setStatus
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setStatus(const QString &msg, bool error)
{
    // Pick colour appropriate to current theme.
    QString colour;
    if (error) {
        colour = "#e8453c";   // red works on both themes
    } else {
        colour = m_darkMode ? "#3ddc84" : "#1a8f4f";  // bright vs muted green
    }

    m_statusLbl->setText(msg);
    m_statusLbl->setStyleSheet(
        QString("color: %1; font-size: 11px; padding: 0 6px;").arg(colour));

    QTimer::singleShot(5000, this, [this] {
        m_statusLbl->setText("Ready");
        // Revert to theme-appropriate neutral colour.
        const QString neutral = m_darkMode ? "#8a95a8" : "#48484a";
        m_statusLbl->setStyleSheet(
            QString("color: %1; font-size: 11px; padding: 0 6px;").arg(neutral));
    });
}

// ─────────────────────────────────────────────────────────────────────────────
//  Drag-and-drop
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasUrls()) { event->ignore(); return; }

    const bool hasLocal = std::any_of(
        event->mimeData()->urls().cbegin(),
        event->mimeData()->urls().cend(),
        [](const QUrl &u) { return u.isLocalFile(); });

    if (hasLocal) {
        if (m_fileList) m_fileList->setProperty("dragActive", true);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    // Clear drop-target highlight.
    if (m_fileList) {
        m_fileList->setProperty("dragActive", false);
        m_fileList->style()->unpolish(m_fileList);
        m_fileList->style()->polish(m_fileList);
    }

    if (!event->mimeData()->hasUrls()) { event->ignore(); return; }

    QStringList paths;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            const QString p = url.toLocalFile();
            if (!p.isEmpty()) paths << p;
        }
    }

    if (paths.isEmpty()) { event->ignore(); return; }

    event->acceptProposedAction();

    // If a single .vyp file is dropped, open it as an archive.
    if (paths.size() == 1) {
        const QFileInfo fi(paths.first());
        if (fi.isFile() && fi.suffix().toLower() == "vyp") {
            openArchive(paths.first());
            return;
        }
    }

    // Everything else: add to the open archive (directories are expanded).
    addFiles(paths);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slots
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onCreateArchive()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Create New Archive", QDir::homePath(),
        "VYP Archives (*.vyp)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".vyp", Qt::CaseInsensitive)) path += ".vyp";

    bool ok = false;
    const QString out = runEngine({"init", path}, &ok);

    if (ok) {
        m_archivePath = path;
        m_archiveLbl->setText(QFileInfo(path).fileName());
        refreshList();
        setStatus("Archive created: " + QFileInfo(path).fileName());
    } else {
        setStatus(out.isEmpty() ? "Failed to create archive" : out, true);
    }
}

void MainWindow::onOpenArchive()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Archive", QDir::homePath(),
        "VYP Archives (*.vyp);;All Files (*)");
    if (path.isEmpty()) return;
    openArchive(path);
}

void MainWindow::onAddFile()
{
    if (m_archivePath.isEmpty()) return;
    const QStringList files = QFileDialog::getOpenFileNames(
        this, "Add Files", QDir::homePath());
    if (!files.isEmpty()) addFiles(files);
}

void MainWindow::onAddFolder()
{
    if (m_archivePath.isEmpty()) return;
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Select Folder", QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) addFiles({dir});   // addFiles handles recursive expansion
}

void MainWindow::onExtractSelected()
{
    if (m_archivePath.isEmpty()) return;

    QListWidgetItem *item = m_fileList->currentItem();
    if (!item) { setStatus("Select a file first", true); return; }

    const QString destDir = QFileDialog::getExistingDirectory(
        this, "Extract To", QDir::homePath(), QFileDialog::ShowDirsOnly);
    if (destDir.isEmpty()) return;

    // QProcess used directly so we can set workingDirectory — the engine
    // extracts files relative to cwd.
    const QString filename = item->text();
    QProcess proc;
    proc.setWorkingDirectory(destDir);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(enginePath(), {"extract", filename, m_archivePath});
    proc.waitForStarted(3000);
    proc.waitForFinished(15000);

    const bool ok = (proc.exitStatus() == QProcess::NormalExit &&
                     proc.exitCode()   == 0);
    if (ok)
        setStatus("Extracted: " + filename);
    else
        setStatus("Extraction failed: " + QString::fromUtf8(proc.readAll()).trimmed(), true);
}

void MainWindow::onExtractAll()
{
    if (m_archivePath.isEmpty()) return;

    const QString destDir = QFileDialog::getExistingDirectory(
        this, "Extract All To", QDir::homePath(), QFileDialog::ShowDirsOnly);
    if (destDir.isEmpty()) return;

    QProcess proc;
    proc.setWorkingDirectory(destDir);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(enginePath(), {"extractall", m_archivePath});
    proc.waitForStarted(3000);
    proc.waitForFinished(30000);

    const bool ok = (proc.exitStatus() == QProcess::NormalExit &&
                     proc.exitCode()   == 0);
    if (ok)
        setStatus("Extraction complete");
    else
        setStatus("Extract all failed: " + QString::fromUtf8(proc.readAll()).trimmed(), true);
}

void MainWindow::onDeleteFile()
{
    if (m_archivePath.isEmpty()) return;

    QListWidgetItem *item = m_fileList->currentItem();
    if (!item) { setStatus("Select a file first", true); return; }

    const QString filename = item->text();

    if (QMessageBox::question(this, "Delete File",
            QString("Remove <b>%1</b> from the archive?").arg(filename),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) != QMessageBox::Yes)
        return;

    bool ok = false;
    const QString out = runEngine({"delete", filename, m_archivePath}, &ok);

    if (ok) {
        refreshList();
        setStatus("Deleted: " + filename);
    } else {
        setStatus(out.isEmpty() ? "Delete failed" : out, true);
    }
}

void MainWindow::onTestArchive()
{
    if (m_archivePath.isEmpty()) return;

    bool ok = false;
    const QString out = runEngine({"test", m_archivePath}, &ok);

    if (ok && out.contains("Archive OK", Qt::CaseInsensitive))
        setStatus("Archive OK — integrity verified");
    else
        setStatus("Archive corrupted: " + out, true);
}

void MainWindow::onItemDoubleClicked()
{
    m_btnExtSel->setEnabled(true);
    onExtractSelected();
}