///////////////////////////////////////////////////////////////////////////////
//
// wxFormBuilder - A Visual Dialog Editor for wxWidgets.
// Copyright (C) 2005 José Antonio Hurtado
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// Written by
//   José Antonio Hurtado - joseantonio.hurtado@gmail.com
//   Juan Antonio Ortega  - jortegalalmolda@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#include <component.h>
#include <plugin.h>
#include <xrcconv.h>
#include <ticpp.h>

#include <wx/splitter.h>
#include <wx/listctrl.h>

// Includes notebook, listbook, choicebook, auibook
#include "bookutils.h"

///////////////////////////////////////////////////////////////////////////////

/**
Event handler for events generated by controls in this plugin
*/
class ComponentEvtHandler : public wxEvtHandler
{
private:
	wxWindow* m_window;
	IManager* m_manager;

public:
	ComponentEvtHandler( wxWindow* win, IManager* manager )
	:
	m_window( win ),
	m_manager( manager )
	{
	}

protected:
	void OnNotebookPageChanged( wxNotebookEvent& event );
	void OnListbookPageChanged( wxListbookEvent& event );
	void OnChoicebookPageChanged( wxChoicebookEvent& event );
	void OnAuiNotebookPageChanged( wxAuiNotebookEvent& event );
	void OnSplitterSashChanged( wxSplitterEvent& event );

	template < class T >
		void OnBookPageChanged( int selPage, wxEvent* event )
	{
		// Only handle events from this book - prevents problems with nested books, because OnSelected is fired on an
		// object and all of its parents
		if ( m_window != event->GetEventObject() )
		{
			return;
		}

		if ( selPage < 0 )
		{
			return;
		}

		size_t count = m_manager->GetChildCount( m_window );
		for ( size_t i = 0; i < count; i++ )
		{
			wxObject* wxChild = m_manager->GetChild( m_window, i );
			IObject*  iChild = m_manager->GetIObject( wxChild );
			if ( iChild )
			{
				if ( (int)i == selPage && !iChild->GetPropertyAsInteger( _("select") ) )
				{
					m_manager->ModifyProperty( wxChild, _("select"), wxT("1"), false );
				}
				else if ( (int)i != selPage && iChild->GetPropertyAsInteger( _("select") ) )
				{
					m_manager->ModifyProperty( wxChild, _("select"), wxT("0"), false );
				}
			}
		}

		// Select the corresponding panel in the object tree
		T* book = wxDynamicCast( m_window, T );
		if ( NULL != book )
		{
			m_manager->SelectObject( book->GetPage( selPage ) );
		}
	}

	void OnAuiNotebookPageClosed( wxAuiNotebookEvent& event )
	{
		wxMessageBox( wxT("wxAuiNotebook pages can normally be closed.\nHowever, it is difficult to design a page that has been closed, so this action has been vetoed."),
						wxT("Page Close Vetoed!"), wxICON_INFORMATION, NULL );
		event.Veto();
	}

	void OnAuiNotebookAllowDND( wxAuiNotebookEvent& event )
	{
		wxMessageBox( wxT("wxAuiNotebook pages can be dragged to other wxAuiNotebooks if the wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND event is caught and allowed.\nHowever, it is difficult to design a page that has been moved, so this action was not allowed."),
						wxT("Page Move Not Allowed!"), wxICON_INFORMATION, NULL );
		event.Veto();
	}

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( ComponentEvtHandler, wxEvtHandler )
	EVT_NOTEBOOK_PAGE_CHANGED( -1, ComponentEvtHandler::OnNotebookPageChanged )
	EVT_LISTBOOK_PAGE_CHANGED( -1, ComponentEvtHandler::OnListbookPageChanged )
	EVT_CHOICEBOOK_PAGE_CHANGED( -1, ComponentEvtHandler::OnChoicebookPageChanged )
	EVT_AUINOTEBOOK_PAGE_CHANGED( -1, ComponentEvtHandler::OnAuiNotebookPageChanged )
	EVT_AUINOTEBOOK_PAGE_CLOSE( -1, ComponentEvtHandler::OnAuiNotebookPageClosed )
	EVT_AUINOTEBOOK_ALLOW_DND( -1, ComponentEvtHandler::OnAuiNotebookAllowDND )
	EVT_SPLITTER_SASH_POS_CHANGED( -1, ComponentEvtHandler::OnSplitterSashChanged )
END_EVENT_TABLE()

///////////////////////////////////////////////////////////////////////////////

class wxCustomSplitterWindow : public wxSplitterWindow
{
public:
	wxCustomSplitterWindow(wxWindow* parent, wxWindowID id, const wxPoint& point = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style=wxSP_3D)
	:
	wxSplitterWindow( parent, id, point, size, style ),
	m_customSashPos( 0 ),
	m_customMinPaneSize( 0 )
	{
	}

	int m_customSashPos;
	int m_customMinPaneSize;
	int m_initialSashPos;

	// Used to ensure sash position is correct
	void OnIdle( wxIdleEvent& )
	{
		Disconnect( wxEVT_IDLE, wxIdleEventHandler( wxCustomSplitterWindow::OnIdle ) );

		// So the selection of the sizer at its initial position is cleared, then shown at the correct position
		Freeze();
		SetSashPosition( m_initialSashPos );
		Layout();
		Refresh();
		Update();
		Thaw();
	}

private:

	bool OnSashPositionChange( int newSashPosition )
	{
		m_customSashPos = newSashPosition;
		return wxSplitterWindow::OnSashPositionChange( newSashPosition );
	}

	void OnDoubleClickSash( int, int )
	{
		if ( 0 == m_customMinPaneSize )
		{
			wxMessageBox( wxT("Double-clicking a wxSplitterWindow sash with the minimum pane size set to 0 would normally unsplit it.\nHowever, it is difficult to design a pane that has been closed, so this action has been vetoed."),
					wxT("Unsplit Vetoed!"), wxICON_INFORMATION, NULL );
		}
	}

};

// Since wxGTK 2.8, wxNotebook has been sending page changed events in its destructor - this causes strange behavior
#if defined( __WXGTK__ ) && wxCHECK_VERSION( 2, 8, 0 )
	class wxCustomNotebook : public wxNotebook
	{
	public:
		wxCustomNotebook( wxWindow* parent, wxWindowID id, const wxPoint& point = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0 )
		:
		wxNotebook( parent, id, point, size, style )
		{
		}

		~wxCustomNotebook()
		{
			while ( GetEventHandler() != this )
			{
				// Remove and delete extra event handlers
				PopEventHandler( true );
			}
		}
	};
#else
	typedef wxNotebook wxCustomNotebook;
#endif

///////////////////////////////////////////////////////////////////////////////

class PanelComponent : public ComponentBase
{
public:

	wxObject* Create(IObject *obj, wxObject *parent)
	{
		wxPanel* panel = new wxPanel((wxWindow *)parent,-1,
			obj->GetPropertyAsPoint(_("pos")),
			obj->GetPropertyAsSize(_("size")),
			obj->GetPropertyAsInteger(_("style")) | obj->GetPropertyAsInteger(_("window_style")));
		return panel;
	}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("wxPanel"), obj->GetPropertyAsString(_("name")));
		xrc.AddWindowProperties();
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("wxPanel"));
		filter.AddWindowProperties();
		return filter.GetXfbObject();
	}

};

class SplitterWindowComponent : public ComponentBase
{
	wxObject* Create(IObject *obj, wxObject *parent)
	{
		wxCustomSplitterWindow *splitter =
			new wxCustomSplitterWindow((wxWindow *)parent,-1,
			obj->GetPropertyAsPoint(_("pos")),
			obj->GetPropertyAsSize(_("size")),
			(obj->GetPropertyAsInteger(_("style")) | obj->GetPropertyAsInteger(_("window_style"))) & ~wxSP_PERMIT_UNSPLIT );

		if ( !obj->IsNull( _("sashgravity") ) )
		{
			float gravity = obj->GetPropertyAsFloat( _("sashgravity") );
			gravity = ( gravity < 0.0 ? 0.0 : gravity );
			gravity = ( gravity > 1.0 ? 1.0 : gravity );
			splitter->SetSashGravity( gravity );
		}
#if wxVERSION_NUMBER < 2900
// From 2.9 docs: The sash size is platform-dependent because
// it conforms to the current platform look-and-feel and cannot be changed. 
		if ( !obj->IsNull( _("sashsize") ) )
		{
			splitter->SetSashSize( obj->GetPropertyAsInteger( _("sashsize") ) );
		}
#endif
		if ( !obj->IsNull( _("min_pane_size") ) )
		{
			int minPaneSize = obj->GetPropertyAsInteger( _("min_pane_size") );
			splitter->m_customMinPaneSize = minPaneSize;
			minPaneSize = ( minPaneSize < 1 ? 1 : minPaneSize );
			splitter->SetMinimumPaneSize( minPaneSize );
		}

		// Always have a child so it is drawn consistently
		splitter->Initialize( new wxPanel( splitter ) );

		// Used to ensure sash position is correct
		splitter->m_initialSashPos = obj->GetPropertyAsInteger( _("sashpos") );
		splitter->Connect( wxEVT_IDLE, wxIdleEventHandler( wxCustomSplitterWindow::OnIdle ) );

		return splitter;
	}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("wxSplitterWindow"), obj->GetPropertyAsString(_("name")));
		xrc.AddWindowProperties();
		xrc.AddProperty(_("sashpos"),_("sashpos"),XRC_TYPE_INTEGER);
		xrc.AddProperty(_("sashgravity"),_("gravity"),XRC_TYPE_FLOAT);
		xrc.AddProperty(_("min_pane_size"),_("minsize"),XRC_TYPE_INTEGER);
		if (obj->GetPropertyAsString(_("splitmode")) == wxT("wxSPLIT_VERTICAL"))
			xrc.AddPropertyValue(_("orientation"),wxT("vertical"));
		else
			xrc.AddPropertyValue(_("orientation"),wxT("horizontal"));

		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("wxSplitterWindow"));
		filter.AddWindowProperties();
		filter.AddProperty(_("sashpos"),_("sashpos"),XRC_TYPE_INTEGER);
		filter.AddProperty(_("gravity"),_("sashgravity"),XRC_TYPE_FLOAT);
		filter.AddProperty(_("minsize"),_("min_pane_size"),XRC_TYPE_INTEGER);
		try
		{
			ticpp::Element *splitmode = xrcObj->FirstChildElement("orientation");
			std::string value = splitmode->GetText();
			if (value == "vertical")
				filter.AddPropertyValue(wxT("splitmode"),wxT("wxSPLIT_VERTICAL"));
			else
				filter.AddPropertyValue(wxT("splitmode"),wxT("wxSPLIT_HORIZONTAL"));
		}
		catch( ticpp::Exception& )
		{
		}

		return filter.GetXfbObject();
	}

	void OnCreated( wxObject* wxobject, wxWindow* /*wxparent*/ )
	{
		wxCustomSplitterWindow* splitter = wxDynamicCast( wxobject, wxCustomSplitterWindow );
		if ( NULL == splitter )
		{
			wxLogError( _("This should be a wxSplitterWindow") );
			return;
		}

		// Remove default panel
		wxWindow* firstChild = splitter->GetWindow1();

		size_t childCount = GetManager()->GetChildCount( wxobject );
		switch ( childCount )
		{
			case 1:
			{
				// The child should be a splitteritem
				wxObject* splitterItem = GetManager()->GetChild( wxobject, 0 );

				// This one should be the actual wxWindow
				wxWindow* subwindow = wxDynamicCast( GetManager()->GetChild( splitterItem, 0 ), wxWindow );
				if ( NULL == subwindow )
				{
					wxLogError( _("A SplitterItem is abstract and must have a child!") );
					return;
				}

				if ( firstChild )
				{
					splitter->ReplaceWindow( firstChild, subwindow );
					firstChild->Destroy();
				}
				else
				{
					splitter->Initialize( subwindow );
				}
				splitter->PushEventHandler( new ComponentEvtHandler( splitter, GetManager() ) );
				break;
			}
			case 2:
			{
				// The child should be a splitteritem
				wxObject* splitterItem0 = GetManager()->GetChild( wxobject, 0 );
				wxObject* splitterItem1 = GetManager()->GetChild( wxobject, 1 );

				// This one should be the actual wxWindow
				wxWindow* subwindow0 = wxDynamicCast( GetManager()->GetChild( splitterItem0, 0 ), wxWindow );
				wxWindow* subwindow1 = wxDynamicCast( GetManager()->GetChild( splitterItem1, 0 ), wxWindow );

				if ( NULL == subwindow0 || NULL == subwindow1 )
				{
					wxLogError( _("A SplitterItem is abstract and must have a child!") );
					return;
				}

				// Get the split mode and sash position
				IObject* obj = GetManager()->GetIObject( wxobject );
				if ( obj == NULL )
				{
					return;
				}

				int sashPos = obj->GetPropertyAsInteger(_("sashpos"));
				int splitmode = obj->GetPropertyAsInteger(_("splitmode"));

				if ( firstChild )
				{
					splitter->ReplaceWindow( firstChild, subwindow0 );
					firstChild->Destroy();
				}

				if ( splitmode == wxSPLIT_VERTICAL )
				{
					splitter->SplitVertically( subwindow0, subwindow1, sashPos );
				}
				else
				{
					splitter->SplitHorizontally( subwindow0, subwindow1, sashPos );
				}

				splitter->PushEventHandler( new ComponentEvtHandler( splitter, GetManager() ) );
				break;
			}
			default:
				return;
		}
	}
};

void ComponentEvtHandler::OnSplitterSashChanged( wxSplitterEvent& )
{
	wxCustomSplitterWindow* window = wxDynamicCast( m_window, wxCustomSplitterWindow );
	if ( window != NULL )
	{
		if ( window->m_customSashPos != 0 )
		{
			m_manager->ModifyProperty( window, _("sashpos"), wxString::Format( wxT("%i"), window->GetSashPosition() ) );
		}
	}
}

class SplitterItemComponent : public ComponentBase
{
	ticpp::Element* ExportToXrc(IObject *obj)
	{
		// A __dummyitem__ will be ignored...
		ObjectToXrcFilter xrc(obj, _("__dummyitem__"),wxT(""));
		return xrc.GetXrcObject();
	}
};

class ScrolledWindowComponent : public ComponentBase
{
public:

    wxObject* Create(IObject *obj, wxObject *parent)
    {
        wxScrolledWindow *sw = new wxScrolledWindow((wxWindow *)parent, -1,
            obj->GetPropertyAsPoint(_("pos")),
            obj->GetPropertyAsSize(_("size")),
            obj->GetPropertyAsInteger(_("style")) | obj->GetPropertyAsInteger(_("window_style")));

        sw->SetScrollRate(
            obj->GetPropertyAsInteger(_("scroll_rate_x")),
            obj->GetPropertyAsInteger(_("scroll_rate_y")));
        return sw;
    }

    ticpp::Element* ExportToXrc(IObject *obj)
    {
        ObjectToXrcFilter xrc(obj, _("wxScrolledWindow"), obj->GetPropertyAsString(_("name")));
        xrc.AddWindowProperties();
		xrc.AddPropertyValue( _("scrollrate"), wxString::Format( wxT("%d,%d"),
				obj->GetPropertyAsInteger(_("scroll_rate_x")),
				obj->GetPropertyAsInteger(_("scroll_rate_y")) ) );
        return xrc.GetXrcObject();
    }

    ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
    {
        XrcToXfbFilter filter(xrcObj, _("wxScrolledWindow"));
        filter.AddWindowProperties();
		
		ticpp::Element *scrollrate = xrcObj->FirstChildElement("scrollrate");
		if( scrollrate ) {
			wxString value( wxString( scrollrate->GetText().c_str(), wxConvUTF8 ) );
			filter.AddPropertyValue( _("scroll_rate_x"), value.BeforeFirst( wxT(',') ) );
			filter.AddPropertyValue( _("scroll_rate_y"), value.AfterFirst( wxT(',') ) );
		}
        return filter.GetXfbObject();
    }
};

class NotebookComponent : public ComponentBase
{
public:
	wxObject* Create(IObject *obj, wxObject *parent)
	{
		wxNotebook* book = new wxCustomNotebook((wxWindow *)parent,-1,
			obj->GetPropertyAsPoint(_("pos")),
			obj->GetPropertyAsSize(_("size")),
			obj->GetPropertyAsInteger(_("style")) | obj->GetPropertyAsInteger(_("window_style")));

		BookUtils::AddImageList( obj, book );

		book->PushEventHandler( new ComponentEvtHandler( book, GetManager() ) );

		return book;
	}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("wxNotebook"), obj->GetPropertyAsString(_("name")));
		xrc.AddWindowProperties();
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("wxNotebook"));
		filter.AddWindowProperties();
		return filter.GetXfbObject();
	}
};

void ComponentEvtHandler::OnNotebookPageChanged( wxNotebookEvent& event )
{
	OnBookPageChanged< wxNotebook >( event.GetSelection(), &event );
	event.Skip();
}

class NotebookPageComponent : public ComponentBase
{
public:
	void OnCreated( wxObject* wxobject, wxWindow* wxparent )
	{
		BookUtils::OnCreated< wxNotebook >( wxobject, wxparent, GetManager(), _("NotebookPageComponent") );
	}

	void OnSelected( wxObject* wxobject )
	{
		BookUtils::OnSelected< wxNotebook >( wxobject, GetManager() );
	}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("notebookpage"));
		xrc.AddProperty(_("label"),_("label"),XRC_TYPE_TEXT);
		xrc.AddProperty(_("select"),_("selected"),XRC_TYPE_BOOL);
		if ( !obj->IsNull( _("bitmap") ) )
		{
			xrc.AddProperty(_("bitmap"),_("bitmap"),XRC_TYPE_BITMAP);
		}
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("notebookpage"));
		filter.AddWindowProperties();
		filter.AddProperty(_("label"),_("label"),XRC_TYPE_TEXT);
		filter.AddProperty(_("selected"),_("select"),XRC_TYPE_BOOL);
		filter.AddProperty(_("bitmap"),_("bitmap"),XRC_TYPE_BITMAP);
		return filter.GetXfbObject();
	}
};

class ListbookComponent : public ComponentBase
{
public:
	wxObject* Create(IObject *obj, wxObject *parent)
	{
		wxListbook* book = new wxListbook((wxWindow *)parent,-1,
			obj->GetPropertyAsPoint(_("pos")),
			obj->GetPropertyAsSize(_("size")),
			obj->GetPropertyAsInteger(_("style")) | obj->GetPropertyAsInteger(_("window_style")));

		BookUtils::AddImageList( obj, book );

		book->PushEventHandler( new ComponentEvtHandler( book, GetManager() ) );

		return book;
	}

// Small icon style not supported by GTK
#ifndef  __WXGTK__
	void OnCreated( wxObject* wxobject, wxWindow* wxparent )
	{
		wxListbook* book = wxDynamicCast( wxparent, wxListbook );
		if ( book )
		{
			// Small icon style if bitmapsize is not set
			IObject* obj = GetManager()->GetIObject( wxobject );
			if ( obj->GetPropertyAsString( _("bitmapsize") ).empty() )
			{
				wxListView* tmpListView = book->GetListView();
				long flags = tmpListView->GetWindowStyleFlag();
				flags = (flags & ~wxLC_ICON) | wxLC_SMALL_ICON;
				tmpListView->SetWindowStyleFlag( flags );
			}
		}
	}
#endif

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("wxListbook"), obj->GetPropertyAsString(_("name")));
		xrc.AddWindowProperties();
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("wxListbook"));
		filter.AddWindowProperties();
		return filter.GetXfbObject();
	}
};

void ComponentEvtHandler::OnListbookPageChanged( wxListbookEvent& event )
{
	OnBookPageChanged< wxListbook >( event.GetSelection(), &event );
	event.Skip();
}

class ListbookPageComponent : public ComponentBase
{
public:
	void OnCreated( wxObject* wxobject, wxWindow* wxparent )
	{
		BookUtils::OnCreated< wxListbook >( wxobject, wxparent, GetManager(), _("ListbookPageComponent") );
	}

	void OnSelected( wxObject* wxobject )
		{
		BookUtils::OnSelected< wxListbook >( wxobject, GetManager() );
				}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("listbookpage"));
		xrc.AddProperty(_("label"),_("label"),XRC_TYPE_TEXT);
		xrc.AddProperty(_("select"),_("selected"),XRC_TYPE_BOOL);
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("listbookpage"));
		filter.AddWindowProperties();
		filter.AddProperty(_("selected"),_("select"),XRC_TYPE_BOOL);
		filter.AddProperty(_("label"),_("label"),XRC_TYPE_TEXT);
		return filter.GetXfbObject();
	}
};

class ChoicebookComponent : public ComponentBase
{
public:
	wxObject* Create(IObject *obj, wxObject *parent)
	{
		wxChoicebook* book = new wxChoicebook((wxWindow *)parent,-1,
			obj->GetPropertyAsPoint(_("pos")),
			obj->GetPropertyAsSize(_("size")),
			obj->GetPropertyAsInteger(_("style")) | obj->GetPropertyAsInteger(_("window_style")));

		book->PushEventHandler( new ComponentEvtHandler( book, GetManager() ) );

		return book;
	}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("wxChoicebook"), obj->GetPropertyAsString(_("name")));
		xrc.AddWindowProperties();
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("wxChoicebook"));
		filter.AddWindowProperties();
		return filter.GetXfbObject();
	}
};

void ComponentEvtHandler::OnChoicebookPageChanged( wxChoicebookEvent& event )
{
	OnBookPageChanged< wxChoicebook >( event.GetSelection(), &event );
	event.Skip();
}

class ChoicebookPageComponent : public ComponentBase
{
public:
	void OnCreated( wxObject* wxobject, wxWindow* wxparent )
	{
		BookUtils::OnCreated< wxChoicebook >( wxobject, wxparent, GetManager(), _("ChoicebookPageComponent") );
	}

	void OnSelected( wxObject* wxobject )
	{
		BookUtils::OnSelected< wxChoicebook >( wxobject, GetManager() );
	}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("choicebookpage"));
		xrc.AddProperty(_("label"),_("label"),XRC_TYPE_TEXT);
		xrc.AddProperty(_("select"),_("selected"),XRC_TYPE_BOOL);
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("choicebookpage"));
		filter.AddWindowProperties();
		filter.AddProperty(_("selected"),_("select"),XRC_TYPE_BOOL);
		filter.AddProperty(_("label"),_("label"),XRC_TYPE_TEXT);
		return filter.GetXfbObject();
	}
};

class AuiNotebookComponent : public ComponentBase
{
public:
	wxObject* Create(IObject *obj, wxObject *parent)
	{
		wxAuiNotebook* book = new wxAuiNotebook((wxWindow *)parent,-1,
			obj->GetPropertyAsPoint(_("pos")),
			obj->GetPropertyAsSize(_("size")),
			obj->GetPropertyAsInteger(_("style")) | obj->GetPropertyAsInteger(_("window_style")));

		book->SetTabCtrlHeight( obj->GetPropertyAsInteger( _("tab_ctrl_height") ) );
		book->SetUniformBitmapSize( obj->GetPropertyAsSize( _("uniform_bitmap_size") ) );

		book->PushEventHandler( new ComponentEvtHandler( book, GetManager() ) );

		return book;
	}

#if wxVERSION_NUMBER >= 2905
	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("wxAuiNotebook"), obj->GetPropertyAsString(_("name")));
		xrc.AddWindowProperties();
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("wxAuiNotebook"));
		filter.AddWindowProperties();
		return filter.GetXfbObject();
	}
#endif
};

void ComponentEvtHandler::OnAuiNotebookPageChanged( wxAuiNotebookEvent& event )
{
	OnBookPageChanged< wxAuiNotebook >( event.GetSelection(), &event );
	event.Skip();
}

class AuiNotebookPageComponent : public ComponentBase
{
public:
	void OnCreated( wxObject* wxobject, wxWindow* wxparent )
	{
		// Easy read-only property access
		IObject* obj = GetManager()->GetIObject( wxobject );

		wxAuiNotebook* book = wxDynamicCast( wxparent, wxAuiNotebook );

		//This wouldn't compile in MinGW - strange
		///wxWindow* page = wxDynamicCast( manager->GetChild( wxobject, 0 ), wxWindow );

		// Do this instead
		wxObject* child = GetManager()->GetChild( wxobject, 0 );
		wxWindow* page = NULL;
		if ( child->IsKindOf(CLASSINFO(wxWindow)))
		{
			page = (wxWindow*)child;
		}

		// Error checking
		if ( !( obj && book && page ) )
		{
			wxLogError( _("AuiNotebookPageComponent is missing its wxFormBuilder object(%i), its parent(%i), or its child(%i)"), obj, book, page );
			return;
		}

		// Prevent event handling by wxFB - these aren't user generated events
		SuppressEventHandlers suppress( book );

		// Save selection
		int selection = book->GetSelection();
		const wxBitmap& bitmap = obj->IsNull( _("bitmap") ) ? wxNullBitmap : obj->GetPropertyAsBitmap( _("bitmap") );
		book->AddPage( page, obj->GetPropertyAsString( _("label") ), false, bitmap );

		if ( obj->GetPropertyAsString( _("select") ) == wxT("0") && selection >= 0 )
		{
			book->SetSelection(selection);
		}
		else
		{
			book->SetSelection( book->GetPageCount() - 1 );
		}
	}

	void OnSelected( wxObject* wxobject )
	{
		BookUtils::OnSelected< wxAuiNotebook >( wxobject, GetManager() );
	}

#if wxVERSION_NUMBER >= 2905
	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, _("notebookpage"));
		xrc.AddProperty(_("label"),_("label"), XRC_TYPE_TEXT);
		xrc.AddProperty(_("selected"),_("selected"), XRC_TYPE_BOOL);
		xrc.AddProperty(_("bitmap"),_("bitmap"), XRC_TYPE_BITMAP);
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, _("notebookpage"));
		filter.AddWindowProperties();
		filter.AddProperty(_("selected"),_("selected"),XRC_TYPE_BOOL);
		filter.AddProperty(_("label"),_("label"),XRC_TYPE_TEXT);
		filter.AddProperty(_("bitmap"),_("bitmap"),XRC_TYPE_BITMAP);
		return filter.GetXfbObject();
	}
#endif
};

class SimplebookComponent : public ComponentBase
{
public:
	wxObject* Create(IObject *obj, wxObject *parent)
	{
		return new wxSimplebook((wxWindow *)parent,-1,
			obj->GetPropertyAsPoint(_("pos")),
			obj->GetPropertyAsSize(_("size")),
			obj->GetPropertyAsInteger(_("window_style")));
	}
};

class SimplebookPageComponent : public ComponentBase
{
public:
	void OnCreated( wxObject* wxobject, wxWindow* wxparent )
	{
		BookUtils::OnCreated< wxSimplebook >( wxobject, wxparent, GetManager(), _("SimplebookPageComponent") );
	}

	void OnSelected( wxObject* wxobject )
	{
		BookUtils::OnSelected< wxSimplebook >( wxobject, GetManager() );
	}
};

///////////////////////////////////////////////////////////////////////////////

BEGIN_LIBRARY()


WINDOW_COMPONENT("wxPanel",PanelComponent)

WINDOW_COMPONENT("wxSplitterWindow",SplitterWindowComponent)
ABSTRACT_COMPONENT("splitteritem",SplitterItemComponent)

WINDOW_COMPONENT("wxScrolledWindow",ScrolledWindowComponent)

WINDOW_COMPONENT("wxNotebook", NotebookComponent)
ABSTRACT_COMPONENT("notebookpage",NotebookPageComponent)

WINDOW_COMPONENT("wxListbook", ListbookComponent)
ABSTRACT_COMPONENT("listbookpage", ListbookPageComponent)

WINDOW_COMPONENT("wxChoicebook", ChoicebookComponent)
ABSTRACT_COMPONENT("choicebookpage", ChoicebookPageComponent)

WINDOW_COMPONENT("wxAuiNotebook", AuiNotebookComponent)
ABSTRACT_COMPONENT("auinotebookpage", AuiNotebookPageComponent)

WINDOW_COMPONENT("wxSimplebook", SimplebookComponent)
ABSTRACT_COMPONENT("simplebookpage", SimplebookPageComponent)

// wxSplitterWindow
MACRO(wxSP_3D)
MACRO(wxSP_3DSASH)
MACRO(wxSP_3DBORDER)
MACRO(wxSP_BORDER)
MACRO(wxSP_NOBORDER)
MACRO(wxSP_NO_XP_THEME)
MACRO(wxSP_PERMIT_UNSPLIT)
MACRO(wxSP_LIVE_UPDATE)

MACRO(wxSPLIT_VERTICAL)
MACRO(wxSPLIT_HORIZONTAL)

// wxScrolledWindow
MACRO(wxHSCROLL);
MACRO(wxVSCROLL);

// wxNotebook
MACRO(wxNB_TOP)
MACRO(wxNB_LEFT)
MACRO(wxNB_RIGHT)
MACRO(wxNB_BOTTOM)
MACRO(wxNB_FIXEDWIDTH)
MACRO(wxNB_MULTILINE)
MACRO(wxNB_NOPAGETHEME)
#if wxVERSION_NUMBER < 3100
MACRO(wxNB_FLAT)
#endif

// wxListbook
MACRO(wxLB_TOP)
MACRO(wxLB_LEFT)
MACRO(wxLB_RIGHT)
MACRO(wxLB_BOTTOM)
MACRO(wxLB_DEFAULT)

// wxChoicebook
MACRO(wxCHB_TOP)
MACRO(wxCHB_LEFT)
MACRO(wxCHB_RIGHT)
MACRO(wxCHB_BOTTOM)
MACRO(wxCHB_DEFAULT)

// wxAuiNotebook
MACRO(wxAUI_NB_DEFAULT_STYLE)
MACRO(wxAUI_NB_TAB_SPLIT)
MACRO(wxAUI_NB_TAB_MOVE)
MACRO(wxAUI_NB_TAB_EXTERNAL_MOVE)
MACRO(wxAUI_NB_TAB_FIXED_WIDTH)
MACRO(wxAUI_NB_SCROLL_BUTTONS)
MACRO(wxAUI_NB_WINDOWLIST_BUTTON)
MACRO(wxAUI_NB_CLOSE_BUTTON)
MACRO(wxAUI_NB_CLOSE_ON_ACTIVE_TAB)
MACRO(wxAUI_NB_CLOSE_ON_ALL_TABS)
MACRO(wxAUI_NB_MIDDLE_CLICK_CLOSE)
MACRO(wxAUI_NB_TOP)
MACRO(wxAUI_NB_BOTTOM)

END_LIBRARY()
