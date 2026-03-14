#pragma once

#include <QMainWindow>
#include <QStringList>

class QPushButton;
class QListWidget;
class QLabel;
class QFrame;
class QDragEnterEvent;
class QDropEvent;

// ─────────────────────────────────────────────────────────────────────────────
//  MainWindow — VYP Archiver
//
//  All engine invocations go through runEngine().
//  enginePath() always resolves relative to applicationDirPath() so the path
//  is never hardcoded and works correctly inside the .app bundle.
//
//  Drag-and-drop behaviour:
//    • .vyp file dropped  → open as archive
//    • folder dropped     → recursively collect all files and add them
//    • other files        → add to currently open archive
// ─────────────────────────────────────────────────────────────────────────────
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(bool darkMode, QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event)           override;

private slots:
    void onCreateArchive();
    void onOpenArchive();
    void onAddFile();
    void onAddFolder();
    void onExtractSelected();
    void onExtractAll();
    void onDeleteFile();
    void onTestArchive();
    void onItemDoubleClicked();
    void onToggleTheme();

private:
    // ── engine ──────────────────────────────────────────────────────────────
    QString enginePath() const;
    QString runEngine(const QStringList &args, bool *ok = nullptr) const;

    // ── archive operations ───────────────────────────────────────────────────
    void openArchive(const QString &path);
    void addFiles(const QStringList &absolutePaths);

    // ── UI helpers ───────────────────────────────────────────────────────────
    void         buildUi();
    void         buildMenuBar();
    void         applyTheme(bool dark);
    void         refreshList();
    void         setStatus(const QString &msg, bool error = false);
    QPushButton *makeToolBtn(const QString &label, const QString &tip);
    QPushButton *makeOpBtn  (const QString &label, const QString &tip);

    // ── state ────────────────────────────────────────────────────────────────
    QString m_archivePath;
    bool    m_darkMode {true};

    // ── widgets ──────────────────────────────────────────────────────────────
    QListWidget *m_fileList    {nullptr};
    QLabel      *m_statusLbl   {nullptr};
    QLabel      *m_archiveLbl  {nullptr};
    QFrame      *m_statusBar   {nullptr};

    QPushButton *m_btnCreate    {nullptr};
    QPushButton *m_btnOpen      {nullptr};
    QPushButton *m_btnAddFile   {nullptr};
    QPushButton *m_btnAddFolder {nullptr};
    QPushButton *m_btnExtSel    {nullptr};
    QPushButton *m_btnExtAll    {nullptr};
    QPushButton *m_btnDelete    {nullptr};
    QPushButton *m_btnTest      {nullptr};
};