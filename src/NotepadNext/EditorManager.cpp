/*
 * This file is part of Notepad Next.
 * Copyright 2021 Justin Dailey
 *
 * Notepad Next is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Notepad Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Notepad Next.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "EditorManager.h"
#include "ScintillaNext.h"
#include "Scintilla.h"

// Editor decorators
#include "BraceMatch.h"
#include "HighlightedScrollBar.h"
#include "SmartHighlighter.h"
#include "SurroundSelection.h"
#include "LineNumbers.h"

const int MARK_BOOKMARK = 24;
const int MARK_HIDELINESBEGIN = 23;
const int MARK_HIDELINESEND = 22;
const int MARK_HIDELINESUNDERLINE = 21;

EditorManager::EditorManager(QObject *parent) : QObject(parent)
{
    connect(this, &EditorManager::editorCreated, [=](ScintillaNext *editor) {
        connect(editor, &ScintillaNext::destroyed, [=]() {
            emit editorClosed(editor);
        });
    });
}

ScintillaNext *EditorManager::createEmptyEditor(const QString &name)
{
    ScintillaNext *editor = new ScintillaNext(new ScintillaBuffer(name));
    QPointer<ScintillaNext> pointer = QPointer<ScintillaNext>(editor);
    editors.append(pointer);

    setupEditor(editor);

    emit editorCreated(editor);

    return editor;
}

ScintillaNext *EditorManager::createEditorFromFile(const QString &filePath)
{
    ScintillaNext *editor = new ScintillaNext(ScintillaBuffer::fromFile(filePath));
    QPointer<ScintillaNext> pointer = QPointer<ScintillaNext>(editor);
    editors.append(pointer);

    setupEditor(editor);

    emit editorCreated(editor);

    return editor;
}

ScintillaNext *EditorManager::getEditorByFilePath(const QString &filePath)
{
    QFileInfo newInfo(filePath);
    newInfo.makeAbsolute();

    purgeOldEditorPointers();

    foreach (ScintillaNext *editor, editors) {
        if (editor->isFile() && editor->fileInfo() == newInfo) {
            return editor;
        }
    }

    return Q_NULLPTR;
}

void EditorManager::setupEditor(ScintillaNext *editor)
{
    qInfo(Q_FUNC_INFO);

    setFoldMarkers(editor, "box");
    editor->setIdleStyling(SC_IDLESTYLING_TOVISIBLE);
    editor->setEndAtLastLine(false);

    editor->setCodePage(SC_CP_UTF8);

    editor->setMultipleSelection(true);
    editor->setAdditionalSelectionTyping(true);
    editor->setMultiPaste(SC_MULTIPASTE_EACH);
    editor->setVirtualSpaceOptions(SCVS_RECTANGULARSELECTION);

    editor->setMarginLeft(2);
    editor->setMarginWidthN(0, 30);
    editor->setMarginMaskN(1, (1<<MARK_BOOKMARK) | (1<<MARK_HIDELINESBEGIN) | (1<<MARK_HIDELINESEND) | (1<<MARK_HIDELINESUNDERLINE));
    editor->setMarginMaskN(2, SC_MASK_FOLDERS);
    editor->setMarginWidthN(2, 14);

    editor->markerSetAlpha(MARK_BOOKMARK, 70);
    editor->markerDefine(MARK_HIDELINESUNDERLINE, SC_MARK_UNDERLINE);
    editor->markerSetBack(MARK_HIDELINESUNDERLINE, 0x77CC77);

    editor->markerDefine(MARK_BOOKMARK, SC_MARK_BOOKMARK);
    editor->markerDefine(MARK_HIDELINESBEGIN, SC_MARK_ARROW);
    editor->markerDefine(MARK_HIDELINESEND, SC_MARK_ARROWDOWN);

    editor->setMarginSensitiveN(1, true);
    editor->setMarginSensitiveN(2, true);

    editor->setFoldFlags(SC_FOLDFLAG_LINEAFTER_CONTRACTED);
    editor->setScrollWidthTracking(true);
    editor->setScrollWidth(1);

    //editor->clearAllCmdKeys();
    editor->setTabDrawMode(SCTD_STRIKEOUT);

    editor->assignCmdKey(SCK_RETURN, SCI_NEWLINE);

    editor->setCaretLineBack(0xFFE8E8);
    editor->setCaretLineVisible(true);
    editor->setCaretLineVisibleAlways(true);
    editor->setCaretFore(0xFF0080);
    editor->setCaretWidth(2);
    editor->setSelBack(true, 0xC0C0C0);

    editor->setEdgeColour(0x80FFFF);

    editor->setWhitespaceFore(true, 0x6AB5FF);
    editor->setWhitespaceSize(2);

    editor->setFoldMarginColour(true, 0xFFFFFF);
    editor->setFoldMarginHiColour(true, 0xE9E9E9);

    editor->setIndentationGuides(SC_IV_LOOKBOTH);

    editor->setAutomaticFold(SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CLICK | SC_AUTOMATICFOLD_CHANGE);
    editor->markerEnableHighlight(true);

    // Indicators
    // Find Mark Style
    // editor->indicSetFore(31, 0x0000FF);
    // Smart HighLighting
    // editor->indicSetFore(29, 0x00FF00);
    // Incremental highlight all
    // editor->indicSetFore(28, 0xFF8000);
    // Tags match highlighting
    // editor->indicSetFore(27, 0xFF0080);
    // Tags attribute
    // editor->indicSetFore(26, 0x00FFFF);

    /*
    -- Mark Style 1
    editor.IndicFore[25] = rgb(0x00FFFF)
    -- Mark Style 2
    editor.IndicFore[24] = rgb(0xFF8000)
    -- Mark Style 3
    editor.IndicFore[23] = rgb(0xFFFF00)
    -- Mark Style 4
    editor.IndicFore[22] = rgb(0x8000FF)
    -- Mark Style 5
    editor.IndicFore[21] = rgb(0x008000)

    SetFolderMarkers("box")
    */

    // -- reset everything
    editor->clearDocumentStyle();
    editor->styleResetDefault();

    editor->styleSetFore(STYLE_DEFAULT, 0x000000);
    editor->styleSetBack(STYLE_DEFAULT, 0xFFFFFF);
    editor->styleSetSize(STYLE_DEFAULT, 10);
    editor->styleSetFont(STYLE_DEFAULT, "Courier New");

    editor->styleClearAll();

    editor->styleSetFore(STYLE_LINENUMBER, 0x808080);
    editor->styleSetBack(STYLE_LINENUMBER, 0xE4E4E4);
    editor->styleSetBold(STYLE_LINENUMBER, false);

    editor->styleSetFore(STYLE_BRACELIGHT, 0x0000FF);
    editor->styleSetBack(STYLE_BRACELIGHT, 0xFFFFFF);

    editor->styleSetFore(STYLE_BRACEBAD, 0x000080);
    editor->styleSetBack(STYLE_BRACEBAD, 0xFFFFFF);

    editor->styleSetFore(STYLE_INDENTGUIDE, 0xC0C0C0);
    editor->styleSetBack(STYLE_INDENTGUIDE, 0xFFFFFF);

    // STYLE_CONTROLCHAR
    // STYLE_CALLTIP
    // STYLE_FOLDDISPLAYTEXT

    // Decorators
    SmartHighlighter *s = new SmartHighlighter(editor);
    s->setEnabled(true);

    HighlightedScrollBarDecorator *h = new HighlightedScrollBarDecorator(editor);
    h->setEnabled(true);

    BraceMatch *b = new BraceMatch(editor);
    b->setEnabled(true);

    LineNumbers *l = new LineNumbers(editor);
    l->setEnabled(true);

    SurroundSelection *ss = new SurroundSelection(editor);
    ss->setEnabled(true);
}

// TODO: Move this into the editor eventually?
void EditorManager::setFoldMarkers(ScintillaNext *editor, const QString &type)
{
    QMap<QString, QList<int>> map{
        {"simple", {SC_MARK_MINUS, SC_MARK_PLUS, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY}},
        {"arrow",  {SC_MARK_ARROWDOWN, SC_MARK_ARROW, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY, SC_MARK_EMPTY}},
        {"circle", {SC_MARK_CIRCLEMINUS, SC_MARK_CIRCLEPLUS, SC_MARK_VLINE, SC_MARK_LCORNERCURVE, SC_MARK_CIRCLEPLUSCONNECTED, SC_MARK_CIRCLEMINUSCONNECTED, SC_MARK_TCORNERCURVE }},
        {"box",    {SC_MARK_BOXMINUS, SC_MARK_BOXPLUS, SC_MARK_VLINE, SC_MARK_LCORNER, SC_MARK_BOXPLUSCONNECTED, SC_MARK_BOXMINUSCONNECTED, SC_MARK_TCORNER }},
    };

    if (!map.contains(type))
        return;

    const auto types = map[type];
    editor->markerDefine(SC_MARKNUM_FOLDEROPEN, types[0]);
    editor->markerDefine(SC_MARKNUM_FOLDER, types[1]);
    editor->markerDefine(SC_MARKNUM_FOLDERSUB, types[2]);
    editor->markerDefine(SC_MARKNUM_FOLDERTAIL, types[3]);
    editor->markerDefine(SC_MARKNUM_FOLDEREND, types[4]);
    editor->markerDefine(SC_MARKNUM_FOLDEROPENMID, types[5]);
    editor->markerDefine(SC_MARKNUM_FOLDERMIDTAIL, types[6]);

    for (int i = SC_MARKNUM_FOLDEREND; i <= SC_MARKNUM_FOLDEROPEN; ++i) {
        editor->markerSetFore(i, 0xF3F3F3);
        editor->markerSetBack(i, 0x808080);
        editor->markerSetBackSelected(i, 0x0000FF);
    }
}

void EditorManager::purgeOldEditorPointers()
{
    QMutableListIterator<QPointer<ScintillaNext>> it(editors);

    while (it.hasNext()) {
        QPointer<ScintillaNext> pointer = it.next();
        if (pointer.isNull())
            it.remove();
    }
}