//  Copyright (c) 2016 Hugo Amiard hugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

#include <toyui/Config.h>
#include <toyui/Window/Window.h>

#include <toyui/Window/Dockspace.h>

#include <toyui/Container/Layout.h>

#include <toyui/Frame/Frame.h>
#include <toyui/Frame/Stripe.h>
#include <toyui/Frame/Layer.h>

#include <toyui/Widget/RootSheet.h>
#include <toyui/Button/Slider.h>

#include <toyui/Input/InputDevice.h>

namespace toy
{
	Popup::Popup(Wedge& parent)
		: Overlay(parent, cls())
	{
		DimFloat local = m_parent->frame().localPosition(this->rootSheet().mouse().lastX(), this->rootSheet().mouse().lastY());
		m_frame->setPosition(local.x(), local.y());

		this->takeControl(CM_MODAL);
	}

	bool Popup::leftClick(MouseEvent& mouseEvent)
	{
		mouseEvent.abort = true;
		this->yieldControl();
		this->destroy();
		return true;
	}

	bool Popup::rightClick(MouseEvent& mouseEvent)
	{
		mouseEvent.abort = true;
		this->yieldControl();
		this->destroy();
		return true;
	}

	WindowHeader::WindowHeader(Window& window)
		: WrapControl(window, cls())
		, m_window(window)
		, m_tooltip("Drag me")
		, m_title(*this, m_window.name())
		, m_close(*this, std::bind(&Window::close, &m_window))
	{
		if(!m_window.closable())
			m_close.hide();
	}

	bool WindowHeader::leftClick(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		m_window.enableState(ACTIVATED);
		if(!m_window.dock()) // crashes for some reason
			m_window.frame().layer().moveToTop();
		return true;
	}

	bool WindowHeader::leftDragStart(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		if(m_window.dock())
			m_window.undock();

		m_window.frame().layer().moveToTop();
		m_window.frame().layer().setOpacity(HOLLOW);
		return true;
	}

	bool WindowHeader::leftDrag(MouseEvent& mouseEvent)
	{
		if(!m_window.movable())
			return true;

		m_window.frame().setPosition(m_window.frame().dposition(DIM_X) + mouseEvent.deltaX, m_window.frame().dposition(DIM_Y) + mouseEvent.deltaY);
		return true;
	}

	bool WindowHeader::leftDragEnd(MouseEvent& mouseEvent)
	{
		if(m_window.dockable())
		{
			Docksection* target = this->docktarget(mouseEvent);
			if(target)
				m_window.dock(*target);
		}

		m_window.frame().layer().setOpacity(OPAQUE);
		return true;
	}

	Docksection* WindowHeader::docktarget(MouseEvent& mouseEvent)
	{
		Widget* widget = this->rootSheet().pinpoint(mouseEvent.posX, mouseEvent.posY);
		Docksection* docksection = widget->findContainer<Docksection>();

		if(docksection)
			return &docksection->docktarget(mouseEvent.posX, mouseEvent.posY);
		else
			return nullptr;
	}

	WindowSizer::WindowSizer(Wedge& parent, Window& window, Type& type, bool left)
		: Control(parent, type)
		, m_window(window)
		, m_resizeLeft(left)
	{}

	bool WindowSizer::leftDragStart(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		m_window.frame().as<Layer>().moveToTop();
		return true;
	}

	bool WindowSizer::leftDrag(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		if(m_resizeLeft)
		{
			m_window.frame().setPositionDim(DIM_X, m_window.frame().dposition(DIM_X) + mouseEvent.deltaX);
			m_window.frame().setSize(std::max(10.f, m_window.frame().dsize(DIM_X) - mouseEvent.deltaX), std::max(25.f, m_window.frame().dsize(DIM_Y) + mouseEvent.deltaY));
		}
		else
		{
			m_window.frame().setSize(std::max(10.f, m_window.frame().dsize(DIM_X) + mouseEvent.deltaX), std::max(25.f, m_window.frame().dsize(DIM_Y) + mouseEvent.deltaY));
		}
		return true;
	}

	bool WindowSizer::leftDragEnd(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		return true;
	}

	WindowSizerLeft::WindowSizerLeft(Wedge& parent, Window& window)
		: WindowSizer(parent, window, cls(), true)
	{}

	WindowSizerRight::WindowSizerRight(Wedge& parent, Window& window)
		: WindowSizer(parent, window, cls(), false)
	{}

	WindowFooter::WindowFooter(Window& window)
		: WrapControl(window, cls())
		, m_firstSizer(*this, window)
		, m_secondSizer(*this, window)
	{}

	WindowBody::WindowBody(Wedge& parent)
		: ScrollSheet(parent, cls())
	{}

	CloseButton::CloseButton(Wedge& parent, const Callback& trigger)
		: Button(parent, "", trigger, cls())
	{}

	Window::Window(Wedge& parent, const string& title, WindowState state, const Callback& onClose, Docksection* dock, Type& type)
		: Overlay(parent, type)
		, m_name(title)
		, m_windowState(state)
		, m_onClose(onClose)
		, m_dock(dock)
		, m_header(*this)
		, m_body(*this)
		, m_footer(*this)
	{
		m_containerTarget = &m_body.target();
		
		if(!this->sizable())
			m_footer.hide();

		if(&type == &Window::cls())
		{
			m_frame->setFixedSize(DIM_X, 480.f);
			m_frame->setFixedSize(DIM_Y, 350.f);
		}

		if(!m_dock)
		{
			float x = (m_parent->frame().dsize(DIM_X) - m_frame->dsize(DIM_X)) / 2.f;
			float y = (m_parent->frame().dsize(DIM_Y) - m_frame->dsize(DIM_Y)) / 2.f;
			m_frame->setPosition(x, y);
		}
	}

	void Window::toggleWindowState(WindowState state)
	{
		m_windowState = static_cast<WindowState>(m_windowState ^ state);
	}

	void Window::toggleClosable()
	{
		m_header.close().frame().hidden() ? m_header.close().show() : m_header.close().hide();
	}

	void Window::toggleMovable()
	{
		this->toggleWindowState(WINDOW_MOVABLE);
	}

	void Window::toggleResizable()
	{
		this->toggleWindowState(WINDOW_SIZABLE);
		this->sizable() ? m_footer.show() : m_footer.hide();
	}

	void Window::toggleWrap()
	{
		this->toggleWindowState(WINDOW_SHRINK);
		this->shrink() ? m_body.enableWrap() : m_body.disableWrap();
		this->shrink() ? this->setStyle(WrapWindow::cls()) : this->setStyle(Window::cls());
	}

	void Window::showTitlebar()
	{
		m_header.show();
	}

	void Window::hideTitlebar()
	{
		m_header.hide();
	}

	void Window::dock(Docksection& docksection)
	{
		this->docked();
		m_dock = &docksection;
		docksection.dock(*this);
	}

	void Window::docked()
	{
		this->setStyle(DockWindow::cls());
		this->toggleMovable();
		this->toggleResizable();
	}

	void Window::undock()
	{
		m_dock->undock(*this);
		m_dock = nullptr;
		this->undocked();
	}

	void Window::undocked()
	{
		this->setStyle(Window::cls());
		this->toggleMovable();
		this->toggleResizable();

		DimFloat absolute = m_frame->absolutePosition();
		m_frame->setPosition(absolute[DIM_X], absolute[DIM_Y]);
		m_frame->as<Layer>().moveToTop();
	}
	
	void Window::close()
	{
		if(m_dock)
			this->undock();
		if(m_onClose)
			m_onClose(*this);
		this->destroy();
	}

	bool Window::leftClick(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		if(!m_dock)
			m_frame->as<Layer>().moveToTop();
		return true;
	}

	bool Window::rightClick(MouseEvent& mouseEvent)
	{
		UNUSED(mouseEvent);
		if(!m_dock)
			m_frame->as<Layer>().moveToTop();
		return true;
	}
}
