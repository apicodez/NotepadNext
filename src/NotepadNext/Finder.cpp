/*
 * This file is part of Notepad Next.
 * Copyright 2019 Justin Dailey
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


#include "Finder.h"

Finder::Finder(ScintillaNext *edit) :
    editor(edit)
{
    search_flags = editor->searchFlags();
}

void Finder::setEditor(ScintillaNext *editor)
{
    this->editor = editor;
}

void Finder::setSearchFlags(int flags)
{
    this->search_flags = flags;
}

void Finder::setWrap(bool wrap)
{
    this->wrap = wrap;
}

void Finder::setSearchText(const QString &text)
{
    this->text = text;
}

Sci_CharacterRange Finder::findNext(int startPos)
{
    if (text.isEmpty())
        return {INVALID_POSITION, INVALID_POSITION};

    const int pos = startPos == INVALID_POSITION ? editor->selectionEnd() : startPos;
    const QByteArray textData = text.toUtf8();

    editor->setTargetRange(pos, editor->length());
    editor->setSearchFlags(search_flags);

    if (editor->searchInTarget(textData.length(), textData.constData()) != INVALID_POSITION) {
        return {static_cast<Sci_PositionCR>(editor->targetStart()), static_cast<Sci_PositionCR>(editor->targetEnd())};
    }
    else if (wrap) {
        editor->setTargetRange(0, pos);
        if (editor->searchInTarget(textData.length(), textData.constData()) != INVALID_POSITION) {
            return {static_cast<Sci_PositionCR>(editor->targetStart()), static_cast<Sci_PositionCR>(editor->targetEnd())};
        }
    }

    return {INVALID_POSITION, INVALID_POSITION};
}

Sci_CharacterRange Finder::findPrev()
{
    if (text.isEmpty())
        return {INVALID_POSITION, INVALID_POSITION};

    const int pos = editor->selectionStart();
    const QByteArray textData = text.toUtf8();

    editor->setTargetRange(pos, editor->length());
    editor->setSearchFlags(search_flags);

    auto range = editor->findText(editor->searchFlags(), textData.constData(), pos, 0);

    if (range.first != INVALID_POSITION) {
        return {static_cast<Sci_PositionCR>(range.first), static_cast<Sci_PositionCR>(range.second)};
    }
    else if (wrap) {
        range = editor->findText(editor->searchFlags(), textData.constData(), editor->length(), pos);
        if (range.first != INVALID_POSITION) {
            return {static_cast<Sci_PositionCR>(range.first), static_cast<Sci_PositionCR>(range.second)};
        }
    }

    return {INVALID_POSITION, INVALID_POSITION};
}

// Count all occurrences in the document
int Finder::count()
{
    int total = 0;

    if (text.length() > 0) {
        this->forEachMatch(text.toUtf8(), [&](int start, int end) {
            Q_UNUSED(start);
            total++;
            return end;
        });
    }

    return total;
}

Sci_CharacterRange Finder::replaceSelectionIfMatch(const QString &replaceText)
{
    const QByteArray textData = text.toUtf8();
    bool isRegex = editor->searchFlags() & SCFIND_REGEXP;

    // Search just in the selection to see if the current selection is a match
    editor->setTargetStart(editor->selectionStart());
    editor->setTargetEnd(editor->selectionEnd());
    editor->setSearchFlags(search_flags);

    if (editor->searchInTarget(textData.length(), textData.constData()) != INVALID_POSITION) {
        const QByteArray replaceData = replaceText.toUtf8();

        if (isRegex)
            editor->replaceTargetRE(replaceData.length(), replaceData.constData());
        else
            editor->replaceTarget(replaceData.length(), replaceData.constData());

        return {static_cast<Sci_PositionCR>(editor->targetStart()), static_cast<Sci_PositionCR>(editor->targetEnd())};
    }

    return {INVALID_POSITION, INVALID_POSITION};
}

int Finder::replaceAll(const QString &replaceText)
{
    if (text.isEmpty())
        return 0;

    const QByteArray replaceData = replaceText.toUtf8();
    bool isRegex = search_flags & SCFIND_REGEXP;
    int total = 0;

    editor->setSearchFlags(search_flags);

    editor->beginUndoAction();
    editor->forEachMatch(text, [&](int start, int end) {
        total++;
        editor->setTargetRange(start, end);

        if (isRegex)
            return start + editor->replaceTargetRE(replaceData.length(), replaceData.constData());
        else
            return start + editor->replaceTarget(replaceData.length(), replaceData.constData());
    });
    editor->endUndoAction();

    return total;
}
