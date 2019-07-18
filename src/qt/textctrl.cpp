/////////////////////////////////////////////////////////////////////////////
// Name:        src/qt/textctrl.cpp
// Author:      Mariano Reingart, Peter Most
// Copyright:   (c) 2010 wxWidgets dev team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#include "wx/textctrl.h"
#include "wx/settings.h"
#include "wx/qt/private/converter.h"
#include "wx/qt/private/winevent.h"
#include "wx/qt/private/utils.h"

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>

/*
 * Abstract base class for wxQtSingleLineEdit and wxQtMultiLineEdit.
 * This splits the polymorphic behaviour into two separate classes, avoiding
 * unnecessary branches.
 */
class wxQtEdit
{
public:
    virtual ~wxQtEdit() {}

    virtual bool IsModified() const = 0;
    virtual int GetNumberOfLines() const = 0;
    virtual wxString DoGetValue() const = 0;
    virtual long GetInsertionPoint() const = 0;
    virtual QWidget *GetHandle() const = 0;
    virtual int GetLineLength(long lineNo) const = 0;
    virtual wxString GetLineText(long lineNo) const = 0;
    virtual bool GetSelection(long *from, long *to) const = 0;
    virtual long XYToPosition(long x, long y) const = 0;
    virtual bool PositionToXY(long pos, long *x, long *y) const = 0;
    virtual QScrollArea *ScrollBarsContainer() const = 0;
    virtual void WriteText( const wxString &text ) = 0;
    virtual void MarkDirty() = 0;
    virtual void DiscardEdits() = 0;
    virtual void blockSignals(bool block) = 0;
    virtual void SetValue( const wxString &value ) = 0;
    virtual void SetSelection( long from, long to ) = 0;
    virtual void SetInsertionPoint(long pos) = 0;
    virtual void SetStyleFlags(long flags) = 0;
    virtual bool SetStyle(int start, int end, const wxTextAttr &attr) = 0;
    virtual bool GetStyle(long position, wxTextAttr &style) = 0;
    virtual bool SetDefaultStyle(const wxTextAttr& style) = 0;
    bool IsHTML() const { return m_isHTML; };
    void SetHTMLContent(bool html) { m_isHTML = html; };
protected:
    void QtSetCharFormat(const wxTextAttr &style, int start, int end);
private:
    bool m_isHTML;
};

namespace
{

// Helper for SetStyleFlags(): takes care of flags that are handled in the same
// way in both QLineEdit and QTextEdit.
template <typename Edit>
void ApplyCommonStyles(Edit* edit, long flags)
{
    edit->setReadOnly(flags & wxTE_READONLY);

    if ( flags & wxTE_CENTRE )
        edit->setAlignment(Qt::AlignHCenter);
    else if ( flags & wxTE_RIGHT )
        edit->setAlignment(Qt::AlignRight);
    else // wxTE_LEFT is 0, so can't test for it, just use it by default
        edit->setAlignment(Qt::AlignLeft);
}


struct wxQtLineInfo
{
  wxQtLineInfo(size_t start, size_t end) :
    startPos(start),
    endPos(end)
  {
  }

  size_t startPos, endPos;
};

class wxQtLineEdit : public wxQtEventSignalHandler< QLineEdit, wxTextCtrl >
{
public:
    wxQtLineEdit( wxWindow *parent, wxTextCtrl *handler );

private:
    void textChanged();
    void returnPressed();
};

class wxQtTextEdit : public wxQtEventSignalHandler< QTextEdit, wxTextCtrl >
{
public:
    wxQtTextEdit( wxWindow *parent, wxTextCtrl *handler );

private:
    void textChanged();
};

class wxQtMultiLineEdit : public wxQtEdit
{
public:
    explicit wxQtMultiLineEdit(QTextEdit *edit) : m_edit(edit)
    {
    }

    virtual bool GetStyle(long position, wxTextAttr &style)
    {
        if( !m_edit->acceptRichText() )
            return false;
        QTextCursor cursor( m_edit->textCursor() );
        cursor.setPosition( position, QTextCursor::MoveAnchor );
        QTextCharFormat format = cursor.charFormat();
        wxFont font( format.font() );
        if( font.IsOk() )
            style.SetFont( font );
        wxColour foreColor = wxColour( format.foreground().color() );
        if( foreColor.IsOk() )
            style.SetTextColour( foreColor );
        wxColour backColour = wxColour( format.background().color() );
        if( backColour.IsOk() )
            style.SetBackgroundColour( backColour );
        Qt::Alignment align = m_edit->alignment();
        switch (align)
        {
            case Qt::AlignRight:
                style.SetAlignment( wxTEXT_ALIGNMENT_RIGHT );
                break;
            case Qt::AlignCenter:
                style.SetAlignment( wxTEXT_ALIGNMENT_CENTER );
                break;
            case Qt::AlignJustify:
                style.SetAlignment( wxTEXT_ALIGNMENT_JUSTIFIED );
                break;
            default:
                style.SetAlignment( wxTEXT_ALIGNMENT_LEFT );
                break;
        }
        return true;
    }

    virtual bool SetStyle(int start, int end, const wxTextAttr &style)
    {
        if( !m_edit->acceptRichText() )
            return false;
        if( start > end )
            wxSwap( start, end );
        QtSetCharFormat( style, start, end );
        if( style.HasAlignment() )
        {
            Qt::Alignment align;
            switch (style.GetAlignment())
            {
                case wxTEXT_ALIGNMENT_RIGHT:
                    align = Qt::AlignRight;
                    break;
                case wxTEXT_ALIGNMENT_CENTER:
                    align = Qt::AlignCenter;
                    break;
                case wxTEXT_ALIGNMENT_JUSTIFIED:
                    align = Qt::AlignJustify;
                    break;
                default:
                    align = Qt::AlignLeft;
                    break;
            }
            m_edit->setAlignment( align );
        }
        if( style.HasLeftIndent() )
        {
// indentation
        }
        if( style.HasTabs() )
        {
// tabs
        }
        return true;
    }

    virtual bool SetDefaultStyle(const wxTextAttr& style)
    {
        m_isDefaultStyle = true;
        int start = DoGetValue().length(), end = DoGetValue().length();
//        if( style.HasTextColour() )
//            m_edit->setTextColor( style.GetTextColour().GetQColor() );
        QtSetCharFormat( style, start, end );
//        m_edit->setCurrentCharFormat( 
        m_isDefaultStyle = false;
        return true;
    }

    void QtSetCharFormat(const wxTextAttr &style, int start, int end)
    {
        QTextCursor cursor( m_edit->textCursor() );
        cursor.setPosition( start, QTextCursor::MoveAnchor );
        if( start != end )
            cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor, end - start );
        else
            cursor.movePosition( QTextCursor::Right, QTextCursor::KeepAnchor, end );
        QTextCharFormat format;
        if( style.HasFont() )
        {
            wxFont font( style.GetFont() );
            if( font.IsOk() )
            {
                format.setFont( font.GetHandle() );
            }
            switch( font.GetWeight() )
            {
                case wxFONTWEIGHT_THIN:
                    format.setFontWeight( QFont::Thin );
                    break;
                case wxFONTWEIGHT_EXTRALIGHT:
                    format.setFontWeight( QFont::ExtraLight );
                    break;
                 case wxFONTWEIGHT_LIGHT:
                    format.setFontWeight( QFont::Light );
                    break;
                case wxFONTWEIGHT_NORMAL:
                    format.setFontWeight( QFont::Normal );
                    break;
                case wxFONTWEIGHT_MEDIUM:
                    format.setFontWeight( QFont::Medium );
                    break;
                case wxFONTWEIGHT_SEMIBOLD:
                    format.setFontWeight( QFont::DemiBold );
                    break;
                case wxFONTWEIGHT_BOLD:
                    format.setFontWeight( QFont::Bold );
                    break;
                case wxFONTWEIGHT_EXTRABOLD:
                    format.setFontWeight( QFont::ExtraBold );
                    break;
                case wxFONTWEIGHT_HEAVY :
                    format.setFontWeight( QFont::Black );
                    break;
                case wxFONTWEIGHT_EXTRAHEAVY:
                    format.setFontWeight( QFont::Black );
                    break;
                default:
                    break;
           }
           switch( font.GetStyle() )
           {
               case wxFONTSTYLE_NORMAL:
                   format.setFontItalic( false );
                   break;
               case wxFONTSTYLE_ITALIC:
               case wxFONTSTYLE_SLANT:
                   format.setFontItalic( true );
                   break;
               default:
                   break;
           }
        }
        if( style.HasTextColour() )
        {
            format.setForeground( QBrush( style.GetTextColour().GetQColor() ) );
        }
        if( style.HasBackgroundColour() )
        {
            format.setBackground( QBrush( style.GetBackgroundColour().GetQColor() ) );
        }
        if( !m_isDefaultStyle )
            cursor.mergeCharFormat( format );
        else
            m_edit->setCurrentCharFormat( format );
    }

    virtual bool IsModified() const wxOVERRIDE
    {
        return m_edit->isWindowModified();
    }

    virtual wxString DoGetValue() const wxOVERRIDE
    {
        return wxQtConvertString( m_edit->toPlainText() );
    }

    virtual long GetInsertionPoint() const wxOVERRIDE
    {
        QTextCursor cursor = m_edit->textCursor();
        return cursor.anchor();
    }

    virtual QWidget *GetHandle() const wxOVERRIDE
    {
        return m_edit;
    }

    virtual int GetNumberOfLines() const wxOVERRIDE
    {
        const wxString &value = DoGetValue();
        return std::count(value.begin(), value.end(), '\n') + 1;
    }

    virtual int GetLineLength(long lineNo) const wxOVERRIDE
    {
        wxQtLineInfo start = GetLineInfo(lineNo, DoGetValue());
        if ( start.startPos == wxString::npos )
            return -1;

        return start.endPos - start.startPos;
    }

    virtual wxString GetLineText(long lineNo) const wxOVERRIDE
    {
        const wxString &value = DoGetValue();

        wxQtLineInfo start = GetLineInfo(lineNo, value);
        if ( start.startPos == wxString::npos )
            return wxString();

        return value.Mid(start.startPos, start.endPos - start.startPos);
    }

    virtual long XYToPosition(long x, long y) const wxOVERRIDE
    {
        if ( x < 0 || y < 0 )
            return -1;

        wxQtLineInfo start = GetLineInfo(y, DoGetValue());
        if ( start.startPos == wxString::npos )
            return -1;

        if ( start.endPos - start.startPos < static_cast<size_t>(x) )
            return -1;

        return start.startPos + x;
    }

    virtual bool PositionToXY(long pos, long *x, long *y) const wxOVERRIDE
    {
        const wxString &value = DoGetValue();

        if ( static_cast<size_t>(pos) > value.length() )
          return false;

        int cnt = 0;
        int xval = 0;

        for ( long xx = 0; xx < pos; xx++ )
        {
            if ( value[xx] == '\n' )
            {
                xval = 0;
                cnt++;
            }
            else xval++;
        }
        *y = cnt;
        *x = xval;

        return true;
    }
    virtual void WriteText( const wxString &text ) wxOVERRIDE
    {
        m_edit->insertPlainText(wxQtConvertString( text ));
        // the cursor is moved to the end, ensure it is shown
        m_edit->ensureCursorVisible();
    }

    virtual void MarkDirty() wxOVERRIDE
    {
        return m_edit->setWindowModified( true );
    }

    virtual void DiscardEdits() wxOVERRIDE
    {
        return m_edit->setWindowModified( false );
    }

    virtual void blockSignals(bool block) wxOVERRIDE
    {
        m_edit->blockSignals(block);
    }

    virtual void SetValue( const wxString &value ) wxOVERRIDE
    {
        if( IsHTML() )
            m_edit->setHtml(wxQtConvertString( value ));
        else
            m_edit->setPlainText(wxQtConvertString( value ));
        // the cursor is moved to the end, ensure it is shown
        m_edit->ensureCursorVisible();
    }

    virtual void SetSelection( long from, long to ) wxOVERRIDE
    {
        QTextCursor cursor = m_edit->textCursor();
        cursor.setPosition(from);

        cursor.setPosition(to, QTextCursor::KeepAnchor);
        m_edit->setTextCursor(cursor);
    }

    virtual bool GetSelection( long *from, long *to ) const wxOVERRIDE
    {
        QTextCursor cursor = m_edit->textCursor();
        *from = cursor.selectionStart();
        *to = cursor.selectionEnd();
        return cursor.hasSelection();
    }

    virtual void SetInsertionPoint(long pos) wxOVERRIDE
    {
        QTextCursor::MoveOperation op;

        // check if pos indicates end of text:
        if ( pos == -1 )
        {
            pos = 0;
            op = QTextCursor::End;
        }
        else
        {
            op = QTextCursor::Start;
        }

        QTextCursor cursor = m_edit->textCursor();
        cursor.movePosition(op, QTextCursor::MoveAnchor, pos);

        if (op != QTextCursor::End )
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, pos);
        m_edit->setTextCursor(cursor);
        m_edit->ensureCursorVisible();
    }

    QScrollArea *ScrollBarsContainer() const wxOVERRIDE
    {
        return (QScrollArea *) m_edit;
    }

    virtual void SetStyleFlags(long flags) wxOVERRIDE
    {
        ApplyCommonStyles(m_edit, flags);

        if ( flags & wxNO_BORDER )
            m_edit->setFrameStyle(QFrame::NoFrame);

        if ( flags & wxTE_RICH || flags & wxTE_RICH2 )
            m_edit->setAcceptRichText(true);

        if( flags & wxTE_AUTO_URL )
            SetHTMLContent( true );
        else
            SetHTMLContent( false );
    }

private:
    wxQtLineInfo GetLineInfo(long lineNo, const wxString &value) const
    {
        size_t pos = 0;
        long cnt = 0;

        while ( cnt < lineNo )
        {
            size_t tpos = value.find('\n', pos);
            if ( tpos == wxString::npos )
              return wxQtLineInfo(tpos, tpos);

            pos = tpos + 1;
            cnt++;
        }

        size_t end = value.find('\n', pos);
        if ( end == wxString::npos )
            end = value.length();

        return wxQtLineInfo(pos, end);
    }

    QTextEdit* const m_edit;
    bool m_isDefaultStyle;
    wxDECLARE_NO_COPY_CLASS(wxQtMultiLineEdit);
};

class wxQtSingleLineEdit : public wxQtEdit
{
public:
    explicit wxQtSingleLineEdit(QLineEdit *edit) :
        m_edit(edit)
    {
    }

    virtual bool IsModified() const wxOVERRIDE
    {
        return m_edit->isModified();
    }

    virtual int GetNumberOfLines() const wxOVERRIDE
    {
        return 1;
    }

    virtual wxString DoGetValue() const wxOVERRIDE
    {
        return wxQtConvertString( m_edit->text() );
    }

    virtual long GetInsertionPoint() const wxOVERRIDE
    {
        long selectionStart = m_edit->selectionStart();

        if ( selectionStart >= 0 )
            return selectionStart;

        return m_edit->cursorPosition();
    }

    virtual QWidget *GetHandle() const wxOVERRIDE
    {
        return m_edit;
    }

    virtual int GetLineLength(long WXUNUSED(lineNo)) const wxOVERRIDE
    {
        return DoGetValue().length();
    }

    virtual wxString GetLineText(long lineNo) const wxOVERRIDE
    {
        return lineNo == 0 ? DoGetValue() : wxString();
    }

    virtual void WriteText( const wxString &text ) wxOVERRIDE
    {
        m_edit->insert(wxQtConvertString( text ));
    }

    virtual void MarkDirty() wxOVERRIDE
    {
        return m_edit->setModified( true );
    }

    virtual void DiscardEdits() wxOVERRIDE
    {
        return m_edit->setModified( false );
    }

    virtual void blockSignals(bool block) wxOVERRIDE
    {
        m_edit->blockSignals(block);
    }

    virtual void SetValue( const wxString &value ) wxOVERRIDE
    {
        m_edit->setText(wxQtConvertString( value ));
    }

    virtual void SetSelection( long from, long to ) wxOVERRIDE
    {
        m_edit->setSelection(from, to - from);
    }

    virtual bool GetSelection( long *from, long *to ) const wxOVERRIDE
    {
        *from = m_edit->selectionStart();
        if ( *from < 0 )
            return false;

        *to = *from + m_edit->selectedText().length();
        return true;
    }

    virtual void SetInsertionPoint(long pos) wxOVERRIDE
    {
        // check if pos indicates end of text:
        if ( pos == -1 )
            m_edit->end(false);
        else
            m_edit->setCursorPosition(pos);
    }

    virtual long XYToPosition(long x, long y) const wxOVERRIDE
    {
        if ( y == 0 && x >= 0 )
        {
            if ( static_cast<size_t>(x) <= DoGetValue().length() )
              return x;
        }

        return -1;
    }

    virtual bool PositionToXY(long pos, long *x, long *y) const wxOVERRIDE
    {
        const wxString &value = DoGetValue();

        if ( static_cast<size_t>(pos) > value.length() )
          return false;

        *y = 0;
        *x = pos;
        return true;
    }

    virtual QScrollArea *ScrollBarsContainer() const wxOVERRIDE
    {
        return NULL;
    }

    virtual void SetStyleFlags(long flags) wxOVERRIDE
    {
        ApplyCommonStyles(m_edit, flags);

        m_edit->setFrame(!(flags & wxNO_BORDER));

        if ( flags & wxTE_PASSWORD )
            m_edit->setEchoMode(QLineEdit::Password);
    }

    virtual bool GetStyle(long WXUNUSED(position), wxTextAttr &WXUNUSED(style))
    {
        return false;
    }

    virtual bool SetStyle(int WXUNUSED(start), int WXUNUSED(end), const wxTextAttr &WXUNUSED(style))
    {
        return false;
    }

    virtual bool SetDefaultStyle(const wxTextAttr& WXUNUSED(style))
    {
        return false;
    }
    
private:
    QLineEdit *m_edit;

    wxDECLARE_NO_COPY_CLASS(wxQtSingleLineEdit);
};

wxQtLineEdit::wxQtLineEdit( wxWindow *parent, wxTextCtrl *handler )
    : wxQtEventSignalHandler< QLineEdit, wxTextCtrl >( parent, handler )
{
    connect(this, &QLineEdit::textChanged,
            this, &wxQtLineEdit::textChanged);
    connect(this, &QLineEdit::returnPressed,
            this, &wxQtLineEdit::returnPressed);
}

void wxQtLineEdit::textChanged()
{
    wxTextEntryBase *handler = GetHandler();
    if ( handler )
    {
        handler->SendTextUpdatedEventIfAllowed();
    }
}

void wxQtLineEdit::returnPressed()
{
    wxTextCtrl *handler = GetHandler();
    if ( handler )
    {
        if ( handler->HasFlag(wxTE_PROCESS_ENTER) )
        {
            wxCommandEvent event( wxEVT_TEXT_ENTER, handler->GetId() );
            event.SetString( handler->GetValue() );
            EmitEvent( event );
        }
    }
}

wxQtTextEdit::wxQtTextEdit( wxWindow *parent, wxTextCtrl *handler )
    : wxQtEventSignalHandler< QTextEdit, wxTextCtrl >( parent, handler )
{
    connect(this, &QTextEdit::textChanged,
            this, &wxQtTextEdit::textChanged);
}

void wxQtTextEdit::textChanged()
{
    wxTextEntryBase *handler = GetHandler();
    if ( handler )
    {
        handler->SendTextUpdatedEventIfAllowed();
    }
}

} // anonymous namespace


wxTextCtrl::wxTextCtrl() :
    m_qtEdit(NULL)
{
}

wxTextCtrl::wxTextCtrl(wxWindow *parent,
           wxWindowID id,
           const wxString &value,
           const wxPoint &pos,
           const wxSize &size,
           long style,
           const wxValidator& validator,
           const wxString &name)
{
    Create( parent, id, value, pos, size, style, validator, name );
}

bool wxTextCtrl::Create(wxWindow *parent,
            wxWindowID id,
            const wxString &value,
            const wxPoint &pos,
            const wxSize &size,
            long style,
            const wxValidator& validator,
            const wxString &name)
{
    if ( style & wxTE_MULTILINE )
    {
        m_qtEdit = new wxQtMultiLineEdit(new wxQtTextEdit(parent, this));
    }
    else
    {
        m_qtEdit = new wxQtSingleLineEdit(new wxQtLineEdit(parent, this));
    }

    m_qtEdit->SetStyleFlags(style);

    if ( QtCreateControl( parent, id, pos, size, style, validator, name ) )
    {
        // set the initial text value without sending the event:
        // (done here as needs CreateBase called to set flags for IsMultiLine)
        ChangeValue( value );
        // set the default inner color (white), as it is replaced by PostCreation
        SetBackgroundColour( wxSystemSettingsNative::GetColour( wxSYS_COLOUR_LISTBOX ) );
        return true;
    }
    return false;
}

wxTextCtrl::~wxTextCtrl()
{
    delete m_qtEdit;
}

wxSize wxTextCtrl::DoGetBestSize() const
{
    return wxTextCtrlBase::DoGetBestSize();
}

int wxTextCtrl::GetLineLength(long lineNo) const
{
    return m_qtEdit->GetLineLength(lineNo);
}

wxString wxTextCtrl::GetLineText(long lineNo) const
{
    return m_qtEdit->GetLineText(lineNo);
}

int wxTextCtrl::GetNumberOfLines() const
{
    return m_qtEdit->GetNumberOfLines();
}

bool wxTextCtrl::IsModified() const
{
    return m_qtEdit->IsModified();
}

void wxTextCtrl::MarkDirty()
{
    m_qtEdit->MarkDirty();
}

void wxTextCtrl::DiscardEdits()
{
    m_qtEdit->DiscardEdits();
}

bool wxTextCtrl::SetStyle(long start, long end, const wxTextAttr& style)
{
    return m_qtEdit->SetStyle( start, end, style ); 
}

bool wxTextCtrl::GetStyle(long position, wxTextAttr& style)
{
    return m_qtEdit->GetStyle( position, style );
}

bool wxTextCtrl::SetDefaultStyle(const wxTextAttr& style)
{
    if ( !wxTextCtrlBase::SetDefaultStyle(style) )
        return false;

    if ( IsEditable() )
    {
        // we have to do this or the style wouldn't apply for the text typed by
        // the user
        m_qtEdit->SetDefaultStyle(m_defaultStyle);
    }

    return true;
}

long wxTextCtrl::XYToPosition(long x, long y) const
{
    return m_qtEdit->XYToPosition(x, y);
}

bool wxTextCtrl::PositionToXY(long pos, long *x, long *y) const
{
    if ( x == NULL || y == NULL || pos < 0 )
        return false;

    return m_qtEdit->PositionToXY(pos, x, y);
}

void wxTextCtrl::ShowPosition(long WXUNUSED(pos))
{
}

bool wxTextCtrl::DoLoadFile(const wxString& WXUNUSED(file), int WXUNUSED(fileType))
{
    return false;
}

bool wxTextCtrl::DoSaveFile(const wxString& WXUNUSED(file), int WXUNUSED(fileType))
{
    return false;
}

void wxTextCtrl::SetInsertionPoint(long pos)
{
    m_qtEdit->SetInsertionPoint(pos);
}

long wxTextCtrl::GetInsertionPoint() const
{
    return m_qtEdit->GetInsertionPoint();
}

wxString wxTextCtrl::DoGetValue() const
{
    return m_qtEdit->DoGetValue();
}

void wxTextCtrl::SetSelection( long from, long to )
{
    // SelectAll uses -1 to -1, adjust for qt:
    if ( to == -1 )
        to = GetValue().length();

    if ( from == -1 )
        from = 0;

    m_qtEdit->SetSelection( from, to );
}

void wxTextCtrl::GetSelection(long* from, long* to) const
{
    if ( m_qtEdit->GetSelection(from, to) )
        return;
    // No selection, call base for default behaviour:
    wxTextEntry::GetSelection(from, to);
}

void wxTextCtrl::WriteText( const wxString &text )
{
    // Insert the text
    m_qtEdit->WriteText(text);
}

void wxTextCtrl::DoSetValue( const wxString &text, int flags )
{
    // do not fire qt signals for certain methods (i.e. ChangeText)
    if ( !(flags & SetValue_SendEvent) )
    {
        m_qtEdit->blockSignals(true);
    }

    m_qtEdit->SetValue( text );

    // re-enable qt signals
    if ( !(flags & SetValue_SendEvent) )
    {
        m_qtEdit->blockSignals(false);
    }
    SetInsertionPoint(0);
}

QWidget *wxTextCtrl::GetHandle() const
{
    return (QWidget *) m_qtEdit->GetHandle();
}

QScrollArea *wxTextCtrl::QtGetScrollBarsContainer() const
{
    return m_qtEdit->ScrollBarsContainer();
}
