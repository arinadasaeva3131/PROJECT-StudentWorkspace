#include "noteswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>
#include <QInputDialog>
#include <QDir>
#include <QDate>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

NotesWidget::NotesWidget(QWidget *parent) : QWidget(parent), m_currentNoteId(-1) {
    QHBoxLayout *rootLayout = new QHBoxLayout(this);

    // ================= ЛЕВАЯ ПАНЕЛЬ =================
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(new QLabel("Поиск и Фильтры", this));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Поиск по тексту или дате...");
    leftLayout->addWidget(m_searchEdit);

    m_filterSubjects = new QComboBox(this);
    leftLayout->addWidget(new QLabel("Предмет:", this));
    leftLayout->addWidget(m_filterSubjects);

    m_filterThemes = new QComboBox(this);
    leftLayout->addWidget(new QLabel("Тема:", this));
    leftLayout->addWidget(m_filterThemes);

    m_notesList = new QListWidget(this);
    leftLayout->addWidget(m_notesList);

    QPushButton *btnCreate = new QPushButton("+ Новая заметка", this);
    leftLayout->addWidget(btnCreate);
    rootLayout->addLayout(leftLayout, 1);

    // ================= ПРАВАЯ ПАНЕЛЬ =================
    QVBoxLayout *rightLayout = new QVBoxLayout();

    QHBoxLayout *editorProps = new QHBoxLayout();
    m_subjectCombo = new QComboBox(this);
    m_themeCombo = new QComboBox(this);
    QPushButton *btnAddTheme = new QPushButton("+ Создать Тему", this);
    QPushButton *btnDeleteTheme = new QPushButton("Удалить Тему", this);

    editorProps->addWidget(new QLabel("Привязка:"));
    editorProps->addWidget(m_subjectCombo);
    editorProps->addWidget(new QLabel("Тема:"));
    editorProps->addWidget(m_themeCombo);
    editorProps->addWidget(btnAddTheme);
    editorProps->addWidget(btnDeleteTheme);

    // Тулбар
    QHBoxLayout *toolbar = new QHBoxLayout();
    QPushButton *btnImg = new QPushButton("🖼 Вставить картинку в текст", this);
    QPushButton *btnImport = new QPushButton("📄 Импорт текста (.txt)", this);
    toolbar->addWidget(btnImg);
    toolbar->addWidget(btnImport);
    toolbar->addStretch();

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("Заголовок заметки...");
    m_titleEdit->setStyleSheet("font-size: 16px; font-weight: bold; padding: 8px;");

    m_contentEdit = new QTextEdit(this);
    m_contentEdit->setPlaceholderText("Начни писать свою заметку здесь...");

    // Блок вложений
    QHBoxLayout *attachLayout = new QHBoxLayout();
    attachLayout->addWidget(new QLabel("Вложения (двойной клик чтобы открыть):", this));
    QPushButton *btnAttach = new QPushButton("📎 Прикрепить файл", this);
    attachLayout->addStretch();
    attachLayout->addWidget(btnAttach);

    m_attachmentsList = new QListWidget(this);
    m_attachmentsList->setMaximumHeight(70);
    m_attachmentsList->setStyleSheet("QListWidget { background-color: #1A1A1A; border: 1px dashed #555; }");

    QHBoxLayout *editorButtons = new QHBoxLayout();
    QPushButton *btnSave = new QPushButton("Сохранить изменения", this);
    btnSave->setStyleSheet("background-color: #2D3748; color: #BAE1FF; font-weight: bold;");
    QPushButton *btnDelete = new QPushButton("Удалить заметку", this);
    btnDelete->setStyleSheet("background-color: #4A2D2D; color: #FFB3BA;");

    editorButtons->addWidget(btnSave);
    editorButtons->addWidget(btnDelete);

    rightLayout->addLayout(editorProps);
    rightLayout->addLayout(toolbar);
    rightLayout->addWidget(m_titleEdit);
    rightLayout->addWidget(m_contentEdit);
    rightLayout->addLayout(attachLayout);
    rightLayout->addWidget(m_attachmentsList);
    rightLayout->addLayout(editorButtons);

    rootLayout->addLayout(rightLayout, 3);

    // ================= ПОДКЛЮЧЕНИЯ =================
    connect(btnCreate, &QPushButton::clicked, this, &NotesWidget::createNote);
    connect(btnSave, &QPushButton::clicked, this, &NotesWidget::saveNote);
    connect(btnDelete, &QPushButton::clicked, this, &NotesWidget::deleteNote);
    connect(btnAddTheme, &QPushButton::clicked, this, &NotesWidget::addTheme);
    connect(btnDeleteTheme, &QPushButton::clicked, this, &NotesWidget::deleteTheme);

    connect(btnImg, &QPushButton::clicked, this, &NotesWidget::insertImage);
    connect(btnImport, &QPushButton::clicked, this, &NotesWidget::importText);
    connect(btnAttach, &QPushButton::clicked, this, &NotesWidget::attachFile);
    connect(m_attachmentsList, &QListWidget::itemDoubleClicked, this, &NotesWidget::openAttachment);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &NotesWidget::searchNotes);
    connect(m_filterSubjects, &QComboBox::currentIndexChanged, this, &NotesWidget::loadThemes);
    connect(m_filterThemes, &QComboBox::currentIndexChanged, this, &NotesWidget::loadNotes);
    connect(m_notesList, &QListWidget::currentRowChanged, this, &NotesWidget::onNoteSelected);

    loadFilters();
}

void NotesWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    loadFilters();
}

void NotesWidget::setSubjectFilter(int subjectId) {
    int idx = m_filterSubjects->findData(subjectId);
    if (idx != -1) m_filterSubjects->setCurrentIndex(idx);
}

void NotesWidget::loadFilters() {
    int currentSub = m_filterSubjects->currentData().toInt();

    m_filterSubjects->blockSignals(true);
    m_subjectCombo->blockSignals(true);

    m_filterSubjects->clear();
    m_subjectCombo->clear();

    m_filterSubjects->addItem("Все предметы", -1);
    m_subjectCombo->addItem("Нет (Общая)", -1);

    QSqlQuery q("SELECT id, name FROM subjects");
    while (q.next()) {
        m_filterSubjects->addItem(q.value(1).toString(), q.value(0).toInt());
        m_subjectCombo->addItem(q.value(1).toString(), q.value(0).toInt());
    }

    m_filterSubjects->blockSignals(false);
    m_subjectCombo->blockSignals(false);

    int idx = m_filterSubjects->findData(currentSub);
    if (idx != -1) m_filterSubjects->setCurrentIndex(idx);

    loadThemes();
}

void NotesWidget::loadThemes() {
    m_filterThemes->blockSignals(true);
    m_themeCombo->blockSignals(true);
    m_filterThemes->clear();
    m_themeCombo->clear();

    m_filterThemes->addItem("Все темы", -1);
    m_themeCombo->addItem("Без темы", -1);

    int subId = m_filterSubjects->currentData().toInt();
    QSqlQuery q;
    if (subId == -1) {
        q.prepare("SELECT id, name FROM note_folders");
    } else {
        q.prepare("SELECT id, name FROM note_folders WHERE subject_id = ? OR subject_id = -1");
        q.addBindValue(subId);
    }

    if (q.exec()) {
        while(q.next()) {
            m_filterThemes->addItem(q.value(1).toString(), q.value(0).toInt());
            m_themeCombo->addItem(q.value(1).toString(), q.value(0).toInt());
        }
    }
    m_filterThemes->blockSignals(false);
    m_themeCombo->blockSignals(false);

    loadNotes();
}

void NotesWidget::loadNotes() {
    m_notesList->clear();
    int subId = m_filterSubjects->currentData().toInt();
    int themeId = m_filterThemes->currentData().toInt();
    QString searchTxt = m_searchEdit->text().trimmed();

    QString queryStr = "SELECT id, title FROM notes WHERE 1=1 ";
    if (subId != -1) queryStr += QString("AND subject_id = %1 ").arg(subId);
    if (themeId != -1) queryStr += QString("AND folder_id = %1 ").arg(themeId);

    if (!searchTxt.isEmpty()) {
        queryStr += QString("AND (title LIKE '%%1%' OR content_richtext LIKE '%%1%' OR created_date LIKE '%%1%') ").arg(searchTxt);
    }
    queryStr += "ORDER BY id DESC";

    QSqlQuery q(queryStr);
    while (q.next()) {
        QListWidgetItem *item = new QListWidgetItem(q.value(1).toString());
        item->setData(Qt::UserRole, q.value(0).toInt());
        m_notesList->addItem(item);
    }
}

void NotesWidget::searchNotes() {
    loadNotes();
}

void NotesWidget::onNoteSelected(int row) {
    m_attachmentsList->clear();

    if (row < 0) {
        m_currentNoteId = -1;
        m_titleEdit->clear();
        m_contentEdit->clear();
        m_subjectCombo->setCurrentIndex(0);
        m_themeCombo->setCurrentIndex(0);
        return;
    }
    m_currentNoteId = m_notesList->item(row)->data(Qt::UserRole).toInt();

    QSqlQuery q;
    q.prepare("SELECT title, content_richtext, subject_id, folder_id FROM notes WHERE id = ?");
    q.addBindValue(m_currentNoteId);
    if (q.exec() && q.next()) {
        m_titleEdit->setText(q.value(0).toString());
        m_contentEdit->setHtml(q.value(1).toString());
        m_subjectCombo->setCurrentIndex(m_subjectCombo->findData(q.value(2).toInt()));
        m_themeCombo->setCurrentIndex(m_themeCombo->findData(q.value(3).toInt()));
    }

    QSqlQuery qAtt;
    qAtt.prepare("SELECT file_name, file_path FROM attachments WHERE note_id = ?");
    qAtt.addBindValue(m_currentNoteId);
    if (qAtt.exec()) {
        while (qAtt.next()) {
            QListWidgetItem *attItem = new QListWidgetItem("📎 " + qAtt.value(0).toString());
            attItem->setData(Qt::UserRole, qAtt.value(1).toString());
            m_attachmentsList->addItem(attItem);
        }
    }
}

void NotesWidget::addTheme() {
    bool ok;
    QString text = QInputDialog::getText(this, "Создать Тему", "Название новой темы:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        QSqlQuery q;
        q.prepare("INSERT INTO note_folders (subject_id, name) VALUES (?, ?)");
        q.addBindValue(m_subjectCombo->currentData().toInt());
        q.addBindValue(text);
        q.exec();
        loadThemes();
    }
}

void NotesWidget::deleteTheme() {
    int themeId = m_themeCombo->currentData().toInt();
    if (themeId == -1) {
        QMessageBox::warning(this, "Ошибка", "Выберите конкретную тему для удаления.");
        return;
    }

    auto reply = QMessageBox::question(this, "Удаление темы",
                                       "Вы уверены, что хотите удалить эту тему?\nЗаметки, которые были в ней, перейдут в категорию 'Без темы'.",
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QSqlQuery q;
        q.prepare("UPDATE notes SET folder_id = -1 WHERE folder_id = ?");
        q.addBindValue(themeId);
        q.exec();

        q.prepare("DELETE FROM note_folders WHERE id = ?");
        q.addBindValue(themeId);
        if (q.exec()) {
            loadThemes();
        }
    }
}

void NotesWidget::importText() {
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите текстовый файл", "", "Текстовые файлы (*.txt);;Все файлы (*.*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString text = file.readAll();
        m_contentEdit->insertPlainText(text);
        file.close();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось прочитать файл.");
    }
}

void NotesWidget::attachFile() {
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите файл для прикрепления", "", "Все файлы (*.*)");
    if (filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    QListWidgetItem *item = new QListWidgetItem("📎 " + fileInfo.fileName());
    item->setData(Qt::UserRole, filePath);
    m_attachmentsList->addItem(item);
}

void NotesWidget::openAttachment(QListWidgetItem *item) {
    QString filePath = item->data(Qt::UserRole).toString();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл. Возможно, он был перемещен или удален.");
    }
}

void NotesWidget::insertImage() {
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите изображение", "", "Изображения (*.png *.jpg *.jpeg);;Все файлы (*.*)");
    if (!filePath.isEmpty()) {
        m_contentEdit->insertHtml(QString("<br><img src='%1' width='300'><br>").arg(filePath));
    }
}

void NotesWidget::createNote() {
    m_attachmentsList->clear();

    QSqlQuery q;
    q.prepare("INSERT INTO notes (title, content_richtext, created_date, modified_date, subject_id, folder_id) VALUES (?, ?, ?, ?, ?, ?)");
    q.addBindValue("Новая заметка");
    q.addBindValue("");
    q.addBindValue(QDate::currentDate().toString(Qt::ISODate));
    q.addBindValue(QDate::currentDate().toString(Qt::ISODate));

    int sId = m_filterSubjects->currentData().toInt();
    q.addBindValue(sId == -1 ? QVariant(QVariant::Int) : sId);
    int tId = m_filterThemes->currentData().toInt();
    q.addBindValue(tId == -1 ? QVariant(QVariant::Int) : tId);

    if (q.exec()) {
        loadNotes();
        m_notesList->setCurrentRow(0);
    }
}

void NotesWidget::saveNote() {
    int sId = m_subjectCombo->currentData().toInt();
    int themeId = m_themeCombo->currentData().toInt();
    QString dateStr = QDate::currentDate().toString(Qt::ISODate);
    QString title = m_titleEdit->text().isEmpty() ? "Без названия" : m_titleEdit->text();

    bool isNewNote = (m_currentNoteId == -1);

    if (isNewNote) {
        QSqlQuery q;
        q.prepare("INSERT INTO notes (title, content_richtext, subject_id, folder_id, created_date, modified_date) VALUES (?, ?, ?, ?, ?, ?)");
        q.addBindValue(title);
        q.addBindValue(m_contentEdit->toHtml());
        q.addBindValue(sId == -1 ? QVariant(QVariant::Int) : sId);
        q.addBindValue(themeId == -1 ? QVariant(QVariant::Int) : themeId);
        q.addBindValue(dateStr);
        q.addBindValue(dateStr);

        if (q.exec()) {
            m_currentNoteId = q.lastInsertId().toInt();
        }
    } else {
        QSqlQuery q;
        q.prepare("UPDATE notes SET title = ?, content_richtext = ?, subject_id = ?, folder_id = ?, modified_date = ? WHERE id = ?");
        q.addBindValue(title);
        q.addBindValue(m_contentEdit->toHtml());
        q.addBindValue(sId == -1 ? QVariant(QVariant::Int) : sId);
        q.addBindValue(themeId == -1 ? QVariant(QVariant::Int) : themeId);
        q.addBindValue(dateStr);
        q.addBindValue(m_currentNoteId);
        q.exec();
    }

    if (m_currentNoteId != -1) {
        QSqlQuery delQ;
        delQ.prepare("DELETE FROM attachments WHERE note_id = ?");
        delQ.addBindValue(m_currentNoteId);
        delQ.exec();

        for (int i = 0; i < m_attachmentsList->count(); ++i) {
            QListWidgetItem *item = m_attachmentsList->item(i);
            QString path = item->data(Qt::UserRole).toString();
            QString name = item->text().remove("📎 ");

            QSqlQuery insQ;
            insQ.prepare("INSERT INTO attachments (note_id, file_name, file_path) VALUES (?, ?, ?)");
            insQ.addBindValue(m_currentNoteId);
            insQ.addBindValue(name);
            insQ.addBindValue(path);
            insQ.exec();
        }
    }

    int activeRow = m_notesList->currentRow();
    loadNotes();
    if (isNewNote) {
        m_notesList->setCurrentRow(0);
    } else {
        m_notesList->setCurrentRow(activeRow);
    }
}

void NotesWidget::deleteNote() {
    if (m_currentNoteId == -1) return;

    auto reply = QMessageBox::question(this, "Удаление заметки",
                                       "Вы уверены, что хотите навсегда удалить эту заметку?",
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QSqlQuery q;
        q.prepare("DELETE FROM notes WHERE id = ?");
        q.addBindValue(m_currentNoteId);

        if (q.exec()) {
            loadNotes();
            m_titleEdit->clear();
            m_contentEdit->clear();
            m_attachmentsList->clear();
            m_currentNoteId = -1;
        }
    }
}