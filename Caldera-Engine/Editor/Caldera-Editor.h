#pragma once

#include "EditorContentBrowser.h"
#include "../Rendering/Renderer.h"

class EditorBase {
public:
	EditorBase();

	EditorContentBrowser contentbrowser;

	Renderer renderer;

public:
	void CreateWindowMenu();
	void CreateEditorViewport();
	void ConstructEditorLayout();
	void ConstructContentBrowser();

public:
	bool showViewport = true;
	bool showContentBrowser = true;
};