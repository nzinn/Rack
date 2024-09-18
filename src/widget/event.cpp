#include <widget/event.hpp>
#include <widget/Widget.hpp>
#include <context.hpp>
#include <window/Window.hpp>
#include <system.hpp>


namespace rack {
namespace widget {


std::string getKeyName(int key) {
	// glfwGetKeyName overrides
	switch (key) {
		case GLFW_KEY_SPACE: return "Space";
	}

	const char* keyNameC = glfwGetKeyName(key, GLFW_KEY_UNKNOWN);
	if (keyNameC) {
		std::string keyName = keyNameC;
		if (keyName.size() == 1)
			keyName[0] = std::toupper((unsigned char) keyName[0]);
		return keyName;
	}

	// Unprintable keys with names
	switch (key) {
		case GLFW_KEY_ESCAPE: return "Escape";
		case GLFW_KEY_ENTER: return "Enter";
		case GLFW_KEY_TAB: return "Tab";
		case GLFW_KEY_BACKSPACE: return "Backspace";
		case GLFW_KEY_INSERT: return "Insert";
		case GLFW_KEY_DELETE: return "Delete";
		case GLFW_KEY_RIGHT: return "Right";
		case GLFW_KEY_LEFT: return "Left";
		case GLFW_KEY_DOWN: return "Down";
		case GLFW_KEY_UP: return "Up";
		case GLFW_KEY_PAGE_UP: return "Page Up";
		case GLFW_KEY_PAGE_DOWN: return "Page Down";
		case GLFW_KEY_HOME: return "Home";
		case GLFW_KEY_END: return "End";
		case GLFW_KEY_KP_ENTER: return "Enter";
	}

	if (GLFW_KEY_F1 <= key && key <= GLFW_KEY_F25)
		return string::f("F%d", key - GLFW_KEY_F1 + 1);

	return "";
}


std::string getKeyCommandName(int key, int mods) {
	std::string modsName;
	if (mods & RACK_MOD_CTRL) {
		modsName += RACK_MOD_CTRL_NAME "+";
	}
	if (mods & GLFW_MOD_SHIFT) {
		modsName += RACK_MOD_SHIFT_NAME "+";
	}
	if (mods & GLFW_MOD_ALT) {
		modsName += RACK_MOD_ALT_NAME "+";
	}
	return modsName + getKeyName(key);
}


void EventState::setHoveredWidget(widget::Widget* w) {
	if (w == hoveredWidget)
		return;

	if (hoveredWidget) {
		// Dispatch LeaveEvent
		Widget::LeaveEvent eLeave;
		hoveredWidget->onLeave(eLeave);
		hoveredWidget = NULL;
	}

	if (w) {
		// Dispatch EnterEvent
		EventContext cEnter;
		cEnter.target = w;
		Widget::EnterEvent eEnter;
		eEnter.context = &cEnter;
		w->onEnter(eEnter);
		hoveredWidget = cEnter.target;
	}
}

void EventState::setDraggedWidget(widget::Widget* w, int button) {
	if (w == draggedWidget)
		return;

	if (draggedWidget) {
		// Dispatch DragEndEvent
		Widget::DragEndEvent eDragEnd;
		eDragEnd.button = dragButton;
		draggedWidget->onDragEnd(eDragEnd);
		draggedWidget = NULL;
	}

	dragButton = button;

	if (w) {
		// Dispatch DragStartEvent
		EventContext cDragStart;
		cDragStart.target = w;
		Widget::DragStartEvent eDragStart;
		eDragStart.context = &cDragStart;
		eDragStart.button = dragButton;
		w->onDragStart(eDragStart);
		draggedWidget = cDragStart.target;
	}
}

void EventState::setDragHoveredWidget(widget::Widget* w) {
	if (w == dragHoveredWidget)
		return;

	if (dragHoveredWidget) {
		// Dispatch DragLeaveEvent
		Widget::DragLeaveEvent eDragLeave;
		eDragLeave.button = dragButton;
		eDragLeave.origin = draggedWidget;
		dragHoveredWidget->onDragLeave(eDragLeave);
		dragHoveredWidget = NULL;
	}

	if (w) {
		// Dispatch DragEnterEvent
		EventContext cDragEnter;
		cDragEnter.target = w;
		Widget::DragEnterEvent eDragEnter;
		eDragEnter.context = &cDragEnter;
		eDragEnter.button = dragButton;
		eDragEnter.origin = draggedWidget;
		w->onDragEnter(eDragEnter);
		dragHoveredWidget = cDragEnter.target;
	}
}

void EventState::setSelectedWidget(widget::Widget* w) {
	if (w == selectedWidget)
		return;

	if (selectedWidget) {
		// Dispatch DeselectEvent
		Widget::DeselectEvent eDeselect;
		selectedWidget->onDeselect(eDeselect);
		selectedWidget = NULL;
	}

	if (w) {
		// Dispatch SelectEvent
		EventContext cSelect;
		cSelect.target = w;
		Widget::SelectEvent eSelect;
		eSelect.context = &cSelect;
		w->onSelect(eSelect);
		selectedWidget = cSelect.target;
	}
}

void EventState::finalizeWidget(widget::Widget* w) {
	if (hoveredWidget == w)
		setHoveredWidget(NULL);
	if (draggedWidget == w)
		setDraggedWidget(NULL, 0);
	if (dragHoveredWidget == w)
		setDragHoveredWidget(NULL);
	if (selectedWidget == w)
		setSelectedWidget(NULL);
	if (lastClickedWidget == w)
		lastClickedWidget = NULL;
}

bool EventState::handleButton(math::Vec pos, int button, int action, int mods) {
	bool cursorLocked = APP->window->isCursorLocked();

	widget::Widget* clickedWidget = NULL;
	if (!cursorLocked) {
		// Dispatch ButtonEvent
		EventContext cButton;
		Widget::ButtonEvent eButton;
		eButton.context = &cButton;
		eButton.pos = pos;
		eButton.button = button;
		eButton.action = action;
		eButton.mods = mods;
		rootWidget->onButton(eButton);
		clickedWidget = cButton.target;
	}

	if (action == GLFW_PRESS) {
		setDraggedWidget(clickedWidget, button);
	}

	if (action == GLFW_RELEASE) {
		setDragHoveredWidget(NULL);

		if (clickedWidget && draggedWidget) {
			// Dispatch DragDropEvent
			Widget::DragDropEvent eDragDrop;
			eDragDrop.button = dragButton;
			eDragDrop.origin = draggedWidget;
			clickedWidget->onDragDrop(eDragDrop);
		}

		setDraggedWidget(NULL, 0);
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			setSelectedWidget(clickedWidget);
		}

		if (action == GLFW_PRESS) {
			const double doubleClickDuration = 0.3;
			double clickTime = system::getTime();
			if (clickedWidget
			    && clickTime - lastClickTime <= doubleClickDuration
			    && lastClickedWidget == clickedWidget) {
				// Dispatch DoubleClickEvent
				Widget::DoubleClickEvent eDoubleClick;
				clickedWidget->onDoubleClick(eDoubleClick);
				// Reset double click
				lastClickTime = -INFINITY;
				lastClickedWidget = NULL;
			}
			else {
				lastClickTime = clickTime;
				lastClickedWidget = clickedWidget;
			}
		}
	}

	return !!clickedWidget;
}

bool EventState::handleHover(math::Vec pos, math::Vec mouseDelta) {
	bool cursorLocked = APP->window->isCursorLocked();

	// Fake a key RACK_HELD event for each held key
	if (!cursorLocked) {
		int mods = APP->window->getMods();
		for (int key : heldKeys) {
			int scancode = glfwGetKeyScancode(key);
			handleKey(pos, key, scancode, RACK_HELD, mods);
		}
	}

	if (draggedWidget) {
		bool dragHovered = false;
		if (!cursorLocked) {
			// Dispatch DragHoverEvent
			EventContext cDragHover;
			Widget::DragHoverEvent eDragHover;
			eDragHover.context = &cDragHover;
			eDragHover.button = dragButton;
			eDragHover.pos = pos;
			eDragHover.mouseDelta = mouseDelta;
			eDragHover.origin = draggedWidget;
			rootWidget->onDragHover(eDragHover);

			setDragHoveredWidget(cDragHover.target);
			// If consumed, don't continue after DragMoveEvent so HoverEvent is not triggered.
			if (cDragHover.target)
				dragHovered = true;
		}

		// Dispatch DragMoveEvent
		Widget::DragMoveEvent eDragMove;
		eDragMove.button = dragButton;
		eDragMove.mouseDelta = mouseDelta;
		draggedWidget->onDragMove(eDragMove);
		if (dragHovered)
			return true;
	}

	if (!cursorLocked) {
		// Dispatch HoverEvent
		EventContext cHover;
		Widget::HoverEvent eHover;
		eHover.context = &cHover;
		eHover.pos = pos;
		eHover.mouseDelta = mouseDelta;
		rootWidget->onHover(eHover);

		setHoveredWidget(cHover.target);
		if (cHover.target)
			return true;
	}
	return false;
}

bool EventState::handleLeave() {
	heldKeys.clear();
	// When leaving the window, don't un-hover widgets because the mouse might be dragging.
	// setDragHoveredWidget(NULL);
	// setHoveredWidget(NULL);
	return true;
}

bool EventState::handleScroll(math::Vec pos, math::Vec scrollDelta) {
	// Dispatch HoverScrollEvent
	EventContext cHoverScroll;
	Widget::HoverScrollEvent eHoverScroll;
	eHoverScroll.context = &cHoverScroll;
	eHoverScroll.pos = pos;
	eHoverScroll.scrollDelta = scrollDelta;
	rootWidget->onHoverScroll(eHoverScroll);

	return !!cHoverScroll.target;
}

bool EventState::handleDrop(math::Vec pos, const std::vector<std::string>& paths) {
	// Dispatch PathDropEvent
	EventContext cPathDrop;
	Widget::PathDropEvent ePathDrop(paths);
	ePathDrop.context = &cPathDrop;
	ePathDrop.pos = pos;
	rootWidget->onPathDrop(ePathDrop);

	return !!cPathDrop.target;
}

bool EventState::handleText(math::Vec pos, uint32_t codepoint) {
	if (selectedWidget) {
		// Dispatch SelectTextEvent
		EventContext cSelectText;
		Widget::SelectTextEvent eSelectText;
		eSelectText.context = &cSelectText;
		eSelectText.codepoint = codepoint;
		selectedWidget->onSelectText(eSelectText);
		if (cSelectText.target)
			return true;
	}

	// Dispatch HoverText
	EventContext cHoverText;
	Widget::HoverTextEvent eHoverText;
	eHoverText.context = &cHoverText;
	eHoverText.pos = pos;
	eHoverText.codepoint = codepoint;
	rootWidget->onHoverText(eHoverText);

	return !!cHoverText.target;
}

bool EventState::handleKey(math::Vec pos, int key, int scancode, int action, int mods) {
	// Update heldKey state
	if (action == GLFW_PRESS) {
		heldKeys.insert(key);
	}
	else if (action == GLFW_RELEASE) {
		auto it = heldKeys.find(key);
		if (it != heldKeys.end())
			heldKeys.erase(it);
	}

	if (selectedWidget) {
		// Dispatch SelectKeyEvent
		EventContext cSelectKey;
		Widget::SelectKeyEvent eSelectKey;
		eSelectKey.context = &cSelectKey;
		eSelectKey.key = key;
		eSelectKey.scancode = scancode;
		const char* keyName = glfwGetKeyName(key, scancode);
		if (keyName)
			eSelectKey.keyName = keyName;
		eSelectKey.action = action;
		eSelectKey.mods = mods;
		selectedWidget->onSelectKey(eSelectKey);
		if (cSelectKey.target)
			return true;
	}

	// Dispatch HoverKeyEvent
	EventContext cHoverKey;
	Widget::HoverKeyEvent eHoverKey;
	eHoverKey.context = &cHoverKey;
	eHoverKey.pos = pos;
	eHoverKey.key = key;
	eHoverKey.scancode = scancode;
	const char* keyName = glfwGetKeyName(key, scancode);
	if (keyName)
		eHoverKey.keyName = keyName;
	eHoverKey.action = action;
	eHoverKey.mods = mods;
	rootWidget->onHoverKey(eHoverKey);
	return !!cHoverKey.target;
}

bool EventState::handleDirty() {
	// Dispatch DirtyEvent
	EventContext cDirty;
	Widget::DirtyEvent eDirty;
	eDirty.context = &cDirty;
	rootWidget->onDirty(eDirty);
	return true;
}


} // namespace widget
} // namespace rack
