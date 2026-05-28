#ifndef NOTESWIDGET_H
#define NOTESWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QShowEvent>

class NotesWidget : public QWidget {
    Q_OBJECT

public:
    explicit NotesWidget(QWidget *parent = nullptr);
    void setSubjectFilter(int subjectId);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void loadFilters();
    void loadThemes();
    void loadNotes();
    void onNoteSelected(int row);
    void createNote();
    void saveNote();
    void deleteNote();
    void addTheme();
    void deleteTheme();
    void searchNotes();

    // Работа с файлами и картинками
    void insertImage();
    void importText();
    void attachFile();
    void openAttachment(QListWidgetItem *item);

private:
    QComboBox *m_filterSubjects;
    QComboBox *m_filterThemes;
    QLineEdit *m_searchEdit;

    QListWidget *m_notesList;
    QLineEdit *m_titleEdit;
    QTextEdit *m_contentEdit;
    QComboBox *m_subjectCombo;
    QComboBox *m_themeCombo;

    QListWidget *m_attachmentsList;

    int m_currentNoteId;
};

#endif // NOTESWIDGET_H