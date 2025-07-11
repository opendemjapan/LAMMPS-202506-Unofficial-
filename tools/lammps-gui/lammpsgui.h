/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifndef LAMMPSGUI_H
#define LAMMPSGUI_H

#include <QMainWindow>

#include <QEvent>
#include <QGridLayout>
#include <QList>
#include <QPair>
#include <QSpacerItem>
#include <QString>
#include <QWizard>
#include <string>
#include <vector>

#include "lammpswrapper.h"

// identifier for LAMMPS restart files
#if !defined(LAMMPS_MAGIC)
#define LAMMPS_MAGIC "LammpS RestartT"
#endif

// forward declarations

class QFont;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QTimer;
class QWidget;
class QWizardPage;

QT_BEGIN_NAMESPACE
namespace Ui {
class LammpsGui;
}
QT_END_NAMESPACE

class ChartWindow;
class GeneralTab;
class Highlighter;
class ImageViewer;
class LammpsRunner;
class LogWindow;
class Preferences;
class SlideShow;
class StdCapture;

class LammpsGui : public QMainWindow {
    Q_OBJECT

    friend class CodeEditor;
    friend class GeneralTab;
    friend class TutorialWizard;

public:
    LammpsGui(QWidget *parent = nullptr, const QString &filename = QString());
    ~LammpsGui() override;

    LammpsGui()                             = delete;
    LammpsGui(const LammpsGui &)            = delete;
    LammpsGui(LammpsGui &&)                 = delete;
    LammpsGui &operator=(const LammpsGui &) = delete;
    LammpsGui &operator=(LammpsGui &&)      = delete;

protected:
    void open_file(const QString &filename);
    void view_file(const QString &filename);
    void inspect_file(const QString &filename);
    void write_file(const QString &filename);
    void update_recents(const QString &filename = "");
    void update_variables();
    void do_run(bool use_buffer);
    void start_lammps();
    void run_done();
    void setDocver();
    void autoSave();
    void setFont(const QFont &newfont);
    QWizardPage *tutorial_intro(const int ntutorial, const QString &infotext);
    QWizardPage *tutorial_directory(const int ntutorial);
    void setup_tutorial(int tutno, const QString &dir, bool purgedir, bool getsolution,
                        bool openwebpage);
    void purge_inspect_list();
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void quit();
    void stop_run();

private slots:
    void new_document();
    void open();
    void view();
    void inspect();
    void open_recent();
    void get_directory();
    void start_exe();
    void save();
    void save_as();
    void copy();
    void cut();
    void paste();
    void undo();
    void redo();
    void findandreplace();
    void run_buffer() { do_run(true); }
    void run_file() { do_run(false); }
    void restart_lammps() { lammps.close(); };

    void edit_variables();
    void render_image();
    void view_slides();
    void view_image();
    void view_chart();
    void view_log();
    void view_variables();
    void about();
    void help();
    void manual();
    void tutorial_web();
    void start_tutorial1();
    void start_tutorial2();
    void start_tutorial3();
    void start_tutorial4();
    void start_tutorial5();
    void start_tutorial6();
    void start_tutorial7();
    void start_tutorial8();
    void howto();
    void logupdate();
    void modified();
    void preferences();
    void defaults();

protected:
    Ui::LammpsGui *ui;

private:
    Highlighter *highlighter;
    StdCapture *capturer;
    QLabel *status;
    LogWindow *logwindow;
    ImageViewer *imagewindow;
    ChartWindow *chartwindow;
    SlideShow *slideshow;
    QTimer *logupdater;
    QLabel *dirstatus;
    QProgressBar *progress;
    Preferences *prefdialog;
    QLabel *lammpsstatus;
    QLabel *varwindow;
    QWizard *wizard;

    struct InspectData {
        QWidget *info;
        QWidget *data;
        QWidget *image;
    };
    QList<InspectData *> inspectList;

    QString current_file;
    QString current_dir;
    QList<QString> recent;
    QList<QPair<QString, QString>> variables;

    LammpsWrapper lammps;
    LammpsRunner *runner;
    QString docver;
    QString plugin_path;
    bool is_running;
    int run_counter;
    std::vector<char *> lammps_args;
};

class TutorialWizard : public QWizard {
    Q_OBJECT

public:
    TutorialWizard(int ntutorial, QWidget *parent = nullptr);
    void accept() override;

private:
    int _ntutorial;
};
#endif // LAMMPSGUI_H

// Local Variables:
// c-basic-offset: 4
// End:
