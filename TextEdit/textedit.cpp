#include "textedit.h"
#include "ui_textedit.h"
#include <QtDebug>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QTextDocument>
#include <QTextDocumentWriter>
#include <QTextCodec>
#include <QtCore>
#include <QColor>
#include <QColorDialog>
#include <QPixmap>
#include <QCloseEvent>
#include <QFontDatabase>
#include <QComboBox>
#include <QFontComboBox>
#include <QTextEdit>
#include <QTextList>
#include <QTextCharFormat>
#include <QClipboard>
#include <QActionGroup>
#include <QAbstractTextDocumentLayout>
#ifndef QT_NO_PRINTER
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintPreviewDialog>
#endif

TextEdit::TextEdit(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TextEdit)
{
    ui->setupUi(this);
    setWindowTitle(QCoreApplication::applicationName());
    textEdit = new QTextEdit(this);
    connect(textEdit, &QTextEdit::currentCharFormatChanged,
            this, &TextEdit::currentCharFormatChanged);
    connect(textEdit, &QTextEdit::cursorPositionChanged,
            this, &TextEdit::cursorPositionChanged);
    setCentralWidget(textEdit);

    connect(ui->actionAbout, &QAction::triggered,
            this, &TextEdit::about);
    connect(ui->actionAbout_Qt, &QAction::triggered,
            qApp, &QApplication::aboutQt);
    connect(ui->actionQuit, &QAction::triggered,
            this, &QWidget::close);

    connect(ui->actionUndo, &QAction::triggered,
            textEdit, &QTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered,
            textEdit, &QTextEdit::redo);
    connect(ui->actionCopy, &QAction::triggered,
            textEdit, &QTextEdit::copy);
    connect(ui->actionCut, &QAction::triggered,
            textEdit, &QTextEdit::cut);
    connect(ui->actionPaste, &QAction::triggered,
            textEdit, &QTextEdit::paste);

#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &TextEdit::clipboardDataChanged);
#endif

#ifdef  QT_NO_PRINTER
    ui->actionPrint(false);
    ui->actionPrint_Preview(false);
#endif
    colorChanged(textEdit->textColor());
    alignmentChanged(textEdit->alignment());

    comboStyle = new QComboBox(ui->toolBar);
    ui->toolBar->addWidget(comboStyle);
    comboStyle->addItem("Standard");
    comboStyle->addItem("Bullet List (Disc)");
    comboStyle->addItem("Bullet List (Circle)");
    comboStyle->addItem("Bullet List (Square)");
    comboStyle->addItem("Ordered List (Decimal)");
    comboStyle->addItem("Ordered List (Alpha lower)");
    comboStyle->addItem("Ordered List (Alpha upper)");
    comboStyle->addItem("Ordered List (Roman lower)");
    comboStyle->addItem("Ordered List (Roman upper)");

    typedef void (QComboBox::*QComboIntSignal)(int);
    connect(comboStyle, static_cast<QComboIntSignal>(&QComboBox::activated), this, &TextEdit::textStyle);

    typedef void (QComboBox::*QComboStringSignal)(const QString &);
    comboFont = new QFontComboBox(ui->toolBar);
    ui->toolBar->addWidget(comboFont);
    connect(comboFont, static_cast<QComboStringSignal>(&QComboBox::activated), this, &TextEdit::textFamily);

    comboSize = new QComboBox(ui->toolBar);
    comboSize->setObjectName("comboSize");
    ui->toolBar->addWidget(comboSize);
    comboSize->setEditable(true);

    const QList<int> standardSizes = QFontDatabase::standardSizes();
    foreach (int size, standardSizes)
        comboSize->addItem(QString::number(size));
    comboSize->setCurrentIndex(standardSizes.indexOf(QApplication::font().pointSize()));

    connect(comboSize, static_cast<QComboStringSignal>(&QComboBox::activated), this, &TextEdit::textSize);


    connect(textEdit->document(), &QTextDocument::modificationChanged,
            ui->actionSave, &QAction::setEnabled);
    connect(textEdit->document(), &QTextDocument::modificationChanged,
            this, &QWidget::setWindowModified);
    connect(textEdit->document(), &QTextDocument::undoAvailable,
            ui->actionUndo, &QAction::setEnabled);
    connect(textEdit->document(), &QTextDocument::redoAvailable,
            ui->actionRedo, &QAction::setEnabled);

    QActionGroup *alignGroup = new QActionGroup(this);
    if (QApplication::isLeftToRight()) {
        alignGroup->addAction(ui->actionLeft);
        alignGroup->addAction(ui->actionCenter);
        alignGroup->addAction(ui->actionRight);
    } else {
        alignGroup->addAction(ui->actionRight);
        alignGroup->addAction(ui->actionCenter);
        alignGroup->addAction(ui->actionLeft);
    }
    alignGroup->addAction(ui->actionJustify);

    setWindowModified(textEdit->document()->isModified());
    ui->actionSave->setEnabled(textEdit->document()->isModified());
    ui->actionUndo->setEnabled(textEdit->document()->isUndoAvailable());
    ui->actionRedo->setEnabled(textEdit->document()->isRedoAvailable());

    setCurrentFileName(QString());
}

TextEdit::~TextEdit()
{
    delete ui;
}

void TextEdit::closeEvent(QCloseEvent *e)
{
    if (maybeSave())
        e->accept();
    else
        e->ignore();
}

bool TextEdit::maybeSave()
{
    if(!textEdit->document()->isModified())
        return true;

    const QMessageBox::StandardButton ret =
        QMessageBox::warning(this, QCoreApplication::applicationName(),
                             tr("The document has been modified.\n"
                                "Do you want to save your changes?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return on_actionSave_triggered();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;

}

void TextEdit::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    textEdit->document()->setModified(false);

    QString shownName;
    if (fileName.isEmpty())
        shownName = "untitled.txt";
    else
        shownName = QFileInfo(fileName).fileName();

    setWindowTitle(tr("%1[*] - %2").arg(shownName, QCoreApplication::applicationName()));
    setWindowModified(false);
}

bool TextEdit::load(const QString &f)
{
    if (!QFile::exists(f))
        return false;
    QFile file(f);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QTextCodec *codec = Qt::codecForHtml(data);
    QString str = codec->toUnicode(data);
    if (Qt::mightBeRichText(str)) {
        textEdit->setHtml(str);
    } else {
        str = QString::fromLocal8Bit(data);
        textEdit->setPlainText(str);
    }

    setCurrentFileName(f);
    return true;
}

void TextEdit::on_actionNew_triggered()
{
    //New File, name was deault untitled.txt.
    if(maybeSave()){
        textEdit->clear();
        setCurrentFileName(QString());
    }
}

void TextEdit::on_actionOpen_triggered()
{
    //Open File
    QFileDialog fileDialog(this, tr("Open File..."));
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setMimeTypeFilters(QStringList() << "text/html" << "text/plain" << "application/octet-stream");
    if (fileDialog.exec() != QDialog::Accepted)
        return;
    const QString fn = fileDialog.selectedFiles().first();
    if (load(fn))
        statusBar()->showMessage(tr("Opened \"%1\"").arg(QDir::toNativeSeparators(fn)));
    else
        statusBar()->showMessage(tr("Could not open \"%1\"").arg(QDir::toNativeSeparators(fn)));
}

bool TextEdit::on_actionSave_triggered()
{
    if(fileName.isEmpty()){
        return on_actionSave_As_triggered();
    }
    if(fileName.startsWith(QStringLiteral(":/"))){
        return on_actionSave_As_triggered();
    }

    QTextDocumentWriter writer(fileName);
    bool success = writer.write(textEdit->document());
    if(success){
        textEdit->document()->setModified(false);
        statusBar()->showMessage(tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName)));
    } else {
        statusBar()->showMessage(tr("Could not write to file \"%1\"")
                                 .arg(QDir::toNativeSeparators(fileName)));
    }

    return success;
}

bool TextEdit::on_actionSave_As_triggered()
{
    QFileDialog fileDialog(this, tr("Save as..."));
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    QStringList mimeTypes;
    mimeTypes << "application/vnd.oasis.opendocument.text" << "text/html" << "text/plain";
    fileDialog.setMimeTypeFilters(mimeTypes);
    fileDialog.setDefaultSuffix("odt");
    if (fileDialog.exec() != QDialog::Accepted)
        return false;
    const QString fn = fileDialog.selectedFiles().first();
    setCurrentFileName(fn);
    return on_actionSave_triggered();
}

void TextEdit::on_actionExport_PDF_triggered()
{
#ifndef QT_NO_PRINTER
//! [0]
    QFileDialog fileDialog(this, tr("Export PDF"));
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setMimeTypeFilters(QStringList("application/pdf"));
    fileDialog.setDefaultSuffix("pdf");
    if (fileDialog.exec() != QDialog::Accepted)
        return;
    QString fileName = fileDialog.selectedFiles().first();
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    textEdit->document()->print(&printer);
    statusBar()->showMessage(tr("Exported \"%1\"")
                             .arg(QDir::toNativeSeparators(fileName)));
//! [0]
#endif
}

void TextEdit::about()
{
    QMessageBox::about(this, tr("About"), tr("This application demonstrates Qt's "
        "rich text editing facilities in action, providing an example "
        "document for you to experiment with."));
}

void TextEdit::clipboardDataChanged()
{
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        ui->actionPaste->setEnabled(md->hasText());
#endif
}

void TextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    textEdit->mergeCurrentCharFormat(format);
}

void TextEdit::on_actionBold_triggered()
{
    QTextCharFormat format;
    format.setFontWeight(ui->actionBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(format);
}


void TextEdit::on_actionItalic_triggered()
{
    QTextCharFormat format;
    format.setFontItalic(ui->actionItalic->isChecked());
    mergeFormatOnWordOrSelection(format);
}

void TextEdit::on_actionUnderline_triggered()
{
    QTextCharFormat format;
    format.setFontUnderline(ui->actionUnderline->isChecked());
    mergeFormatOnWordOrSelection(format);
}

void TextEdit::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void TextEdit::textStyle(int styleIndex)
{
    QTextCursor cursor = textEdit->textCursor();

    if (styleIndex != 0) {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex) {
            default:
            case 1:
                style = QTextListFormat::ListDisc;
                break;
            case 2:
                style = QTextListFormat::ListCircle;
                break;
            case 3:
                style = QTextListFormat::ListSquare;
                break;
            case 4:
                style = QTextListFormat::ListDecimal;
                break;
            case 5:
                style = QTextListFormat::ListLowerAlpha;
                break;
            case 6:
                style = QTextListFormat::ListUpperAlpha;
                break;
            case 7:
                style = QTextListFormat::ListLowerRoman;
                break;
            case 8:
                style = QTextListFormat::ListUpperRoman;
                break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        } else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    } else {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}

void TextEdit::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui->actionColor->setIcon(pix);
}

void TextEdit::fontChanged(const QFont &f)
{
    comboFont->setCurrentIndex(comboFont->findText(QFontInfo(f).family()));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(f.pointSize())));
    ui->actionBold->setChecked(f.bold());
    ui->actionItalic->setChecked(f.italic());
    ui->actionUnderline->setChecked(f.underline());
}

void TextEdit::on_actionColor_triggered()
{
    QColor col = QColorDialog::getColor(textEdit->textColor(), this);
    if(!col.isValid()){
        qDebug() << "isValid";
        return;
    }
    qDebug()<<"success";
    QTextCharFormat format;
    format.setForeground(col);
    mergeFormatOnWordOrSelection(format);
    colorChanged(col);
}

void TextEdit::on_actionLeft_triggered()
{
    textEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
}

void TextEdit::on_actionCenter_triggered()
{
    textEdit->setAlignment(Qt::AlignCenter);
}

void TextEdit::on_actionRight_triggered()
{
    textEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
}

void TextEdit::on_actionJustify_triggered()
{
    textEdit->setAlignment(Qt::AlignJustify);
}

void TextEdit::on_actionPrint_triggered()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (textEdit->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted)
        textEdit->print(&printer);
    delete dlg;
#endif
}

void TextEdit::on_actionPrint_Preview_triggered()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, &TextEdit::printPreview);
    preview.exec();
#endif
}

void TextEdit::printPreview(QPrinter *printer)
{
#ifdef QT_NO_PRINTER
    Q_UNUSED(printer);
#else
    textEdit->print(printer);
#endif
}

void TextEdit::currentCharFormatChanged(const QTextCharFormat &format)
{
    colorChanged(format.foreground().color());
}

void TextEdit::cursorPositionChanged()
{
    alignmentChanged(textEdit->alignment());
}

void TextEdit::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft)
        ui->actionLeft->setChecked(true);
    else if (a & Qt::AlignHCenter)
        ui->actionCenter->setChecked(true);
    else if (a & Qt::AlignRight)
        ui->actionRight->setChecked(true);
    else if (a & Qt::AlignJustify)
        ui->actionJustify->setChecked(true);
}
