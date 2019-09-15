#ifndef BLUEGENSTONE_H
#define BLUEGENSTONE_H

#include <QMainWindow>

namespace Ui {
class BlueGenstone;
}

class BlueGenstone : public QMainWindow
{
    Q_OBJECT

public:
    explicit BlueGenstone(QWidget *parent = nullptr);
    ~BlueGenstone();

private slots:
    void on_sequenceAddButton_clicked();

    void on_sequenceDeleteButton_clicked();

    void on_sequenceComboBox_currentIndexChanged(int index);

    void on_bitmapAddButton_clicked();

    void on_bitmapDeleteButton_clicked();

    void on_bitmapMoveUpButton_clicked();

    void on_bitmapMoveDownButton_clicked();

    void on_findOutputTIFFButton_clicked();

    void on_generateTIFFButton_clicked();

    void on_dummySpaceColor_textChanged(const QString &arg1);

    void on_aboutButton_clicked();

private:
    struct Sequence {
        std::vector<QString> paths;
    };

    std::vector<Sequence> sequences;

    Ui::BlueGenstone *ui;

    int current_index();
    void add_sequence();
    void delete_sequence(std::vector<Sequence>::difference_type sequence_index);
    void update_sequence_buttons();
    void update_sequence_interface();

    QStringList open_file(const QStringList &valid_files, bool multiple, bool save = false);
    QString *last_directory = nullptr;

    void set_status(const QString &status);

    void add_file(const QString &file);

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
};

#endif // BLUEGENSTONE_H
