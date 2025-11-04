#pragma once

#include "EditorContentBrowser.h"

// Forward declaration to avoid circular dependency
class Renderer;

class EditorBase {
public:
	EditorBase();

	EditorContentBrowser contentBrowser;

	// Remove the Renderer instance - use external renderer instead
	// Renderer renderer;

public:
	void CreateWindowMenu();
	void CreateEditorViewport();
	void ConstructEditorLayout();
	void ConstructContentBrowser();
	void SetRenderer(Renderer* r); // Add method to set renderer

public:
	bool showViewport = true;
	bool showContentBrowser = true;

private:
	Renderer* renderer = nullptr; // Store pointer instead
};