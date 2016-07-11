#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QMainWindow>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;
class QMenu;
class QPrinter;
QT_END_NAMESPACE

namespace Ui {
class TextEdit;
}

class TextEdit : public QMainWindow
{
    Q_OBJECT

public:
    explicit TextEdit(QWidget *parent = 0);
    bool TextEdit::load(const QString &f);
    ~TextEdit();

protected:
    virtual void closeEvent(QCloseEvent *e) Q_DECL_OVERRIDE;

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    bool on_actionSave_triggered();
    bool on_actionSave_As_triggered();
    void on_actionExport_PDF_triggered();
    void on_actionBold_triggered();
    void on_actionItalic_triggered();
    void on_actionUnderline_triggered();
    void on_actionColor_triggered();
    void on_actionLeft_triggered();
    void on_actionCenter_triggered();
    void on_actionRight_triggered();
    void on_actionJustify_triggered();
    void on_actionPrint_triggered();
    void on_actionPrint_Preview_triggered();
    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();

private:
    void setCurrentFileName(const QString &fileName);
    bool maybeSave();
    void about();
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void textStyle(int styleIndex);
    void textFamily(const QString &f);
    void textSize(const QString &p);
    void colorChanged(const QColor &c);
    void fontChanged(const QFont &f);
    void clipboardDataChanged();
    void alignmentChanged(Qt::Alignment a);
    void printPreview(QPrinter *printer);

private:
    Ui::TextEdit *ui;

    QComboBox *comboStyle;
    QFontComboBox *comboFont;
    QComboBox *comboSize;

    QTextEdit *textEdit;
    QString fileName;
};

#endif // TEXTEDIT_H
