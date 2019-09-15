#include <QDebug>
#include <QStringListModel>
#include <QFileDialog>
#include <QProcess>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "bluegenstone.h"
#include "ui_bluegenstone.h"
#include "aboutdialog.h"

BlueGenstone::BlueGenstone(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BlueGenstone)
{
    ui->setupUi(this);
    ui->statusText->setVisible(false);
    this->add_sequence();
    this->on_dummySpaceColor_textChanged("");
}

BlueGenstone::~BlueGenstone()
{
    delete ui;
}

void BlueGenstone::add_sequence() {
    this->sequences.emplace_back();
    this->update_sequence_buttons();

    ui->sequenceComboBox->setCurrentIndex(static_cast<int>(this->sequences.size() - 1));
}

void BlueGenstone::delete_sequence(std::vector<Sequence>::difference_type sequence_index) {
    this->sequences.erase(this->sequences.begin() + sequence_index);
    this->update_sequence_buttons();

    if(static_cast<std::size_t>(sequence_index) == this->sequences.size()) {
        sequence_index--;
    }

    ui->sequenceComboBox->setCurrentIndex(static_cast<int>(sequence_index));
}

// So the user doesn't make a million sequences by accident lol
#define MAX_SEQUENCES 255

// Typing this is long. Let's make it shorter
int BlueGenstone::current_index() {
    return ui->sequenceComboBox->currentIndex();
}

void BlueGenstone::update_sequence_buttons() {
    auto sequence_count = this->sequences.size();

    if(sequence_count == 1) {
        ui->sequenceDeleteButton->setEnabled(false);
    }
    else {
        ui->sequenceDeleteButton->setEnabled(true);
    }

    if(sequence_count == MAX_SEQUENCES) {
        ui->sequenceAddButton->setEnabled(false);
    }
    else {
        ui->sequenceAddButton->setEnabled(true);
    }

    // Disable signals so we don't call update_sequence_interface() 9000 times
    ui->sequenceComboBox->blockSignals(true);
    ui->sequenceComboBox->clear();

    // The sequence names are basically just Sequence #0, Sequence #1, etc.
    for(std::size_t i = 0; i < sequence_count; i++) {
        ui->sequenceComboBox->addItem(QString("Sequence #") + QString::number(i));
    }

    // Set the current index to -1 so it can be set to something else while still calling update_sequence_interface()
    ui->sequenceComboBox->setCurrentIndex(-1);
    ui->sequenceComboBox->blockSignals(false);
}

void BlueGenstone::update_sequence_interface() {
    // Create a new model
    QStringListModel *model = new QStringListModel();
    QStringList new_list;
    for(auto &bitmap : this->sequences[static_cast<std::size_t>(this->current_index())].paths) {
        new_list.push_back(bitmap);
    }
    model->setStringList(new_list);

    // Set the model for our bitmap list
    ui->bitmapList->setModel(model);
}

void BlueGenstone::on_sequenceAddButton_clicked()
{
    this->add_sequence();
}

void BlueGenstone::on_sequenceDeleteButton_clicked()
{
    this->delete_sequence(this->current_index());
}

void BlueGenstone::on_sequenceComboBox_currentIndexChanged(int)
{
    this->update_sequence_interface();
}

void BlueGenstone::on_bitmapAddButton_clicked()
{
    // Prepare a list of allowed extensions
    QStringList allowed_extensions;
    allowed_extensions.push_back("Allowed Images (*.tif *.tiff *.png *.bmp *.tga)");

    auto files = this->open_file(allowed_extensions, true);
    for(QString file : files) {
        this->add_file(file);
    }
}

QStringList BlueGenstone::open_file(const QStringList &valid_files, bool multiple, bool save) {
    QFileDialog qfd;

    if(this->last_directory) {
        qfd.setDirectory(*last_directory);
    }

    if(multiple) {
        qfd.setFileMode(QFileDialog::FileMode::ExistingFiles);
    }

    qfd.setNameFilters(valid_files);
    if(save) {
        qfd.setFileMode(QFileDialog::FileMode::AnyFile);
        qfd.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
        qfd.setDefaultSuffix("tif");
    }
    else {
        qfd.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
    }

    if(qfd.exec()) {
        delete this->last_directory;
        this->last_directory = new QString(qfd.directory().path());
        return qfd.selectedFiles();
    }
    else {
        return QStringList();
    }
}

void BlueGenstone::on_bitmapDeleteButton_clicked()
{
    int selected_thing = ui->bitmapList->currentIndex().row();
    if(selected_thing >= 0) {
        auto &paths = this->sequences[static_cast<std::size_t>(this->current_index())].paths;
        paths.erase(paths.begin() + selected_thing);
        this->update_sequence_interface();
    }
}

void BlueGenstone::on_bitmapMoveUpButton_clicked()
{
    int selected_thing = ui->bitmapList->currentIndex().row();
    if(selected_thing > 0) {
        auto &paths = this->sequences[static_cast<std::size_t>(this->current_index())].paths;
        std::size_t selected_thing_index = static_cast<std::size_t>(selected_thing);

        QString a = paths[selected_thing_index];
        QString b = paths[selected_thing_index - 1];

        paths[selected_thing_index] = b;
        paths[selected_thing_index - 1] = a;

        this->update_sequence_interface();

        ui->bitmapList->setCurrentIndex(ui->bitmapList->model()->index(selected_thing - 1,0));
    }
}

void BlueGenstone::on_bitmapMoveDownButton_clicked()
{
    int selected_thing = ui->bitmapList->currentIndex().row();
    if(selected_thing >= 0) {
        auto &paths = this->sequences[static_cast<std::size_t>(this->current_index())].paths;
        std::size_t selected_thing_index = static_cast<std::size_t>(selected_thing);
        if(selected_thing_index == paths.size() - 1) {
            return;
        }

        QString a = paths[selected_thing_index];
        QString b = paths[selected_thing_index + 1];

        paths[selected_thing_index] = b;
        paths[selected_thing_index + 1] = a;

        this->update_sequence_interface();

        ui->bitmapList->setCurrentIndex(ui->bitmapList->model()->index(selected_thing + 1,0));
    }
}

void BlueGenstone::on_findOutputTIFFButton_clicked()
{
    // Prepare a list of allowed extensions
    QStringList allowed_extensions;
    allowed_extensions.push_back("Tagged Image File Format (*.tif)");

    QStringList opened = this->open_file(allowed_extensions, false, true);
    if(opened.size() == 1) {
        ui->outputTIFFPath->setText(opened[0]);
    }
}

void BlueGenstone::on_generateTIFFButton_clicked()
{
    // Begin by checking if we have a place to save it
    QString output_tiff = ui->outputTIFFPath->text();
    if(output_tiff.isEmpty()) {
        this->set_status("( ')> But where would I save it?");
        ui->outputTIFFPath->setFocus();
        return;
    }

    // Next, we need to see if we have bitmaps
    bool have_bitmaps = false;
    for(auto &s : this->sequences) {
        if(s.paths.size()) {
            have_bitmaps = true;
            break;
        }
    }
    if(!have_bitmaps) {
        this->set_status("( ')> You need to select some bitmaps first!");
        ui->boxOfSequences->setFocus();
        return;
    }

    // Start with the process thing
    QProcess p;
    QStringList arguments;

    if(ui->dummySpaceColor->text().size() != 0) {
        arguments.push_back("-d");
        arguments.push_back(ui->dummySpaceColor->text());
    }

    arguments.push_back(output_tiff);
    for(auto &s : this->sequences) {
        arguments.push_back("-s");
        for(auto &p : s.paths) {
            arguments.push_back(p);
        }
    }

    // Run it
#ifdef _WIN32
#define BG_EXECUTABLE "blue-gen.exe"
#else
#define BG_EXECUTABLE "blue-gen"
#endif
    p.start(BG_EXECUTABLE, arguments);
    p.waitForFinished(-1);
    auto err = p.readAllStandardError();
    auto out = p.readAllStandardOutput();

    if(p.error() == QProcess::FailedToStart) {
        this->set_status("(X)> Failed to open blue-gen.");
        return;
    }
    else if(p.exitCode() == 0) {
        this->set_status(QString::fromUtf8(out).replace("\r", "").replace("\n", ""));
    }
    else {
        this->set_status(QString::fromUtf8(err).replace("\r", "").replace("\n", ""));
    }
}

void BlueGenstone::set_status(const QString &status) {
    ui->statusText->setText(status);
    ui->statusText->setVisible(true);
}

void BlueGenstone::on_dummySpaceColor_textChanged(const QString &text)
{
    if(text.size() == 6) {
        ui->colorViewer->setStyleSheet(QString("* { background-color: #") + text + "}");
    }
    else if(text.size() == 0) {
        ui->colorViewer->setStyleSheet("* { background-color: #00FFFF}");
    }
}

void BlueGenstone::on_aboutButton_clicked()
{
    AboutDialog().exec();
}

void BlueGenstone::add_file(const QString &file) {
    this->sequences[static_cast<std::size_t>(this->current_index())].paths.push_back(file);
    this->update_sequence_interface();
    //ui->bitmapList->setCurrentIndex(ui->bitmapList->model()->index(ui->bitmapList->model()->rowCount() - 1,0));
}

void BlueGenstone::dragEnterEvent(QDragEnterEvent *event) {
    event->setAccepted(event->mimeData()->hasUrls());
}

void BlueGenstone::dropEvent(QDropEvent *event) {
    for(auto &url : event->mimeData()->urls()) {
        this->add_file(url.toLocalFile());
    }
}
